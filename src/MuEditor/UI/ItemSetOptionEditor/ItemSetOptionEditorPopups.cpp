#include "stdafx.h"

#ifdef _EDITOR

#include "ItemSetOptionEditorPopups.h"
#include "Translation/i18n.h"
#include "imgui.h"

namespace
{
    void RenderSimplePopup(const char* popupId, const char* messageKey)
    {
        if (ImGui::BeginPopupModal(popupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%s", EDITOR_TEXT(messageKey));
            ImGui::Separator();
            if (ImGui::Button(EDITOR_TEXT("btn_ok"), ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

void CItemSetOptionEditorPopups::RenderAll()
{
    RenderSimplePopup("ItemSetOption Save Success", "msg_itemsetopt_save_success");
    RenderSimplePopup("ItemSetOption Save Failed",  "msg_itemsetopt_save_failed");
}

#endif // _EDITOR
