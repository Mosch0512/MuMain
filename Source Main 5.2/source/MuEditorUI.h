#pragma once

#ifdef _EDITOR

// Layout constants for bordered editor layout
struct EditorLayout
{
    static constexpr float SIDE_PANEL_WIDTH = 300.0f;  // Width of side panels
    static constexpr float TOP_TOOLBAR_HEIGHT = 40.0f;  // Height of top toolbar
    static constexpr float BOTTOM_PANEL_HEIGHT = 250.0f; // Height of bottom console panel

    // Game viewport native size (the actual game rendering resolution)
    // Match the window size so UI elements work correctly without scaling
    static constexpr int GAME_NATIVE_WIDTH = 1024;
    static constexpr int GAME_NATIVE_HEIGHT = 768;

    // Calculate center panel area (where scrollable game viewport goes)
    static float GetCenterPanelX() { return SIDE_PANEL_WIDTH; }
    static float GetCenterPanelY() { return TOP_TOOLBAR_HEIGHT; }
    static float GetCenterPanelWidth(float displayWidth) { return displayWidth - (SIDE_PANEL_WIDTH * 2); }
    static float GetCenterPanelHeight(float displayHeight) { return displayHeight - TOP_TOOLBAR_HEIGHT - BOTTOM_PANEL_HEIGHT; }
};

class CMuEditorUI
{
public:
    static CMuEditorUI& GetInstance();

    void RenderToolbar(bool& editorEnabled, bool& showItemEditor);
    void RenderCenterViewport();
    void RenderBottomPanel(bool& showItemEditor);  // Render bottom panel with tabs
    void BeginGameViewport();  // Set up viewport for game rendering
    void EndGameViewport();    // Restore viewport after game rendering
    void InitializeFramebuffer();  // Initialize framebuffer for game rendering

    float GetZoomLevel() const { return m_fZoomLevel; }
    void SetZoomLevel(float zoom) { m_fZoomLevel = zoom; }
    void ZoomIn() { m_fZoomLevel += 0.1f; if (m_fZoomLevel > 3.0f) m_fZoomLevel = 3.0f; }
    void ZoomOut() { m_fZoomLevel -= 0.1f; if (m_fZoomLevel < 0.25f) m_fZoomLevel = 0.25f; }
    void ResetZoom() { m_fZoomLevel = 1.0f; }

    // Get the screen position where game should be rendered
    void GetGameViewportScreenPos(float& outX, float& outY, float& outWidth, float& outHeight) const
    {
        outX = m_gameViewportScreenX;
        outY = m_gameViewportScreenY;
        outWidth = m_gameViewportWidth;
        outHeight = m_gameViewportHeight;
    }

private:
    CMuEditorUI() : m_fZoomLevel(1.0f), m_gameViewportScreenX(0), m_gameViewportScreenY(0),
                    m_gameViewportWidth(0), m_gameViewportHeight(0),
                    m_framebuffer(0), m_gameTexture(0), m_depthRenderbuffer(0),
                    m_framebufferInitialized(false), m_savedWindowWidth(0), m_savedWindowHeight(0),
                    m_savedInputWidth(0), m_savedInputHeight(0), m_savedMouseX(0), m_savedMouseY(0),
                    m_savedScreenRateX(0.0f), m_savedScreenRateY(0.0f), m_fBottomPanelHeight(250.0f) {}
    ~CMuEditorUI() { CleanupFramebuffer(); }

    void RenderToolbarOpen(bool& editorEnabled);
    void RenderGameViewportWindow();  // Render center scrollable game viewport
    void RenderEditorConsole();       // Render editor console in tab
    void RenderGameConsole();         // Render game console in tab

    float m_fZoomLevel;  // Zoom level for game viewport (0.25 to 3.0)
    float m_fBottomPanelHeight;  // Resizable height of bottom panel

    // Cached viewport position for game rendering
    float m_gameViewportScreenX;
    float m_gameViewportScreenY;
    float m_gameViewportWidth;
    float m_gameViewportHeight;

    // Framebuffer for rendering game to texture
    unsigned int m_framebuffer;
    unsigned int m_gameTexture;
    unsigned int m_depthRenderbuffer;
    bool m_framebufferInitialized;

    // Saved window dimensions (to restore after framebuffer rendering)
    unsigned int m_savedWindowWidth;
    unsigned int m_savedWindowHeight;
    long m_savedInputWidth;
    long m_savedInputHeight;
    int m_savedMouseX;
    int m_savedMouseY;
    float m_savedScreenRateX;
    float m_savedScreenRateY;

    void CleanupFramebuffer();
    void ResizeFramebuffer(int width, int height);
    void RenderToolbarFull(bool& editorEnabled, bool& showItemEditor);
};

#define g_MuEditorUI CMuEditorUI::GetInstance()

#endif // _EDITOR
