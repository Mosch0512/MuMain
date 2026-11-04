# Developer convenience targets for code quality tooling.
# Usage: make <target> from the MuMain/ directory.

# Run from a git hook or any other context that sets them, these three point at the repository
# git is working on, and every recipe here runs git one way or another: the static checks build
# throwaway repositories, a configure clones FetchContent dependencies, applies patches and
# updates submodules. Unexporting an unset variable does nothing, so this needs no guard.
#
# Only these three. `git rev-parse --local-env-vars` also lists GIT_CONFIG_COUNT and
# GIT_CONFIG_PARAMETERS, which is how an environment supplies an insteadOf mirror, a proxy or an
# auth header — dropping those makes the FetchContent clones fail on exactly the setups that
# need them.
unexport GIT_DIR
unexport GIT_WORK_TREE
unexport GIT_INDEX_FILE

# nproc is Linux-only, sysctl is the macOS spelling, and getconf covers the rest. The literal
# fallback matters: an empty value leaves `cppcheck -j` and `ctest --parallel` without a number,
# and ctest before 3.29 then reads the next flag as the level.
#
# `ifndef` rather than `?=` so the detection runs once instead of on every reference, while
# `JOBS=2 make lint` from the environment still wins.
ifndef JOBS
JOBS := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
endif

# What CI runs ctest with. Higher levels make the two `dotnet run` self-checks exceed their 30 s
# timeout while the SDK warms up. Overridable from the environment, like the two below.
TEST_PARALLEL ?= 4

# The host's native preset, so the local test build matches CI's: both pick up the toolchain
# file, whose -march=x86-64 baseline the bit-for-bit animation test depends on.
#
# Architecture is part of the match, not just the OS: linux-x64.cmake's -march=x86-64 fails the
# compiler check on aarch64, and macos-arm64.cmake's CMAKE_OSX_ARCHITECTURES=arm64 produces
# binaries an x86_64 Mac cannot run during test discovery. A host that matches nothing gets no
# preset and is never guessed at; it can still be tested from a hand-configured directory.
#
# TEST_HOST_NOTE carries the one case where the host is recognized and the answer is still not a
# preset, so the message can say what to do instead of only what is missing.
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

TEST_PRESET :=
TEST_HOST_NOTE :=

ifeq ($(UNAME_S),Linux)
ifeq ($(UNAME_M),x86_64)
TEST_PRESET := linux-x64
endif
endif

ifeq ($(UNAME_S),Darwin)
# uname says x86_64 in a Rosetta shell, so ask the kernel what the machine is. A Rosetta shell
# must not map to macos-arm64: /usr/local Homebrew would hand the arm64 build x86_64 curl,
# brotli, cryptopp and openssl, and the link fails with "building for macOS-arm64 but attempting
# to link with file built for macOS-x86_64". Running natively is the fix, so ask for that.
MACOS_IS_ARM64 := $(shell sysctl -n hw.optional.arm64 2>/dev/null)
ifeq ($(UNAME_M),arm64)
TEST_PRESET := macos-arm64
else ifeq ($(MACOS_IS_ARM64),1)
# No backticks in the note: the host check echoes it inside double quotes, and the shell would
# run it as a command substitution.
TEST_HOST_NOTE := this is an arm64 Mac in a Rosetta x86_64 shell; run: arch -arm64 make test
endif
endif

# Git Bash and the MSYS2 MINGW64, UCRT64 and CLANG64 shells run the native cmake.exe, which
# reports hostSystemName Windows, so windows-base's condition holds there — the setup
# .githooks/pre-push already supports. Excluded on purpose:
#
#   - the MSYS shell, whose /usr/bin/cmake reports hostSystemName MSYS and so disables the
#     windows presets, with a message about no preset matching rather than about the shell;
#   - arm64 and 32-bit hosts, because only windows-x86 and windows-x64 exist and the x86 one
#     needs its own vcpkg triplet; those get the hand-configured path.
ifneq (,$(filter MINGW%,$(UNAME_S)))
ifeq ($(UNAME_M),x86_64)
TEST_PRESET := windows-x64
endif
endif
# Release, because that is what every native CI job builds and tests. The configuration is not
# cosmetic: MuClient's PUBLIC definitions differ between the two (_DEBUG/_FOREIGN_DEBUG vs
# NDEBUG/_FOREIGN_NDEBUG/LDS_PATCH_GLOBAL_100520), and Debug's -O0 hides the fused multiply-add
# differences the toolchain's -march baseline exists to pin down.
#
# `make test TEST_CONFIG=Debug` builds the other one, in its own directory, for the assert()s
# and the _DEBUG code paths Release never compiles, and `TEST_CONFIG=Debug make test` from the
# environment does the same.
TEST_CONFIG ?= Release

