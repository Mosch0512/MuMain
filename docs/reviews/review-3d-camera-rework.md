# Code Review: 3d-camera-rework

**Date:** 2026-04-10
**Base:** main
**Commits reviewed:**
- a43084c9 Add CameraState and CameraProjection utilities
- 1d3d7710 Add CameraConfig, CameraMode, and ICamera interface
- 032f7582 Add frustum culling system
- 58fc27e9 Add DefaultCamera implementation
- 3e81ae90 Add OrbitalCamera with spherical orbit controls
- 0d1e8002 Add LegacyCamera for A/B performance baseline
- 3fd0cc63 Add FreeFlyCamera for editor spectator mode
- 1d805371 Add CameraManager mode-switching singleton
- 9cead0b3 Migrate camera globals to CameraState and refactor projection calls
- 3e66f675 Integrate frustum culling into terrain and object rendering
- 4be206ae Update scene rendering and fix scene transitions
- c142a845 Add window resize and resolution handling
- e2f104e7 Add DevEditor UI panel for camera and graphics debugging
- d3b6339c Apply DevEditor render toggles to game code
- 3434c630 Add percentage-based fog system with per-world fog colors
- 2cfce03d Improve character pick detection and fix viewport ray calculation
- 8455976c Increase login scene render distance for touring camera
- d89251c2 Reorganize DevEditor UI into scene-based layout
- 7ba4c4d7 Add login scene render distance sliders and scene-isolate DevEditor settings
- b775d71d Remove LegacyCamera -- no longer needed
- 04a68077 Persist orbital camera zoom level in config.ini
- 79914f57 Clean up DevEditor: remove unused culling debug, fix DefaultCamera flicker
- 8f10b53f Fix PR #29 review issues and window resize
- 0875eb6a Unify ESC menu handling and add option window to login/character scenes
- 324afddd Split volume config into SoundVolume/MusicVolume and always init audio
- 9ab2d480 Add music volume slider and resolution selector to option window

## Summary

This is a large branch (25 commits, 77 files, +7347/-971 lines) that replaces the legacy camera system with a well-architected multi-camera framework including DefaultCamera, OrbitalCamera, FreeFlyCamera, frustum culling, a DevEditor debug panel, and option window improvements. The overall architecture is solid -- the ICamera interface, CameraManager singleton, and separation into CameraConfig/CameraState/CameraProjection show good design thinking. However, several implementation files had grown too large, there was significant code duplication between camera implementations, and magic numbers were scattered throughout.

**Initial verdict:** NEEDS WORK
**Post-fix verdict:** READY (all 65 flagged issues resolved)

### Resolution summary
- **58 FIXED** — code was refactored to address the violation
- **3 FALSE POSITIVE** — rule not actually violated (idiomatic usage or codebase-wide convention)
- **1 MISATTRIBUTED** — issue was in a different file
- **2 ALREADY FIXED** — comment/fix already in place when the review ran
- **1 PARTIALLY FIXED** — later revisited and fully resolved (see ZzzCharacter.cpp)

## File Reviews

### src/source/Camera/CameraConfig.h (PASS)

Well-structured configuration with named constants, clear documentation, and appropriate presets. No violations.

### src/source/Camera/CameraMode.h (PASS)

Simple, focused enum with straightforward helper functions. No violations.

### src/source/Camera/ICamera.h (PASS)

Clean interface definition with clear contract. No violations.

### src/source/Camera/CameraProjection.h (PASS)

Clear interface, well-documented static methods. No violations.

### src/source/Camera/CameraUtility.h (PASS)

Clean minimal header. No violations.

### src/source/Camera/CameraUtility.cpp (PASS)

Simple, focused file with single responsibility. No violations.

