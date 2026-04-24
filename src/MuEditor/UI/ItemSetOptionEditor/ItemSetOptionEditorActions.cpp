#include "stdafx.h"

#ifdef _EDITOR

#include "ItemSetOptionEditorActions.h"
#include "CSItemOption.h"
#include "../MuEditor/UI/Console/MuEditorConsoleUI.h"
#include "Translation/i18n.h"
#include "imgui.h"
#include <string>

extern std::wstring g_strSelectedML;

void CItemSetOptionEditorActions::RenderSaveButton()
{
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));

    if (ImGui::Button(EDITOR_TEXT("btn_save")))
    {
        wchar_t fileName[256];
        swprintf_s(fileName, _countof(fileName),
                   L"Data\\Local\\%ls\\ItemSetOption_%ls.bmd",
                   g_strSelectedML.c_str(), g_strSelectedML.c_str());

        if (g_csItemOption.SaveItemSetOption(fileName))
        {
            g_MuEditorConsoleUI.LogEditor("=== ItemSetOption SAVE COMPLETED ===");
            ImGui::OpenPopup("ItemSetOption Save Success");
        }
        else
        {
            g_MuEditorConsoleUI.LogEditor(EDITOR_TEXT("msg_itemsetopt_save_failed"));
            ImGui::OpenPopup("ItemSetOption Save Failed");
        }
    }

    ImGui::PopStyleColor(2);
}

void CItemSetOptionEditorActions::RenderAllButtons()
{
    RenderSaveButton();
}

#endif // _EDITOR
