# Developer convenience targets for code quality tooling.
# Usage: make <target> from the MuMain/ directory.

# nproc is Linux-only; stock macOS has sysctl instead.
JOBS := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu)

# The host's native preset, so the local test build matches CI's: both pick up the toolchain
# file, whose -march=x86-64 baseline the bit-for-bit animation test depends on.
UNAME_S := $(shell uname -s)
TEST_PRESET := linux-x64
ifeq ($(UNAME_S),Darwin)
TEST_PRESET := macos-arm64
endif
ifneq (,$(filter MINGW% MSYS% CYGWIN%,$(UNAME_S)))
TEST_PRESET := windows-x64
endif
# Every preset uses Ninja Multi-Config, so the configuration is chosen per build and per ctest
# run, not at configure time.
TEST_DIR := build-test
TEST_CONFIG := Debug

SOURCES := $(shell find src/source -name '*.cpp' -o -name '*.h' -o -name '*.hpp' | grep -v ThirdParty/ | grep -v 'src/source/Dotnet/')

.PHONY: format format-check lint tidy tidy-gate hooks test static

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
# The `static` label is excluded because `make static` already ran those without needing a build.
test: static
	cmake --preset $(TEST_PRESET) -B $(TEST_DIR) -DBUILD_TESTING=ON
	cmake --build $(TEST_DIR) --config $(TEST_CONFIG) -j$(JOBS)
	ctest --test-dir $(TEST_DIR) -C $(TEST_CONFIG) --output-on-failure --no-tests=error --label-exclude static --parallel $(JOBS)

# Source-only architecture guards and contracts — what CI's Quality Gates and the pre-push hook
# run first. No build and no game data needed.
static:
	cmake -S tests/static -B out/static-checks
	ctest --test-dir out/static-checks -C Release --parallel $(JOBS) --output-on-failure --no-tests=error
