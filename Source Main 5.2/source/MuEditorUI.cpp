#include "stdafx.h"

#ifdef _EDITOR

#include "MuEditorUI.h"
#include "MuEditor.h"
#include "MuEditorConsole.h"
#include "ZzzOpenglUtil.h"
#include "Input.h"
#include "imgui.h"

// Define framebuffer extension function pointers manually (avoiding GLEW dependency)
typedef void (APIENTRY * PFNGLGENFRAMEBUFFERSEXTPROC)(GLsizei n, GLuint* framebuffers);
typedef void (APIENTRY * PFNGLBINDFRAMEBUFFEREXTPROC)(GLenum target, GLuint framebuffer);
typedef void (APIENTRY * PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY * PFNGLGENRENDERBUFFERSEXTPROC)(GLsizei n, GLuint* renderbuffers);
typedef void (APIENTRY * PFNGLBINDRENDERBUFFEREXTPROC)(GLenum target, GLuint renderbuffer);
typedef void (APIENTRY * PFNGLRENDERBUFFERSTORAGEEXTPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY * PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef GLenum (APIENTRY * PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)(GLenum target);
typedef void (APIENTRY * PFNGLDELETEFRAMEBUFFERSEXTPROC)(GLsizei n, const GLuint* framebuffers);
typedef void (APIENTRY * PFNGLDELETERENDERBUFFERSEXTPROC)(GLsizei n, const GLuint* renderbuffers);

static PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT_func = nullptr;
static PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT_func = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT_func = nullptr;
static PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT_func = nullptr;
static PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT_func = nullptr;
static PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT_func = nullptr;
static PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT_func = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT_func = nullptr;
static PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT_func = nullptr;
static PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT_func = nullptr;

static bool g_FramebufferExtensionLoaded = false;

static void LoadFramebufferExtension()
{
    if (g_FramebufferExtensionLoaded)
        return;

    g_MuEditorConsole.Log("Loading framebuffer extension functions...", ConsoleCategory::Editor);

    glGenFramebuffersEXT_func = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
    glBindFramebufferEXT_func = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
    glFramebufferTexture2DEXT_func = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
    glGenRenderbuffersEXT_func = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
    glBindRenderbufferEXT_func = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
    glRenderbufferStorageEXT_func = (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT");
    glFramebufferRenderbufferEXT_func = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");
    glCheckFramebufferStatusEXT_func = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
    glDeleteFramebuffersEXT_func = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");
    glDeleteRenderbuffersEXT_func = (PFNGLDELETERENDERBUFFERSEXTPROC)wglGetProcAddress("glDeleteRenderbuffersEXT");

    g_FramebufferExtensionLoaded = (glGenFramebuffersEXT_func != nullptr &&
                                     glBindFramebufferEXT_func != nullptr &&
                                     glFramebufferTexture2DEXT_func != nullptr);

    if (g_FramebufferExtensionLoaded)
    {
        g_MuEditorConsole.Log("Framebuffer extension functions loaded successfully", ConsoleCategory::Editor);
    }
    else
    {
        g_MuEditorConsole.Log("ERROR: Failed to load framebuffer extension functions!", ConsoleCategory::Error);
        char msg[256];
        sprintf_s(msg, "glGenFramebuffersEXT: %p, glBindFramebufferEXT: %p, glFramebufferTexture2DEXT: %p",
                  glGenFramebuffersEXT_func, glBindFramebufferEXT_func, glFramebufferTexture2DEXT_func);
        g_MuEditorConsole.Log(msg, ConsoleCategory::Error);
    }
}

// GL constants (since we're not using GLEW headers fully)
#ifndef GL_FRAMEBUFFER_EXT
#define GL_FRAMEBUFFER_EXT 0x8D40
#define GL_RENDERBUFFER_EXT 0x8D41
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#define GL_DEPTH_ATTACHMENT_EXT 0x8D00
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
#define GL_DEPTH_COMPONENT24 0x81A6
#endif

// UI Layout constants
constexpr float TOOLBAR_HEIGHT = 40.0f;
constexpr float TOOLBAR_PADDING = 8.0f;
constexpr float TOOLBAR_INDENT = 10.0f;
constexpr float BUTTON_WIDTH_OFFSET = 110.0f;
constexpr int MOUSE_BUTTON_LEFT = 0;

CMuEditorUI& CMuEditorUI::GetInstance()
{
    static CMuEditorUI instance;
    return instance;
}

void CMuEditorUI::RenderToolbar(bool& editorEnabled, bool& showItemEditor)
{
    // Always render full toolbar (no open/close functionality)
    RenderToolbarFull(editorEnabled, showItemEditor, showConsole);
}

void CMuEditorUI::RenderToolbarOpen(bool& editorEnabled)
{
    // When editor is disabled, show only "Open Editor" button
    // Use NoInputs flag to allow game mouse to pass through, but handle button clicks manually
    ImGuiIO& io = ImGui::GetIO();

    // Prevent ImGui from wanting mouse input when editor is closed
    io.WantCaptureMouse = false;
    io.WantCaptureKeyboard = false;

    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, TOOLBAR_HEIGHT), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

    // NoInputs prevents ImGui from capturing mouse/keyboard, allowing game cursor to work
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs;

    // Use same window background and border as Toolbar but fully transparent
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(TOOLBAR_PADDING, TOOLBAR_PADDING));

    if (ImGui::Begin("ToolbarClosed", nullptr, flags))
    {
        ImGui::Spacing();
        ImGui::Indent(TOOLBAR_INDENT);

        // Open button on the far right (same position as Close button)
        ImGui::SameLine(ImGui::GetWindowWidth() - BUTTON_WIDTH_OFFSET);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));

        // Draw the button (won't be interactive due to NoInputs flag)
        ImGui::Button("Open Editor");

        // Get button rect for manual click detection
        ImVec2 buttonMin = ImGui::GetItemRectMin();
        ImVec2 buttonMax = ImGui::GetItemRectMax();

        ImGui::PopStyleColor(2);

        // Check if mouse is hovering over the button specifically
        bool isHoveringButton = (io.MousePos.x >= buttonMin.x && io.MousePos.x <= buttonMax.x &&
                                 io.MousePos.y >= buttonMin.y && io.MousePos.y <= buttonMax.y);

        if (isHoveringButton)
        {
            g_MuEditor.SetHoveringUI(true);

            if (ImGui::IsMouseClicked(MOUSE_BUTTON_LEFT))
            {
                editorEnabled = true;

                // Clear game mouse button states to prevent click from going through to game
                extern bool MouseLButton, MouseLButtonPop, MouseLButtonPush, MouseLButtonDBClick;
                MouseLButton = false;
                MouseLButtonPop = false;
                MouseLButtonPush = false;
                MouseLButtonDBClick = false;
            }
        }

        ImGui::Unindent(10.0f);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void CMuEditorUI::RenderToolbarFull(bool& editorEnabled, bool& showItemEditor)
{
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, TOOLBAR_HEIGHT), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 0.95f));

    if (ImGui::Begin("Toolbar", nullptr, flags))
    {
        // Check if hovering this window
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        {
            g_MuEditor.SetHoveringUI(true);
        }

        ImGui::Spacing();
        ImGui::Indent(10.0f);

        ImGui::Text("MU Editor");
        ImGui::SameLine();

        if (ImGui::Button("Item Editor"))
        {
            showItemEditor = !showItemEditor;
        }

        // No close button - editor is always on

        ImGui::Unindent(10.0f);
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

void CMuEditorUI::RenderCenterViewport()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    // Render left panel
    ImGui::SetNextWindowPos(ImVec2(0, EditorLayout::TOP_TOOLBAR_HEIGHT), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(EditorLayout::SIDE_PANEL_WIDTH,
                                     EditorLayout::GetCenterPanelHeight(io.DisplaySize.y)),
                             ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.12f, 0.95f));
    if (ImGui::Begin("LeftPanel", nullptr, flags))
    {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        {
            g_MuEditor.SetHoveringUI(true);
        }
        ImGui::Text("Left Panel");
        ImGui::Separator();
        // Add left panel content here (e.g., scene hierarchy, object list)
    }
    ImGui::End();
    ImGui::PopStyleColor();

    // Render center game viewport window (scrollable)
    RenderGameViewportWindow();

    // Render right panel
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - EditorLayout::SIDE_PANEL_WIDTH,
                                    EditorLayout::TOP_TOOLBAR_HEIGHT),
                            ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(EditorLayout::SIDE_PANEL_WIDTH,
                                     EditorLayout::GetCenterPanelHeight(io.DisplaySize.y)),
                             ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.12f, 0.95f));
    if (ImGui::Begin("RightPanel", nullptr, flags))
    {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        {
            g_MuEditor.SetHoveringUI(true);
        }
        ImGui::Text("Right Panel");
        ImGui::Separator();
        // Add right panel content here (e.g., properties, inspector)
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

void CMuEditorUI::RenderGameViewportWindow()
{
    ImGuiIO& io = ImGui::GetIO();

    // Position center window between left and right panels
    ImVec2 centerPos = ImVec2(EditorLayout::GetCenterPanelX(), EditorLayout::GetCenterPanelY());
    ImVec2 centerSize = ImVec2(EditorLayout::GetCenterPanelWidth(io.DisplaySize.x),
                                EditorLayout::GetCenterPanelHeight(io.DisplaySize.y));

    ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(centerSize, ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 0.95f));
    if (ImGui::Begin("GameViewport", nullptr, flags))
    {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        {
            g_MuEditor.SetHoveringUI(true);
        }

        // Zoom controls
        ImGui::Text("Game View");
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 200);
        if (ImGui::Button("-"))
        {
            ZoomOut();
        }
        ImGui::SameLine();
        ImGui::Text("%.0f%%", m_fZoomLevel * 100.0f);
        ImGui::SameLine();
        if (ImGui::Button("+"))
        {
            ZoomIn();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            ResetZoom();
        }

        ImGui::Separator();

        // Scrollable child window for game viewport
        ImGui::BeginChild("GameViewportScroll", ImVec2(0, 0), false,
                          ImGuiWindowFlags_HorizontalScrollbar);

        // Calculate zoomed game size
        float zoomedWidth = EditorLayout::GAME_NATIVE_WIDTH * m_fZoomLevel;
        float zoomedHeight = EditorLayout::GAME_NATIVE_HEIGHT * m_fZoomLevel;

        // Get the cursor position before drawing (this is where game texture will be displayed)
        ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

        // Store the viewport position
        m_gameViewportScreenX = cursorScreenPos.x;
        m_gameViewportScreenY = cursorScreenPos.y;
        m_gameViewportWidth = zoomedWidth;
        m_gameViewportHeight = zoomedHeight;

        // Display the game texture using ImGui::Image
        if (m_framebufferInitialized && m_gameTexture)
        {
            // ImGui expects texture coordinates from bottom-left, OpenGL has top-left origin
            // So we flip the UV coordinates vertically
            ImGui::Image((void*)(intptr_t)m_gameTexture,
                        ImVec2(zoomedWidth, zoomedHeight),
                        ImVec2(0, 1),  // UV top-left (flipped)
                        ImVec2(1, 0)); // UV bottom-right (flipped)

            // Check if mouse is over the game viewport area
            if (ImGui::IsItemHovered())
            {
                g_MuEditor.SetHoveringUI(false);  // Allow game interaction

                // Tell ImGui not to capture mouse - allow game to receive mouse input
                ImGuiIO& io = ImGui::GetIO();
                io.WantCaptureMouse = false;
            }
        }
        else
        {
            // Fallback: show a placeholder if framebuffer not ready
            ImGui::Dummy(ImVec2(zoomedWidth, zoomedHeight));
            ImVec2 textPos = ImGui::GetItemRectMin();

            // Show detailed error info
            char debugMsg[256];
            sprintf_s(debugMsg, "Framebuffer init: %s, Texture ID: %u\nExtension loaded: %s",
                     m_framebufferInitialized ? "YES" : "NO",
                     m_gameTexture,
                     g_FramebufferExtensionLoaded ? "YES" : "NO");

            ImGui::GetWindowDrawList()->AddText(
                ImVec2(textPos.x + 10, textPos.y + zoomedHeight / 2 - 20),
                ImGui::GetColorU32(ImVec4(1, 0.5f, 0.5f, 1)),
                debugMsg);
        }

        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

void CMuEditorUI::BeginGameViewport()
{
    // Initialize framebuffer if not done yet
    if (!m_framebufferInitialized)
    {
        InitializeFramebuffer();
    }

    // Only proceed if framebuffer is available
    if (m_framebufferInitialized && glBindFramebufferEXT_func)
    {
        // Simply bind framebuffer - don't override any dimensions
        // This lets the game render normally, just to a texture instead of screen
        glBindFramebufferEXT_func(GL_FRAMEBUFFER_EXT, m_framebuffer);
        glViewport(0, 0, EditorLayout::GAME_NATIVE_WIDTH, EditorLayout::GAME_NATIVE_HEIGHT);
    }
}

void CMuEditorUI::EndGameViewport()
{
    glDisable(GL_SCISSOR_TEST);

    // Unbind framebuffer - render back to screen
    if (glBindFramebufferEXT_func)
    {
        glBindFramebufferEXT_func(GL_FRAMEBUFFER_EXT, 0);
    }

    // Restore viewport for ImGui rendering
    extern unsigned int WindowWidth;
    extern unsigned int WindowHeight;
    glViewport(0, 0, WindowWidth, WindowHeight);
}

void CMuEditorUI::InitializeFramebuffer()
{
    if (m_framebufferInitialized)
        return;

    g_MuEditorConsole.Log("Initializing framebuffer...", ConsoleCategory::Editor);

    // Load framebuffer extension functions
    LoadFramebufferExtension();

    // Check if the framebuffer extension is available
    if (!g_FramebufferExtensionLoaded)
    {
        g_MuEditorConsole.Log("Framebuffer objects not supported! Using fallback rendering.", ConsoleCategory::Error);
        m_framebufferInitialized = false;
        return;
    }

    // Generate framebuffer
    glGenFramebuffersEXT_func(1, &m_framebuffer);
    g_MuEditorConsole.Log("Framebuffer generated", ConsoleCategory::Editor);

    glBindFramebufferEXT_func(GL_FRAMEBUFFER_EXT, m_framebuffer);

    // Create texture for color attachment
    glGenTextures(1, &m_gameTexture);
    glBindTexture(GL_TEXTURE_2D, m_gameTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, EditorLayout::GAME_NATIVE_WIDTH,
                 EditorLayout::GAME_NATIVE_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2DEXT_func(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_2D, m_gameTexture, 0);

    char msg[128];
    sprintf_s(msg, "Texture created: %u (%dx%d)", m_gameTexture,
              EditorLayout::GAME_NATIVE_WIDTH, EditorLayout::GAME_NATIVE_HEIGHT);
    g_MuEditorConsole.Log(msg, ConsoleCategory::Editor);

    // Create renderbuffer for depth and stencil
    glGenRenderbuffersEXT_func(1, &m_depthRenderbuffer);
    glBindRenderbufferEXT_func(GL_RENDERBUFFER_EXT, m_depthRenderbuffer);
    glRenderbufferStorageEXT_func(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24,
                             EditorLayout::GAME_NATIVE_WIDTH, EditorLayout::GAME_NATIVE_HEIGHT);
    glFramebufferRenderbufferEXT_func(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, m_depthRenderbuffer);

    // Check framebuffer status
    GLenum status = glCheckFramebufferStatusEXT_func(GL_FRAMEBUFFER_EXT);
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        sprintf_s(msg, "Framebuffer incomplete! Status: 0x%X", status);
        g_MuEditorConsole.Log(msg, ConsoleCategory::Error);
        m_framebufferInitialized = false;
    }
    else
    {
        m_framebufferInitialized = true;
        g_MuEditorConsole.Log("Game viewport framebuffer initialized successfully!", ConsoleCategory::Editor);
    }

    // Unbind framebuffer
    glBindFramebufferEXT_func(GL_FRAMEBUFFER_EXT, 0);
}

void CMuEditorUI::CleanupFramebuffer()
{
    if (!m_framebufferInitialized)
        return;

    if (m_gameTexture)
    {
        glDeleteTextures(1, &m_gameTexture);
        m_gameTexture = 0;
    }

    if (m_depthRenderbuffer && glDeleteRenderbuffersEXT_func)
    {
        glDeleteRenderbuffersEXT_func(1, &m_depthRenderbuffer);
        m_depthRenderbuffer = 0;
    }

    if (m_framebuffer && glDeleteFramebuffersEXT_func)
    {
        glDeleteFramebuffersEXT_func(1, &m_framebuffer);
        m_framebuffer = 0;
    }

    m_framebufferInitialized = false;
}

void CMuEditorUI::ResizeFramebuffer(int width, int height)
{
    if (!m_framebufferInitialized || !glBindRenderbufferEXT_func)
        return;

    // Resize texture
    glBindTexture(GL_TEXTURE_2D, m_gameTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    // Resize depth renderbuffer
    glBindRenderbufferEXT_func(GL_RENDERBUFFER_EXT, m_depthRenderbuffer);
    glRenderbufferStorageEXT_func(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, width, height);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbufferEXT_func(GL_RENDERBUFFER_EXT, 0);
}

#endif // _EDITOR
