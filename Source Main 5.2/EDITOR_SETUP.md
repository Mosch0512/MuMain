# MU Editor - Setup Guide

## Overview
The MU Editor is a C++ ImGui-based in-game editor that provides:
- **Top Toolbar**: Start/Stop game buttons with status
- **Center Viewport**: Game renders here
- **Bottom Console**: Split view with Editor Console (left) and Game Console (right)

## Key Features
- ✅ Completely wrapped in `#ifdef _EDITOR` - **excluded from release builds**
- ✅ F12 key to toggle editor on/off
- ✅ `--editor` command line flag to start in editor mode
- ✅ Direct access to game internals (same process!)

## Files Added

### ImGui Library (ThirdParty/imgui/)
Downloaded via git sparse-checkout from https://github.com/ocornut/imgui
- `imgui.h`, `imgui.cpp`
- `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`
- `imgui_internal.h`, `imconfig.h`
- `backends/imgui_impl_win32.h`, `backends/imgui_impl_win32.cpp`
- `backends/imgui_impl_opengl2.h`, `backends/imgui_impl_opengl2.cpp`

**To update ImGui:**
```bash
cd "D:\MuMain\Source Main 5.2\ThirdParty\imgui"
git pull origin master
```

### Editor Files (source/)
- `MuEditor.h` - Editor class declaration
- `MuEditor.cpp` - Editor implementation

## Build Configurations

### Debug Builds (Editor Enabled)
- Preprocessor define: `_EDITOR`
- Include paths: `ThirdParty\imgui` and `ThirdParty\imgui\backends`
- All editor code is compiled in

### Release Builds (Editor Excluded)
- No `_EDITOR` define
- All editor code is **completely excluded** via `#ifdef _EDITOR`
- Zero overhead - as if the editor never existed

## Integration Points in Winmain.cpp

1. **Includes** (lines 55-62):
```cpp
#ifdef _EDITOR
#include "MuEditor.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(...);
#endif
```

2. **WndProc Message Forwarding** (lines 463-467):
```cpp
#ifdef _EDITOR
if (g_MuEditor.IsEnabled() && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
    return true;
#endif
```

3. **Initialization** (lines 1245-1256):
```cpp
#ifdef _EDITOR
g_MuEditor.Initialize(g_hWnd, g_hDC);
if (szCmdLine && wcsstr(GetCommandLineW(), L"--editor"))
    g_MuEditor.SetEnabled(true);
#endif
```

4. **Main Loop** (lines 877-905):
```cpp
#ifdef _EDITOR
// F12 toggle
// Update editor
g_MuEditor.Update();
#endif

RenderScene(g_hDC);

#ifdef _EDITOR
// Render editor overlay
g_MuEditor.Render();
#endif
```

## How to Use

### Starting the Editor
```bash
# Option 1: Command line flag
main.exe --editor

# Option 2: Press F12 at runtime to toggle
```

### Current Features
- Start/Stop buttons (placeholder - doesn't actually start separate process yet)
- Editor console with timestamps
- Game console (placeholder for redirected game output)
- Fullscreen dockable layout

### Next Steps to Implement
Since you have direct access to game code, you can now:

1. **Hook Item Hover**:
```cpp
// In your mouse handling code:
#ifdef _EDITOR
if (g_MuEditor.IsEnabled())
{
    if (mouseOverItem)
    {
        g_MuEditor.ShowItemInspector(ItemAttribute[itemIndex]);
    }
}
#endif
```

2. **Read Game State**:
```cpp
// Direct access to game memory:
g_MuEditor.LogGame("Player HP: " + std::to_string(PlayerHP));
```

3. **Modify Game in Real-Time**:
```cpp
// Change values live:
ItemAttribute[index].Durability = 255;
```

## Building

### Debug (with Editor):
```bash
MSBuild Main.sln /p:Configuration="Global Debug" /p:Platform=x86
```

### Release (no Editor):
```bash
MSBuild Main.sln /p:Configuration="Global Release" /p:Platform=x86
```

The release build will have **zero editor code** - it's completely stripped out by the preprocessor.

## Advantages Over C# Editor
- ✅ Direct memory access to game internals
- ✅ No IPC/marshalling needed
- ✅ Can hook any game event
- ✅ Read/write game structures directly
- ✅ Single codebase
- ✅ Zero overhead in release builds
- ✅ ImGui docking and modern UI

## File Structure
```
D:\MuMain\Source Main 5.2\
├── ThirdParty/
│   └── imgui/                    # ImGui library (git tracked)
│       ├── imgui.h
│       ├── imgui.cpp
│       ├── backends/
│       │   ├── imgui_impl_win32.*
│       │   └── imgui_impl_opengl2.*
│       └── ...
├── source/
│   ├── MuEditor.h                # Editor class
│   ├── MuEditor.cpp              # Editor implementation
│   └── Winmain.cpp               # Integration points
└── Main.vcxproj                  # Project file (updated)
```

## Important Notes
- All editor code is wrapped in `#ifdef _EDITOR`
- Release builds have **NO** editor code at all
- F12 key toggles editor visibility
- Editor has full access to game internals
- ImGui is lightweight and fast (~10 files, no dependencies)