### src/source/Camera/CameraState.h (MINOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 30 | 3 | ~~`ZoomLevel` as `short` with implicit range 0-5, no named bounds~~ **FALSE POSITIVE** -- bounds enforced by switch cases in DefaultCamera; OrbitalCamera has explicit MIN/MAX_RADIUS constants | N/A |
| 32-34 | 9 | ~~Legacy fields `FOV3D`, `Roll3D` marked unused but kept -- confusing~~ **FIXED** -- removed dead code, no references anywhere | N/A |

### src/source/Camera/CameraState.cpp (MINOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 15-42 | 7 | ~~`Reset()` uses both `IdentityVector3D()` and direct assignment inconsistently~~ **FALSE POSITIVE** -- `IdentityVector3D()` is the idiomatic way to zero `vec3_t` arrays; direct assignment is correct for scalars | N/A |

### src/source/Camera/CameraManager.h (MINOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 66-84 | 9 | ~~Multiple related spectated-state booleans/vectors scattered across class~~ **FIXED** -- grouped into `SpectatedState` nested struct | N/A |

### src/source/Camera/CameraManager.cpp (MAJOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 82-130 | 1 | ~~`Update()` is ~69 lines with complex spectated camera state management~~ **FIXED** -- extracted `UpdateSpectatedCamera()` | N/A |
| 63-133 | 2 | ~~Nested `#ifdef` blocks create deep nesting and complex control flow~~ **FIXED** -- spectated logic moved to dedicated helper | N/A |
| 192-245 | 1 | ~~`TransitionToCamera()` is 53 lines with complex state transitions~~ **FIXED** -- uses `SaveGlobalToSpectated()` / `RestoreSpectatedToGlobal()` helpers | N/A |
| 192-245 | 4 | ~~Editor-only spectated state save/restore code duplicated between `Update()` and `TransitionToCamera()`~~ **FIXED** -- shared via `SaveGlobalToSpectated()` / `RestoreSpectatedToGlobal()` | N/A |

### src/source/Camera/CameraProjection.cpp (MAJOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 56-78 | 3 | ~~Scale factors `640`, `480` for coordinate conversion are magic numbers~~ **FIXED** -- added `REFERENCE_WIDTH`/`REFERENCE_HEIGHT` constants to `stdafx.h` and replaced across all branch-changed files | N/A |
| 113 | 3 | ~~Threshold `1.0f` in terrain cull distance comparison unexplained~~ **MISATTRIBUTED** -- this line is in Frustum.cpp:113, not CameraProjection.cpp | N/A |

### src/source/Camera/DefaultCamera.h (MAJOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 61-103 | 1 | ~~Class has 30+ members mixing frustum state, scene tracking, editor state~~ **FIXED** -- grouped 7 cache fields into `FrustumCache` nested struct; constructor simplified to `m_FrustumCache = {}` | N/A |

### src/source/Camera/DefaultCamera.cpp (CRITICAL)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 70-196 | 1 | ~~`ResetForScene()` is 126 lines~~ **FIXED** -- split into `InitCharacterScene()`, `InitMainScene()`, `InitLoginScene()`, `ApplyConfigToState()`, `InvalidateFrustumCache()`. Now ~25 lines | N/A |
| 125-168 | 2 | ~~LoginScene initialization has 3 nested if/else levels~~ **FIXED** -- `InitLoginScene()` uses early returns | N/A |
| 198-259 | 1 | ~~`OnActivate()` is 62 lines with complex state transitions~~ **FIXED** -- debug boilerplate replaced with `CAMERA_LOG()` macro, dead commented code removed, down to ~45 lines | N/A |
| 145-167 | 3 | ~~Hardcoded `150.0f`, `500.0f`, `80.0f`, `-75.0f`~~ **FIXED** -- named constants in anonymous namespace: `CHAR_SCENE_CAM_X/Y/Z/PITCH/ROLL`, `MAIN_SCENE_CHARACTER_HEIGHT_OFFSET`, `LOGIN_SCENE_FALLBACK_Z/PITCH`, `CACHE_INVALIDATE_SENTINEL` | N/A |
| 206+ | 4 | ~~`sprintf_s + LogEditor()` debug pattern repeated 6+ times, same in OrbitalCamera~~ **FIXED** -- created `src/source/Camera/CameraDebugLog.h` with `CAMERA_LOG(fmt, ...)` macro; replaced 13 occurrences across DefaultCamera.cpp and OrbitalCamera.cpp; removed now-unused include from CameraManager.cpp | N/A |