# Out of tree, under the already ignored /out/, and one directory per configuration:
#
#   - A directory the old recipe configured with Unix Makefiles cannot be reused by a preset
#     ("generator does not match"), and CMake never re-applies a toolchain file to an existing
#     cache, so -march would silently stay off.
#   - `make tidy-gate` looks for compile_commands.json under out/build/*/.
#   - doctest writes one discovered-tests file per target, naming the config that was built last.
#     Sharing a directory between configurations makes `ctest -C Debug` run Release binaries
#     whenever that config is already up to date.
TEST_DIR := out/build/test-$(TEST_CONFIG)

# What the native CI jobs pass on top of the preset. MU_COPY_RUNTIME_ASSETS=OFF skips staging
# the ~760 MB of src/bin on every build; the tests read their fixtures from the source tree.
# BUILD_TESTING stays explicit even though linux-base and macos-base set it: windows-base does
# not, and Git Bash hosts configure through windows-x64.
#
# Recursively expanded, so `brew --prefix` runs when a configure needs it rather than on every
# make invocation, including `make format`. Homebrew's OpenSSL is not in a default search path
# and find_package(OpenSSL REQUIRED) fails without the hint.
TEST_CONFIGURE_FLAGS = -DBUILD_TESTING=ON -DMU_COPY_RUNTIME_ASSETS=OFF $(TEST_OPENSSL_FLAG)
ifeq ($(UNAME_S),Darwin)
TEST_OPENSSL_FLAG = -DOPENSSL_ROOT_DIR=$(shell brew --prefix openssl@3 2>/dev/null)
endif

SOURCES := $(shell find src/source -name '*.cpp' -o -name '*.h' -o -name '*.hpp' | grep -v ThirdParty/ | grep -v 'src/source/Dotnet/')

.PHONY: format format-check lint tidy tidy-gate hooks test unit static check-test-host

# Format all C++ source files in-place
format:
	@echo "Formatting $(words $(SOURCES)) files..."
	@echo $(SOURCES) | xargs clang-format -i
	@echo "Done."

# Check formatting without modifying files (exits non-zero on diff)
format-check:
	@echo "Checking formatting..."
	@echo $(SOURCES) | xargs clang-format --dry-run --Werror

# Run cppcheck static analysis
#
# Suppression policy (see also: docs/cppcheck-guidance.md)
#
#   GLOBALLY SUPPRESSED (style-only, never indicate bugs):
#     missingInclude        — cppcheck can't resolve Win32/DirectX/third-party paths
#     unmatchedSuppression  — avoids noise across cppcheck versions
#     unusedFunction        — too many false positives without whole-program analysis
#     useInitializationList — style preference, not a correctness issue
#     passedByValue         — performance style (pass-by-const-ref)
#     postfixOperator       — trivial perf (++i vs i++)
#     returnByReference     — performance style
#     *:*/ThirdParty/*      — don't lint third-party code
#
#   ENFORCED (catch real bugs — legacy violations use inline cppcheck-suppress):
#     uninitMemberVar / uninitMemberVarPrivate — uninitialized members
#     dangerousTypeCast    — unsafe C-style casts
#     noOperatorEq / noCopyConstructor — Rule of Three violations
#     duplInheritedMember  — accidental base class member shadowing
#
#   When adding new code, do NOT add inline suppressions for these enforced
#   checks. Fix the issue instead. Existing suppressions are legacy baseline.
#
lint:
	@echo "Running cppcheck..."
	@echo $(SOURCES) | xargs cppcheck \
		-j$(JOBS) \
		--error-exitcode=1 \
		--enable=warning,performance,portability \
		--std=c++20 \
		--language=c++ \
		--suppress=missingInclude \
		--suppress=unmatchedSuppression \
		--suppress=unusedFunction \
		--suppress=*:*/ThirdParty/* \
		--suppress=useInitializationList \
		--suppress=passedByValue \
		--suppress=postfixOperator \
		--suppress=returnByReference \
		--inline-suppr

# Auto-detect build directory containing compile_commands.json.
# Prefers the native build (out/build/*) over the cross-compile (build-mingw).
BUILD_DIR := $(firstword $(wildcard out/build/*/compile_commands.json))
BUILD_DIR := $(if $(BUILD_DIR),$(dir $(BUILD_DIR)),$(if $(wildcard build-mingw/compile_commands.json),build-mingw/,))

# Run clang-tidy with full .clang-tidy config (manual use — noisy on legacy code)
tidy:
	@if [ -z "$(BUILD_DIR)" ]; then \
		echo "Error: compile_commands.json not found. Run cmake configure first."; \
		exit 1; \
	fi
	@echo "Running clang-tidy (full config) against $(BUILD_DIR)..."
	@echo $(SOURCES) | xargs clang-tidy -p $(BUILD_DIR)

