#pragma once

#ifdef _EDITOR

#include "stdafx.h"

class CMuEditor
{
public:
    static CMuEditor& GetInstance();

    void Initialize(HWND hwnd, HDC hdc);
    void Shutdown();
    void Update();
    void Render();
    void RenderBeforeGame();  // Render ImGui UI, prepare for game rendering
    void RenderAfterGame();   // Finish ImGui rendering after game

    bool IsEnabled() const { return m_bEditorMode; }
    bool IsEditorMode() const { return m_bEditorMode; }  // Alias for clarity
    void SetEnabled(bool enabled) { m_bEditorMode = enabled; }
    void ToggleEditor() { m_bEditorMode = !m_bEditorMode; }

    bool IsShowingItemEditor() const { return m_bShowItemEditor; }
    bool IsHoveringUI() const { return m_bHoveringUI; }
    void SetHoveringUI(bool hovering) { m_bHoveringUI = hovering; }

private:
    CMuEditor();
    ~CMuEditor();

    bool m_bEditorMode;
    bool m_bInitialized;
    bool m_bFrameStarted;
    bool m_bShowItemEditor;
    bool m_bHoveringUI;
};

// Global accessor
#define g_MuEditor CMuEditor::GetInstance()

#endif // _EDITOR
