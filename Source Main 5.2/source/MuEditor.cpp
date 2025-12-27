#include "stdafx.h"

#ifdef _EDITOR

#include "MuEditor.h"
#include "MuEditorUI.h"
#include "MuEditorConsole.h"
#include "MuItemEditor.h"
#include "MuInputBlocker.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl2.h"

CMuEditor::CMuEditor()
    : m_bEditorMode(false)
    , m_bInitialized(false)
    , m_bFrameStarted(false)
    , m_bShowItemEditor(false)
    , m_bHoveringUI(false)
{
}

CMuEditor::~CMuEditor()
{
    Shutdown();
}

CMuEditor& CMuEditor::GetInstance()
{
    static CMuEditor instance;
    return instance;
}

void CMuEditor::Initialize(HWND hwnd, HDC hdc)
{
    if (m_bInitialized)
        return;

    std::fwprintf(stderr, L"[MuEditor] Initialize() called\n");
    std::fflush(stderr);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Let ImGui manage the mouse cursor (Windows cursor) when editor is open
    // Game cursor is disabled when editor is open (see ZzzScene.cpp)
    io.MouseDrawCursor = false; // Don't let ImGui draw its own cursor, use Windows cursor

    std::fwprintf(stderr, L"[MuEditor] ImGui context created\n");
    std::fflush(stderr);

    // Dark theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplOpenGL2_Init();

    std::fwprintf(stderr, L"[MuEditor] ImGui backends initialized\n");
    std::fflush(stderr);

    m_bInitialized = true;
    g_MuEditorConsole.LogEditor("MU Editor initialized");

    std::fwprintf(stderr, L"[MuEditor] Initialize() completed\n");
    std::fflush(stderr);
}

void CMuEditor::Shutdown()
{
    if (!m_bInitialized)
        return;

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_bInitialized = false;
}

void CMuEditor::Update()
{
    if (!m_bInitialized)
        return;

    // Only start a new frame if we haven't already
    if (!m_bFrameStarted)
    {
        ImGui_ImplOpenGL2_NewFrame();

        // Only let Win32 backend update mouse when editor is open
        if (m_bEditorMode)
        {
            ImGui_ImplWin32_NewFrame();
        }
        else
        {
            // When closed, manually create a minimal frame and update mouse position
            ImGuiIO& io = ImGui::GetIO();

            // Get window size
            extern HWND g_hWnd;
            RECT rect;
            GetClientRect(g_hWnd, &rect);
            io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
            io.DeltaTime = 1.0f / 60.0f;

            // Manually update mouse position for button detection
            POINT mousePos;
            if (GetCursorPos(&mousePos))
            {
                ScreenToClient(g_hWnd, &mousePos);
                io.MousePos = ImVec2((float)mousePos.x, (float)mousePos.y);
            }

            // Update mouse button states
            extern bool MouseLButton;
            io.MouseDown[0] = MouseLButton;
        }

        ImGui::NewFrame();
        m_bFrameStarted = true;
    }

    // When editor is closed, check if mouse is over "Open Editor" button area and block input
    if (!m_bEditorMode)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.WantCaptureMouse = false;
        io.WantCaptureKeyboard = false;

        // Check if mouse is over button area (top-right corner)
        float buttonX = io.DisplaySize.x - 110;
        float buttonY = 8.0f;
        float buttonWidth = 100.0f;
        float buttonHeight = 24.0f;

        if (io.MousePos.x >= buttonX && io.MousePos.x <= (buttonX + buttonWidth) &&
            io.MousePos.y >= buttonY && io.MousePos.y <= (buttonY + buttonHeight))
        {
            // Mouse is over button - block game input
            extern bool MouseLButton, MouseLButtonPop, MouseLButtonPush, MouseLButtonDBClick;
            MouseLButton = false;
            MouseLButtonPop = false;
            MouseLButtonPush = false;
            MouseLButtonDBClick = false;
        }
    }
    // Only block game input when editor is fully open and ImGui is capturing mouse/keyboard
    else
    {
        g_MuInputBlocker.ProcessInputBlocking();
    }
}

void CMuEditor::Render()
{
    if (!m_bInitialized)
        return;

    // Only render if we have a frame started
    if (!m_bFrameStarted)
        return;

    // Reset hover state at start of frame
    m_bHoveringUI = false;

    // Render toolbar (handles both open and closed states)
    g_MuEditorUI.RenderToolbar(m_bEditorMode, m_bShowItemEditor);

    if (m_bEditorMode)
    {
        // Render center viewport
        g_MuEditorUI.RenderCenterViewport();

        // Render console
        g_MuEditorConsole.Render();

        // Render editor windows
        if (m_bShowItemEditor)
        {
            g_MuItemEditor.Render(m_bShowItemEditor);
        }
    }

    // Smart cursor management based on ImGui hover state
    // m_bHoveringUI is set by each UI component that detects hover
    static bool lastHoveringState = false;
    static int frameCounter = 0;

    // Control game cursor rendering via global flag
    extern bool g_bRenderGameCursor;
    g_bRenderGameCursor = !m_bHoveringUI;

    if (m_bHoveringUI != lastHoveringState)
    {
        // State changed, update cursor
        if (m_bHoveringUI)
        {
            // Show Windows cursor - make it VERY visible
            int cursorCount = ShowCursor(TRUE);
            while (cursorCount < 10) // Force it to be very positive
            {
                cursorCount = ShowCursor(TRUE);
            }

            // Debug log
            std::string msg = "Frame " + std::to_string(frameCounter) + ": Hovering UI - Windows cursor ON (count=" + std::to_string(cursorCount) + "), Game cursor OFF";
            g_MuEditorConsole.LogEditor(msg);
        }
        else
        {
            // Hide Windows cursor (game cursor will show)
            int cursorCount = ShowCursor(FALSE);
            while (cursorCount >= 0)
            {
                cursorCount = ShowCursor(FALSE);
            }

            // Debug log
            std::string msg = "Frame " + std::to_string(frameCounter) + ": NOT hovering UI - Windows cursor OFF (count=" + std::to_string(cursorCount) + "), Game cursor ON";
            g_MuEditorConsole.LogEditor(msg);
        }
        lastHoveringState = m_bHoveringUI;
    }
    frameCounter++;

    // Render ImGui and reset frame state
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    // Frame is complete, reset for next frame
    m_bFrameStarted = false;
}

#endif // _EDITOR