### src/source/Camera/FreeFlyCamera.h (MINOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 86-87 | 3 | ~~`MIN_PITCH = -160.0f` and `MAX_PITCH = -20.0f` both negative with no explanation~~ **ALREADY FIXED** -- comment `// Engine convention: Angle[0]=-90 = horizontal, more negative = looking up` was already in place | N/A |

### src/source/Camera/FreeFlyCamera.cpp (MAJOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 140-195 | 1 | ~~`HandleMovement()` is 54 lines combining input reading, normalization, speed calc, velocity~~ **FIXED** -- extracted `ReadMovementInput()` and uses early-return guard; down to ~25 lines | N/A |
| 170-174 | 3 | ~~Hardcoded `0.3f` (sensitivity), `360.0f` (yaw normalization)~~ **FIXED** -- named `MOUSE_LOOK_SENSITIVITY` and `FULL_ROTATION_DEG` in anonymous namespace | N/A |
| 225-268 | 1 | ~~`UpdateFrustum()` is 43 lines with repetitive angle/vector calculations~~ **FIXED** -- duplicated yaw/pitch→forward math extracted to `ComputeForwardVector()` helper (also used by `HandleMovement`); removed unnecessary temp VectorCopy dance; down to ~28 lines. Also added `ComputeRightVectorXY()` helper | N/A |

### src/source/Camera/Frustum.h (MINOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 8-24 | 6 | ~~`AABB` struct defined in Frustum header but is an independent data structure~~ **FALSE POSITIVE** -- rule 6 allows small helper structs tightly coupled to a single class to stay in that class's header; AABB is only used for frustum culling | N/A |
| 154 | 3 | ~~Max hull points hardcoded as `[12]` without documentation~~ **ALREADY FIXED** -- comment at line 153 already explains: `// Max 12 points: 8 frustum corners + up to 4 terrain-cull far corners` | N/A |

### src/source/Camera/Frustum.cpp (MAJOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 33-192 | 1 | ~~`BuildFromCamera()` is 159 lines~~ **FIXED** -- split into `CalculateFrustumVertices()`, `CalculateTerrainExtension()`, `CalculatePlanes()`. `BuildFromCamera()` is now ~35 lines | N/A |
| 113-139 | 4 | ~~Terrain far vertices calculation duplicates vertex calc logic from lines 74-108~~ **FIXED** -- shared via `ComputePlaneCorners()` helper in anonymous namespace; called by both frustum corners and terrain extension | N/A |
| 262-341 | 1 | ~~`Calculate2DProjection()` is 79 lines with struct definition, insertion sort, and convex hull all inline~~ **FIXED** -- `Point2D`, `Cross2D`, `SortPoints2D`, `ConvexHullCCW` extracted to anonymous namespace helpers; `Calculate2DProjection()` is now ~35 lines | N/A |
| 38, 113 | 3 | ~~Threshold `1.0f` in `terrainCullDist > farDist + 1.0f` unexplained~~ **FIXED** -- named `TERRAIN_EXTENSION_EPSILON` with explanatory comment | N/A |
| 273, 281 | 3 | Magic `0.01f` world→tile conversion | **FIXED** -- named `WORLD_TO_TILE = 1.0f / TERRAIN_SCALE`, `MAX_HULL_POINTS = 12` |

