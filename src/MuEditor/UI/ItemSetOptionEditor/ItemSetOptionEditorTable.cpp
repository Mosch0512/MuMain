#include "stdafx.h"

#ifdef _EDITOR

#include "ItemSetOptionEditorTable.h"
#include "CSItemOption.h"
#include "Translation/i18n.h"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <vector>

namespace
{
    // Extract a lowercase UTF-8 version of the set name for search comparison.
    std::string ToLowerUtf8(const wchar_t* wide)
    {
        char utf8[MAX_ITEM_SET_NAME * 4] = {};
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, sizeof(utf8), nullptr, nullptr);
        std::string s(utf8);
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    bool MatchesFilter(const std::string& filterLower, const wchar_t* setName)
    {
        if (filterLower.empty()) return true;
        const auto lowered = ToLowerUtf8(setName);
        return lowered.find(filterLower) != std::string::npos;
    }

    // RenderRow pushes the row id, so widgets only need a unique id within
    // the row. widgetOffset is that per-row slot.
    bool RenderByteEdit(std::uint8_t& value, int widgetOffset,
                        const char* label)
    {
        bool changed = false;
        ImGui::PushID(widgetOffset);
        int tmp = static_cast<int>(value);
        ImGui::SetNextItemWidth(70.0f);
        if (ImGui::InputInt(label, &tmp, 0, 0))
        {
            if (tmp < 0)   tmp = 0;
            if (tmp > 255) tmp = 255;
            if (tmp != static_cast<int>(value))
            {
                value = static_cast<std::uint8_t>(tmp);
                changed = true;
            }
        }
        ImGui::PopID();
        return changed;
    }

