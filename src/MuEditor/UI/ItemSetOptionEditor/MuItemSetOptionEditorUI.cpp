#include "stdafx.h"

#ifdef _EDITOR

#include "MuItemSetOptionEditorUI.h"
#include "ItemSetOptionEditorTable.h"
#include "ItemSetOptionEditorActions.h"
#include "ItemSetOptionEditorPopups.h"
#include "../MuEditor/Core/MuEditorCore.h"
#include "CSItemOption.h"
#include "Translation/i18n.h"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cstring>

CMuItemSetOptionEditorUI::CMuItemSetOptionEditorUI()
    : m_selectedRow(-1)
    , m_bShowEmpty(false)
    , m_pTable(nullptr)
{
    std::memset(m_szSearchBuffer, 0, sizeof(m_szSearchBuffer));
    m_pTable = new CItemSetOptionEditorTable();
}

CMuItemSetOptionEditorUI::~CMuItemSetOptionEditorUI()
{
    delete m_pTable;
    m_pTable = nullptr;
}

CMuItemSetOptionEditorUI& CMuItemSetOptionEditorUI::GetInstance()
{
    static CMuItemSetOptionEditorUI instance;
    return instance;
}

void CMuItemSetOptionEditorUI::Render(bool& showEditor)
{
    constexpr float TOOLBAR_HEIGHT = 40.0f;
    constexpr float CONSOLE_HEIGHT = 200.0f;

    ImGuiIO& io = ImGui::GetIO();

    const float availableTop    = TOOLBAR_HEIGHT;
    const float availableBottom = io.DisplaySize.y - CONSOLE_HEIGHT;
    const float availableHeight = availableBottom - availableTop;

    ImGui::SetNextWindowSizeConstraints(
        ImVec2(500, 300),
        ImVec2(io.DisplaySize.x, availableHeight)
    );

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("ItemSet Editor", &showEditor, flags))
    {
        ImVec2 windowPos  = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();

        if (windowPos.y < availableTop)
            ImGui::SetWindowPos(ImVec2(windowPos.x, availableTop));
        if (windowPos.y + windowSize.y > availableBottom)
            ImGui::SetWindowPos(ImVec2(windowPos.x, availableBottom - windowSize.y));
        if (windowPos.x < 0)
            ImGui::SetWindowPos(ImVec2(0, windowPos.y));
        if (windowPos.x + windowSize.x > io.DisplaySize.x)
            ImGui::SetWindowPos(ImVec2(io.DisplaySize.x - windowSize.x, windowPos.y));

        const bool isHovering =
            ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup) ||
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ||
            ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId);

        if (isHovering)
        {
            g_MuEditorCore.SetHoveringUI(true);
        }

        CItemSetOptionEditorActions::RenderAllButtons();
        ImGui::Separator();

        // Diagnostic: how many of the 64 slots have non-empty names.
        // Useful for verifying the .bmd loaded correctly.
        {
            const ITEM_SET_OPTION* sets = g_csItemOption.GetItemSetOptions();
            int populated = 0;
            for (int i = 0; i < MAX_SET_OPTION; ++i)
            {
                if (sets[i].strSetName[0] != L'\0') ++populated;
            }

            char firstName[MAX_ITEM_SET_NAME * 4]  = {};
            char secondName[MAX_ITEM_SET_NAME * 4] = {};
            int  firstIdx = -1, secondIdx = -1;
            for (int i = 0; i < MAX_SET_OPTION; ++i)
            {
                if (sets[i].strSetName[0] == L'\0') continue;
                if (firstIdx < 0)
                {
                    firstIdx = i;
                    WideCharToMultiByte(CP_UTF8, 0, sets[i].strSetName, -1,
                                        firstName, sizeof(firstName), nullptr, nullptr);
                }
                else if (secondIdx < 0)
                {
                    secondIdx = i;
                    WideCharToMultiByte(CP_UTF8, 0, sets[i].strSetName, -1,
                                        secondName, sizeof(secondName), nullptr, nullptr);
                    break;
                }
            }

            ImGui::Text("Loaded: %d / %d sets with non-empty names", populated, MAX_SET_OPTION);
            if (firstIdx  >= 0) ImGui::Text("  [%d] = \"%s\"", firstIdx,  firstName);
            if (secondIdx >= 0) ImGui::Text("  [%d] = \"%s\"", secondIdx, secondName);
        }

        RenderSearchBar();
        ImGui::SameLine();
        ImGui::Checkbox("Show empty slots", &m_bShowEmpty);
        ImGui::Separator();

        std::string searchLower = m_szSearchBuffer;
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (m_pTable)
        {
            m_pTable->Render(searchLower, m_selectedRow, m_bShowEmpty);
        }

        CItemSetOptionEditorPopups::RenderAll();
    }
    ImGui::End();
}

void CMuItemSetOptionEditorUI::RenderSearchBar()
{
    ImGui::Text("%s", EDITOR_TEXT("label_search_text"));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(300);
    ImGui::InputText("##ItemSetOptionSearch",
                     m_szSearchBuffer, sizeof(m_szSearchBuffer));
}

#endif // _EDITOR