### src/source/Camera/FrustumRenderer.h (MAJOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 31-225 | 1 | ~~`RenderFrustumWireframe()` is 194 lines inline~~ **FIXED** -- moved implementation to new `FrustumRenderer.cpp`; header only forward-declares. Split into `ComputeFrustumApex()`, `ScaleFarVerticesFromApex()`, `RenderPyramidWireframe()`, `RenderPyramidFilled()`, `RenderGroundProjection()`, `RenderCameraMarker()`. Entry-point function is now ~15 lines | N/A |
| 39-82 | 2 | ~~Camera position calculation has multiple nesting levels for plane distance checks~~ **FIXED** -- extracted to `ComputeFrustumApex()` with `AverageQuad()` / `Distance3D()` primitives | N/A |
| 163-217 | 3 | ~~Hardcoded `5.0f` z-offset, `2.0f * TERRAIN_SCALE` subdivision, `32` max segments, `50.0f` marker size~~ **FIXED** -- named `GROUND_LINE_Z_OFFSET`, `SUBDIVISIONS_PER_TILE`, `MAX_EDGE_SEGMENTS`, `CAMERA_MARKER_HALF_LENGTH`, `WIREFRAME_LINE_WIDTH`, `GROUND_LINE_WIDTH`, `DEGENERATE_EPSILON` | N/A |
| 95-224 | 8 | Manual GL state save/restore pattern is error-prone on early exits | **FIXED** -- added `GLStateScope` RAII helper in anonymous namespace; state is automatically restored on destruction | N/A |

### src/source/Camera/OrbitalCamera.h (MAJOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 69-103 | 1 | ~~21 member variables mixing orbital math, input state, constraints, scene tracking~~ **PARTIALLY FIXED** -- grouped the 4 mouse-drag fields into `InputState` struct. Orbital state (yaw/pitch/radius) and scene tracking left as-is because they're used across many methods with already-clear naming; grouping them would add indirection without real clarity gains | N/A |

### src/source/Camera/OrbitalCamera.cpp (CRITICAL)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 72-149 | 4 | ~~`ResetForScene()` duplicates DefaultCamera logic~~ **FIXED** -- extracted `LoadConfigForScene()` and `ApplyConfigToState()` helpers. Character scene constants `POSITION_X/Y/Z`, `ANGLE_PITCH`, `ANGLE_ROLL` moved to shared `CharacterSceneCamera` namespace in `CameraConfig.h` so both cameras reference the same values. `ResetForScene()` down to ~38 lines | N/A |
| 151-349 | 1 | ~~`OnActivate()` is ~198 lines~~ **FIXED** -- split into `LoadConfigForScene()`, `ApplyConfigToState()`, `CalculateOrbitOriginForStaticScene()`, `CalculateLookAtPoint()`, `InitializeOrbitalFromCurrentState()`, `SyncStateToGlobalCamera()`. `OnActivate()` down to ~54 lines, reads as clear numbered steps | N/A |
| 229-274 | 2 | ~~3+ levels of nesting in non-MainScene branch for orbit origin calculation~~ **FIXED** -- extracted to `CalculateOrbitOriginForStaticScene()` with sanity-check constant `ORBIT_ORIGIN_MAX_T` | N/A |
| 145-332 | 4 | ~~Debug logging pattern repeated 6+ times~~ **FIXED** -- already addressed in earlier DefaultCamera pass by creating `CAMERA_LOG` macro in `CameraDebugLog.h`; all 7 OrbitalCamera debug blocks now use it | N/A |
| 211-221 | 3 | Magic numbers `300.0f`, `-45.0f`, `5000.0f`, `2000.0f` for static-scene orbit origin sanity checks | **FIXED** -- named `NON_CHAR_STATIC_CAM_HEIGHT_OFFSET`, `NON_CHAR_STATIC_CAM_PITCH`, `ORBIT_ORIGIN_MAX_T`, `LOOKAT_MAX_T` in anonymous namespace | N/A |

---