    // Format an (option, value) pair to a localized label like
    // "Double damage rate + 10%". Returns empty for 0xFF / empty slots.
    std::string ExplainOptionUtf8(std::uint8_t option, int value)
    {
        if (option == 0xFF) return {};
        wchar_t wide[256] = {};
        if (!CSItemOption::getExplainText(wide, option, value))
            return {};
        char utf8[512] = {};
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, sizeof(utf8), nullptr, nullptr);
        return std::string(utf8);
    }

    // Fetches the localized template for an option id and produces a UTF-8
    // label with "N" in place of the value. Does NOT call mu_swprintf/swprintf,
    // so format strings that expect multiple args (e.g. mastery options with
    // %ls in some locales) can't crash the editor.
    std::string OptionLabelWithPlaceholder(std::uint8_t option)
    {
        const int textIndex = CSItemOption::getOptionTemplateTextIndex(option);
        if (textIndex < 0) return {};

        const wchar_t* tmpl = GlobalText[textIndex];
        if (!tmpl) return {};

        std::wstring ws(tmpl);

        // Substitute printf-style specifiers textually:
        //   "%d" (or "%u", "%i") -> "N"
        //   "%%"                  -> "%"   (literal percent, must come after %d)
        //   "%ls", "%s", "%S"     -> "" (drop placeholders we can't fill)
        const std::pair<const wchar_t*, const wchar_t*> subs[] = {
            { L"%d",  L"N" },
            { L"%u",  L"N" },
            { L"%i",  L"N" },
            { L"%ls", L""  },
            { L"%s",  L""  },
            { L"%S",  L""  },
            { L"%%",  L"%" },
        };
        for (const auto& s : subs)
        {
            const std::wstring from(s.first);
            const std::wstring to(s.second);
            size_t pos = 0;
            while ((pos = ws.find(from, pos)) != std::wstring::npos)
            {
                ws.replace(pos, from.size(), to);
                pos += to.size();
            }
        }

        char utf8[512] = {};
        WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, utf8, sizeof(utf8), nullptr, nullptr);
        return std::string(utf8);
    }

    // Dropdown selector for option ids. Uses the template-index lookup
    // (no printf) for the dropdown items, then calls getExplainText once
    // with the actual value for the resolved description shown next to
    // the row.
    bool RenderOptionCombo(std::uint8_t& optionId, int widgetOffset)
    {
        constexpr int MAX_KNOWN_OPTION = 36;  // 0..35 inclusive

        std::string currentLabel;
        if (optionId == 0xFF)
        {
            currentLabel = "(empty)";
        }
        else
        {
            currentLabel = OptionLabelWithPlaceholder(optionId);
            if (currentLabel.empty())
            {
                char fallback[32];
                std::snprintf(fallback, sizeof(fallback), "?? (%u)", optionId);
                currentLabel = fallback;
            }
        }

        bool changed = false;
        ImGui::PushID(widgetOffset);
        ImGui::SetNextItemWidth(230.0f);
        if (ImGui::BeginCombo("##opt", currentLabel.c_str()))
        {
            if (ImGui::Selectable("(empty)", optionId == 0xFF))
            {
                optionId = 0xFF;
                changed = true;
            }
            for (int opt = 0; opt < MAX_KNOWN_OPTION; ++opt)
            {
                const auto label = OptionLabelWithPlaceholder(static_cast<std::uint8_t>(opt));
                if (label.empty()) continue;
                char buf[544];
                std::snprintf(buf, sizeof(buf), "%3d  %s", opt, label.c_str());
                if (ImGui::Selectable(buf, optionId == opt))
                {
                    optionId = static_cast<std::uint8_t>(opt);
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
        return changed;
    }

    // Render one (option, value) pair: combobox for the option id,
    // numeric input for the value, and the live localized description.
    bool RenderOptionPair(std::uint8_t& opt, std::uint8_t& val,
                          int widgetOffsetBase)
    {
        bool changed = false;
        if (RenderOptionCombo(opt, widgetOffsetBase))         changed = true;
        ImGui::SameLine();
        if (RenderByteEdit(val, widgetOffsetBase + 1, "##v")) changed = true;

        const auto explanation = ExplainOptionUtf8(opt, static_cast<int>(val));
        if (!explanation.empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("= %s", explanation.c_str());
        }
        return changed;
    }
}

void CItemSetOptionEditorTable::RenderRow(int setIndex, bool& rowInteracted)
{
    ITEM_SET_OPTION* sets = g_csItemOption.GetItemSetOptions();
    ITEM_SET_OPTION& row = sets[setIndex];

    ImGui::TableNextRow();

    // Single row-scope ID. All helpers push sub-IDs unique within this row,
    // so there's no risk of cross-row collisions regardless of offset counts.
    ImGui::PushID(setIndex);

    int col = 0;

    // Column: Index (read-only).
    ImGui::TableSetColumnIndex(col++);
    ImGui::Text("%d", setIndex);

    // Column: Set name (UTF-8 editable).
    ImGui::TableSetColumnIndex(col++);
    {
        char utf8Buffer[MAX_ITEM_SET_NAME * 4] = {};
        WideCharToMultiByte(CP_UTF8, 0, row.strSetName, -1,
                            utf8Buffer, sizeof(utf8Buffer), nullptr, nullptr);

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##name", utf8Buffer, sizeof(utf8Buffer)))
        {
            std::memset(row.strSetName, 0, sizeof(row.strSetName));
            MultiByteToWideChar(CP_UTF8, 0, utf8Buffer, -1,
                                row.strSetName, MAX_ITEM_SET_NAME);
            rowInteracted = true;
        }
    }

    // Column: Set item count (computed at load time; show read-only).
    ImGui::TableSetColumnIndex(col++);
    ImGui::Text("%u", static_cast<unsigned>(row.bySetItemCount));

    // Column: Option count (editable byte).
    ImGui::TableSetColumnIndex(col++);
    if (RenderByteEdit(row.byOptionCount, 1, "##optcount"))
        rowInteracted = true;

    // Column: Standard options (per-piece bonuses, 2nd..6th piece).
    //         Only the first slot of each pair is meaningful in practice.
    ImGui::TableSetColumnIndex(col++);
    for (int o = 0; o < MAX_ITEM_SET_STANDARD_OPTION_COUNT; ++o)
    {
        ImGui::BeginGroup();
        if (RenderOptionPair(row.byStandardOption[o][0],
                             row.byStandardOptionValue[o][0],
                             100 + o * 2))
        {
            rowInteracted = true;
        }
        ImGui::EndGroup();
    }

    // Column: Ext options (applied when any 2 set pieces are equipped).
    ImGui::TableSetColumnIndex(col++);
    for (int o = 0; o < MAX_ITEM_SET_EXT_OPTION_COUNT; ++o)
    {
        ImGui::BeginGroup();
        if (RenderOptionPair(row.byExtOption[o], row.byExtOptionValue[o],
                             150 + o * 2))
        {
            rowInteracted = true;
        }
        ImGui::EndGroup();
    }

    // Column: Full options (applied only when the complete set is equipped).
    ImGui::TableSetColumnIndex(col++);
    for (int o = 0; o < MAX_ITEM_SET_FULL_OPTION_COUNT; ++o)
    {
        ImGui::BeginGroup();
        if (RenderOptionPair(row.byFullOption[o], row.byFullOptionValue[o],
                             200 + o * 2))
        {
            rowInteracted = true;
        }
        ImGui::EndGroup();
    }

    // Column: Required classes (bitmap across MAX_CLASS slots).
    ImGui::TableSetColumnIndex(col++);
    ImGui::BeginGroup();
    for (int c = 0; c < MAX_CLASS; ++c)
    {
        ImGui::PushID(300 + c);
        bool allowed = row.byRequireClass[c] != 0;
        if (ImGui::Checkbox("##cls", &allowed))
        {
            row.byRequireClass[c] = allowed ? 1 : 0;
            rowInteracted = true;
        }
        ImGui::PopID();
        if (c + 1 < MAX_CLASS) ImGui::SameLine();
    }
    ImGui::EndGroup();

    ImGui::PopID();
}

void CItemSetOptionEditorTable::Render(const std::string& searchFilter, int& selectedRow, bool showEmpty)
{
    (void)selectedRow;  // Selection not used yet; kept for parity with Item editor.

    const ITEM_SET_OPTION* sets = g_csItemOption.GetItemSetOptions();

    // Filter indices up-front so the row renderer only walks matches.
    std::vector<int> visibleRows;
    visibleRows.reserve(MAX_SET_OPTION);
    for (int i = 0; i < MAX_SET_OPTION; ++i)
    {
        const bool empty = (sets[i].strSetName[0] == L'\0');
        if (empty && !showEmpty) continue;
        if (MatchesFilter(searchFilter, sets[i].strSetName))
            visibleRows.push_back(i);
    }

    // WidthFixed (not Stretch) lets the total column width exceed the
    // visible table width, which is what actually activates the horizontal
    // scrollbar when ScrollX is set. With Stretch, ImGui shrinks columns
    // to fit and there's nothing to scroll.
    constexpr ImGuiTableFlags flags =
        ImGuiTableFlags_Borders      |
        ImGuiTableFlags_Resizable    |
        ImGuiTableFlags_ScrollX      |
        ImGuiTableFlags_ScrollY      |
        ImGuiTableFlags_RowBg;

    if (!ImGui::BeginTable("ItemSetOptionTable", 8, flags))
        return;

    ImGui::TableSetupScrollFreeze(2, 1);
    ImGui::TableSetupColumn(EDITOR_TEXT("label_index"),             ImGuiTableColumnFlags_WidthFixed,  50.0f);
    ImGui::TableSetupColumn(EDITOR_TEXT("col_itemsetopt_name"),     ImGuiTableColumnFlags_WidthFixed, 260.0f);
    ImGui::TableSetupColumn(EDITOR_TEXT("col_itemsetopt_piecect"),  ImGuiTableColumnFlags_WidthFixed,  60.0f);
    ImGui::TableSetupColumn(EDITOR_TEXT("col_itemsetopt_optct"),    ImGuiTableColumnFlags_WidthFixed,  90.0f);
    ImGui::TableSetupColumn(EDITOR_TEXT("col_itemsetopt_stdopt"),   ImGuiTableColumnFlags_WidthFixed, 420.0f);
    ImGui::TableSetupColumn(EDITOR_TEXT("col_itemsetopt_extopt"),   ImGuiTableColumnFlags_WidthFixed, 280.0f);
    ImGui::TableSetupColumn(EDITOR_TEXT("col_itemsetopt_fullopt"),  ImGuiTableColumnFlags_WidthFixed, 420.0f);
    ImGui::TableSetupColumn(EDITOR_TEXT("col_itemsetopt_classes"),  ImGuiTableColumnFlags_WidthFixed, 220.0f);
    ImGui::TableHeadersRow();

    bool anyRowInteracted = false;
    for (int idx : visibleRows)
    {
        RenderRow(idx, anyRowInteracted);
    }

    ImGui::EndTable();
}

#endif // _EDITOR