# Focused clang-tidy gate: only high-value portability/correctness checks.
# These catch real bugs (e.g. sizeof(pointer) on 64-bit) without legacy noise.
# Pre-existing compilation errors (missing include guards etc.) are ignored —
# the build step already validates compilation. Only bugprone-* findings fail.
TIDY_CHECKS := -*,bugprone-sizeof-expression,bugprone-suspicious-memset-usage,bugprone-misplaced-pointer-arithmetic-in-alloc,-clang-diagnostic-*
tidy-gate:
	@if [ -z "$(BUILD_DIR)" ]; then \
		echo "Error: compile_commands.json not found. Run cmake configure first."; \
		exit 1; \
	fi
	@echo "Running clang-tidy portability gate against $(BUILD_DIR)..."
	@FINDINGS=$$(echo $(SOURCES) | xargs clang-tidy -p $(BUILD_DIR) \
		--checks='$(TIDY_CHECKS)' \
		--extra-arg=-Wno-everything 2>&1 \
		| grep -c '\[bugprone-' || true); \
	if [ "$$FINDINGS" -gt 0 ]; then \
		echo $(SOURCES) | xargs clang-tidy -p $(BUILD_DIR) \
			--checks='$(TIDY_CHECKS)' \
			--extra-arg=-Wno-everything 2>&1 \
			| grep -E '\[bugprone-'; \
		echo "$$FINDINGS bugprone finding(s) — fix before committing."; \
		exit 1; \
	fi
	@echo "0 bugprone findings."

# Install git hooks
hooks:
	@bash scripts/install-hooks.sh

# Build everything and run the unit tests, the way the native CI jobs do.
#
# The default target is the one that works: besides the doctest binaries from mu_add_test(),
# tests/ defines standalone fixtures (rml_text_layout_tests, texture_failure_fixture, ...) and
# tests that read $<TARGET_OBJECTS:Main> or an OBJECT-library fixture. A target that covers only
# part of that leaves tests running against stale binaries or failing outright.
#
# The `static` label is excluded here because `make static` covers those without needing a
# build; `make test` runs both.
#
# Sub-makes rather than prerequisites, so the three stages stay in this order under `make -j`
# instead of a quick static failure waiting behind a long first configure. The host check comes
# first: on a host without a preset there is no point running the static suite for two minutes
# before saying so.
test:
	@$(MAKE) --no-print-directory check-test-host
	@$(MAKE) --no-print-directory static
	@$(MAKE) --no-print-directory unit

# The unit tests alone. Useful while a static check is failing for an unrelated reason, and the
# inner loop when iterating on one test.
unit: check-test-host
	cmake --preset $(TEST_PRESET) -B $(TEST_DIR) $(TEST_CONFIGURE_FLAGS)
	cmake --build $(TEST_DIR) --config $(TEST_CONFIG) -j$(JOBS)
	ctest --test-dir $(TEST_DIR) -C $(TEST_CONFIG) --output-on-failure --no-tests=error --label-exclude static --parallel $(TEST_PARALLEL)

# Source-only architecture guards and contracts — what CI's Quality Gates and the pre-push hook
# run first. No build and no game data needed.
static:
	cmake -S tests/static -B out/static-checks
	ctest --test-dir out/static-checks -C Release --parallel $(JOBS) --output-on-failure --no-tests=error

# What the presets need beyond a compiler, checked before a configure fails halfway in. An
# already configured directory is taken as-is: it names its own CMAKE_MAKE_PROGRAM, and a host
# this Makefile has no preset for can still have been configured by hand.
check-test-host:
	@if [ -f "$(TEST_DIR)/CMakeCache.txt" ]; then exit 0; fi; \
	if [ -z "$(TEST_PRESET)" ]; then \
		echo "make test: no CMake preset for $(UNAME_S)/$(UNAME_M). Configure $(TEST_DIR)" >&2; \
		echo "           yourself (docs/build/reference-tests.md); make test then uses it." >&2; \
		if [ -n "$(TEST_HOST_NOTE)" ]; then echo "           Note: $(TEST_HOST_NOTE)" >&2; fi; \
		exit 1; \
	fi; \
	if [ "$(TEST_PRESET)" = "windows-x64" ]; then \
		if [ -z "$${VCPKG_ROOT:-}" ]; then \
			echo "make test: windows-x64 reads its toolchain from VCPKG_ROOT, which is unset." >&2; \
			echo "           docs/build/windows has the vcpkg setup." >&2; \
			exit 1; \
		fi; \
		if [ -z "$${VCINSTALLDIR:-}$${VSCMD_ARG_TGT_ARCH:-}" ]; then \
			echo "make test: windows-x64 sets architecture strategy external, so it needs an" >&2; \
			echo "           active MSVC environment. Start the shell from an x64 Native Tools" >&2; \
			echo "           prompt, or run vcvarsall.bat x64 first." >&2; \
			exit 1; \
		fi; \
	fi; \
	for generator in ninja ninja-build samu; do \
		command -v "$$generator" >/dev/null && exit 0; \
	done; \
	echo "make test: the $(TEST_PRESET) preset generates Ninja Multi-Config, and none of" >&2; \
	echo "           ninja, ninja-build or samu is on PATH." >&2; \
	exit 1