### src/source/Winmain.cpp (MAJOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 966-1017 | 1 | ~~`ReinitializeFonts()` is 52 lines~~ **FIXED** -- split into `CalculateFontSizes()`, `CreateTahoma()`, `CreateNewFonts()`, `ReinitializeTextRenderer()`, `RefreshInventoryEquipmentSlots()` in anonymous namespace. Public `ReinitializeFonts()` is now ~18 lines and reads as clear steps. Also deduplicated the font creation block that was copy-pasted in `WinMain()` (rule 4) — WinMain now just calls `CreateNewFonts(CalculateFontSizes())` | N/A |
| 576-586 | 3 | ~~`WM_SIZE` handler uses hardcoded `640.0f`, `480.0f` reference resolution~~ **FIXED** -- already addressed earlier with global `REFERENCE_WIDTH` / `REFERENCE_HEIGHT` constants in `stdafx.h` | N/A |
| 1354-1372 | 4 | ~~Audio initialization logic duplicated from option window~~ **FIXED** -- extracted `InitializeAudioSystem()` helper with named `VOLUME_SCALE_FACTOR`, `VOLUME_MIN/MAX/DEFAULT` constants and `ClampVolume()`. Note: NewUIOptionWindow does not actually duplicate the init; it only adjusts volume at runtime on slider changes, which is a different concern and left alone | N/A |
| 969-973 | 3 | Magic numbers in font sizing logic: `12`, `200.f`, `14`, `15`, `600` | **FIXED** -- named `BASE_FONT_HEIGHT`, `FONT_HEIGHT_GROWTH_PER_PIXEL`, `FIX_FONT_HEIGHT_SMALL/LARGE`, `SMALL_WINDOW_HEIGHT_THRESHOLD` | N/A |

### src/source/Scenes/LoginScene.cpp (PASS)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 220-245 | 9 | ~~`InterpolateCameraMovement()` has embedded "FIX" comment with unclear explanation~~ **FIXED** -- replaced historical "FIX:" comments with a proper docstring describing both modes (smooth vs linear), combined position/angle loops for each mode, named `SMOOTH_EASING_FACTOR` constant | N/A |

### src/source/Scenes/MainScene.cpp (PASS)

No significant violations in changed code.

### src/source/Scenes/CharacterScene.cpp (MINOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 294-310 | 3 | ~~Character height adjustment uses hardcoded `194.5f`, `169.5f`~~ **FIXED** -- named `CHARACTER_Z_ON_DINORANT` and `CHARACTER_Z_DEFAULT`, also collapsed if/else to a ternary | N/A |
| 350-351 | 3 | ~~Aurora luminance magic numbers~~ **FIXED** -- named `AURORA_FREQUENCY`, `AURORA_AMPLITUDE`, `AURORA_BASE_LUMINANCE` with a comment explaining the pulse range | N/A |

### src/source/Scenes/SceneManager.cpp (MINOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 332-393 | 4 | ~~`SetWorldClearColor()` has massive if-else chain with repeated `glClearColor()` + `FogColor` pattern~~ **FIXED** -- extracted `SetClearAndFogColor(r, g, b)` helper + local `rgb8(r, g, b)` lambda for byte-scale colors with named `BYTE_TO_FLOAT = 1/256.f`. Every branch collapsed from 2 lines to 1, function ~45 lines → ~35 lines and much easier to scan | N/A |

### src/source/NewUIOptionWindow.cpp (MAJOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 107-259 | 1 | ~~`UpdateMouseEvent()` is 152 lines~~ **FIXED** -- split into `HandleCheckboxInputs()`, `HandleVolumeSlider()`, `OnSoundVolumeChanged()`, `OnMusicVolumeChanged()`, `HandleRenderLevelSlider()`, `HandleResolutionArrows()`. Public entry is now ~25 lines | N/A |
| 133-225 | 4 | ~~Sound volume slider and music slider are nearly identical~~ **FIXED** -- extracted shared `HandleVolumeSlider(int& level, int yOffset)` helper; it returns `true` on change so callers dispatch to distinct `OnSoundVolumeChanged()` / `OnMusicVolumeChanged()` handlers. Duplicate clamping collapsed into a single `std::clamp(level, 0, MAX_VOLUME)` | N/A |
| 154-195 | 3 | ~~`(10.f * x) / 124.f` magic numbers~~ **FIXED** -- named `SLIDER_WIDTH`, `MAX_VOLUME`, `SLIDER_HIT_PADDING`, `SLIDER_HIT_HEIGHT`, `SLIDER_X_LOCAL`, `WZAUDIO_VOLUME_SCALE`, plus render-level and resolution-arrow constants | N/A |
| 115-131 | 1 | 4 checkbox toggles were copy-pasted with only coordinates differing | **FIXED** -- `HandleCheckboxInputs()` uses a local `Checkbox` table driving a single loop; checkbox positions are data, not branches | N/A |

### src/source/NewUIOptionWindow.h (PASS)

No significant violations in changed code.

### src/source/ZzzLodTerrain.cpp (CRITICAL)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 2020-2103 | 1 | ~~`BuildHull2DAndBounds()` is 84 lines~~ **FIXED** -- extracted `SortPointsByXThenY()`, `Cross2D()`, `ConvexHullCCW()` to anonymous namespace, and `ComputeIterationBoundsFromHull()` as a file-static function. `BuildHull2DAndBounds()` is now ~20 lines reading as clear steps | N/A |
| 2048-2071 | 9 | ~~Andrew's monotone chain dense with no comments~~ **FIXED** -- split lower-hull and upper-hull phases into labeled sections within `ConvexHullCCW()` | N/A |
| 2084-2103 | 3 | ~~Hardcoded `12` for max hull vertices, `4` for tile width~~ **FIXED** -- named `MAX_HULL_VERTICES`, `TERRAIN_ITERATION_TILE`, `EDGE_LENGTH_EPSILON`, `BISECTOR_COS_MIN`. `ResetFrustrumBoundsFullTerrain()` now uses the tile constant too | N/A |
| 2158-2176 | 4 | Iteration-bounds computation duplicated between `BuildHull2DAndBounds` and `ExpandHullOutward` (same 18-line block) | **FIXED** -- both now call `ComputeIterationBoundsFromHull()` | N/A |

### src/source/ZzzOpenglUtil.cpp (PASS)

No significant violations in changed code. Projection setup changes are clean.

### src/source/ZzzObject.cpp (PASS)

Frustum culling integration is clean. ICamera parameter addition is well done.

### src/source/ZzzInterface.cpp (MINOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 7920-7948 | 3 | ~~OBB construction magic numbers~~ **FIXED** -- named `IN_GAME_HEIGHT_GROWTH`, `IN_GAME_PICK_PADDING`, `CHAR_SCENE_MIN_HEIGHT`, `CHAR_SCENE_HALF_WIDTH` in-place with explanatory comments; removed the `10.0f` which was `2 * IN_GAME_PICK_PADDING` | N/A |

### src/source/GameConfig/GameConfig.cpp (PASS)

Clean additions for SoundVolume/MusicVolume. No violations.

### src/source/GameConfig/GameConfig.h (PASS)

No violations.

### src/source/GameConfig/GameConfigConstants.h (PASS)

No violations.

### src/source/CullingConstants.h (PASS)

Properly named constants. No violations.

### src/source/CameraMove.cpp (MINOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 126-149 | 3 | ~~LoginScene offset defaults lack documentation of why~~ **FIXED** -- expanded the header comment to explain the waypoint file was authored against an older terrain version; replaced the magic `73` with `WD_73NEW_LOGIN_SCENE` and applied an early-return pattern | N/A |

### src/source/ZzzCharacter.cpp (MINOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 11214-11246 | 1 | ~~Debug visualization code (~33 lines) added inline in main rendering loop~~ **FIXED** -- extracted to `RenderCharacterPickBoxDebug(o)` in an `_EDITOR`-only anonymous namespace | N/A |
| 11283-11311 | 4 | ~~Pick-box dimensions duplicated between debug viz and `SelectCharacter()`~~ **FULLY FIXED** -- added shared `BuildCharacterPickOBB(o, mainScene, outOBB)` declared in `ZzzCharacter.h`; `SelectCharacter()` in ZzzInterface.cpp (45 lines → 2 lines) and the debug overlay both call it. Named constants (`PICK_IN_GAME_*`, `PICK_CHAR_SCENE_*`) now live in a single place | N/A |

### src/source/ZzzEffectFireLeave.cpp (PASS)

Minor changes, no significant violations in changed code.

### src/source/NewUI3DRenderMng.cpp (PASS)

Clean additions. No violations.

### src/MuEditor/UI/DevEditor/DevEditorUI.cpp (MAJOR)

| Line(s) | Rule | Issue | Suggestion |
|----------|------|-------|------------|
| 77-170 | 1 | ~~`RenderScenesTab()` is 93 lines~~ **FIXED** -- `RenderScenesTab()` is actually ~380 lines counting all collapsing headers. Split into `RenderCameraModeControls()`, `RenderCameraSummaryLine()`, `RenderLoginSceneSection()`, `RenderGameSceneSection()`, `RenderScenesDebugSection()`. Dispatcher is now ~30 lines and reads as a clear section list | N/A |
| 459-550 | 1 | ~~`RenderGraphicsTab()` is 91 lines~~ **FIXED** -- `RenderGraphicsTab()` was actually ~335 lines. Split into `RenderGraphicsDebugInfo()`, `RenderWindowSizePresets()`, `RenderCustomResolutionInput()`, `RenderFullscreenToggle()`, plus a shared `ApplyNewWindowSize()` helper that eliminated ~40 lines of duplicated save/resize logic (used by presets, custom size, and fullscreen toggle — rule 4) | N/A |
| 91 | 3 | ~~`m_LoginTerrainDist = 3995.0f` and `m_LoginObjectDist = 5903.0f` are magic reset values~~ **FIXED** -- promoted to `LoginSceneCameraDefaults::RENDER_TERRAIN_DIST` / `RENDER_OBJECT_DIST` in `CameraMove.h`. Also eliminated rule 4 duplication: ZzzObject.cpp's `5903.0f` non-editor fallback and LoginScene.cpp's `3995.f` fallback now reference the same constants | N/A |
| 306, 332 | 3 | ~~FOV range `10.0f, 150.0f` hardcoded in ImGui slider calls~~ **FIXED** -- named `MIN_HFOV`, `MAX_HFOV` in anonymous namespace. Also named `CAMERA_MODE_DEFAULT/ORBITAL/FREEFLY`, `LOGIN_DIST_MIN/MAX`, `CUSTOM_RES_MAX_WIDTH/HEIGHT`, `WORLD_TO_TILE_DIVISOR` | N/A |
| 188-215 | 4 | Reset Offsets button had hardcoded `-300.0f`, `650.0f`, `950.0f`, `40.0f`, `-5.0f` duplicating CameraMove.cpp's initializers | **FIXED** -- both files now use the `LoginSceneCameraDefaults` namespace constants |

### src/MuEditor/UI/DevEditor/DevEditorUI.h (PASS)

No violations.

### src/MuEditor/Core/MuEditorCore.cpp (PASS)

Clean integration of DevEditor toggle. No violations.

### src/MuEditor/UI/Common/MuEditorUI.cpp (PASS)

Toolbar extension is clean. No violations.

## Positive Notes

- **Good architecture**: The ICamera interface, CameraManager singleton, CameraConfig/CameraState/CameraProjection separation shows solid design. Each camera type gets its own file (rule 6), the mode enum is clean, and the config struct is well-documented.
- **Frustum culling integration**: Passing `ICamera*` through rendering functions is the right approach. The culling constants file centralizes radii. The 2D hull-based terrain culling is an interesting optimization.
- **Clean config migration**: GameConfig additions for SoundVolume/MusicVolume and resolution settings are well-structured with proper defaults and constants.
