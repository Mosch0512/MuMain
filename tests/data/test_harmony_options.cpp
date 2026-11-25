#include "stdafx.h"

#include "doctest.h"

#include "ClientDataScope.h"
#include "TooltipDataScope.h"

#include "Data/DataHandler/ItemData/ItemDataHandler.h"
#include "Data/GameData/ItemData/ItemDatabase.h"
#include "Engine/Object/ItemOptionLines.h"
#include "GameLogic/Items/ItemOptions.h"
#include "I18N/All.h"
#include "UI/Inventory/ItemMng.h"
#include "UI/Inventory/UIJewelHarmony.h"

#include <array>
#include <filesystem>
#include <string>

namespace
{
const std::filesystem::path ClientDirectory = std::filesystem::path(MU_TEST_DATA_DIR).parent_path();

// JewelOfHarmonyOption_eng.bmd as the client read it before Harmony.json: per kind (0 weapons, groups 0 to 4; 1 staffs,
// group 5; 2 defense items, groups 6 to 11) and number, the line ("<name> +<value>", the damage decrement with a %),
// the value at each harmony level 0 to 13 and the removal's Zen.
struct OldHarmonyOption
{
    int kind;
    int number;
    const wchar_t* lineStart;
    const wchar_t* lineEnd;
    std::array<int, 14> values;
    std::array<int, 14> zen;
};

constexpr OldHarmonyOption OldHarmonyOptions[] = {
    {0,
     1,
     L"Min. attack increase +",
     L"",
     {2, 3, 4, 5, 6, 7, 9, 11, 12, 14, 15, 16, 17, 20},
     {100000, 110000, 120000, 130000, 140000, 150000, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {0,
     2,
     L"Max. attack increase +",
     L"",
     {3, 4, 5, 6, 7, 8, 10, 12, 14, 17, 20, 23, 26, 29},
     {100000, 110000, 120000, 130000, 140000, 150000, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {0,
     3,
     L"Required strength decrease +",
     L"",
     {6, 8, 10, 12, 14, 16, 20, 23, 26, 29, 32, 35, 37, 40},
     {100000, 110000, 120000, 130000, 140000, 150000, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {0,
     4,
     L"Required agility decrease +",
     L"",
     {6, 8, 10, 12, 14, 16, 20, 23, 26, 29, 32, 35, 37, 40},
     {100000, 110000, 120000, 130000, 140000, 150000, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {0,
     5,
     L"Attack increase(Min,Max) +",
     L"",
     {0, 0, 0, 0, 0, 0, 7, 8, 9, 11, 12, 14, 16, 19},
     {0, 0, 0, 0, 0, 0, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {0,
     6,
     L"Critical damage increase +",
     L"",
     {0, 0, 0, 0, 0, 0, 12, 14, 16, 18, 20, 22, 24, 30},
     {0, 0, 0, 0, 0, 0, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {0,
     7,
     L"Skill attack increase +",
     L"",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 14, 16, 18, 22},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 280000, 320000, 360000, 400000, 500000}},
    {0,
     8,
     L"Attack sucess rate increase(PVP) +",
     L"",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 7, 9, 11, 14},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 280000, 320000, 360000, 400000, 500000}},
    {0,
     9,
     L"SD decrement increase +",
     L"",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 5, 7, 9, 10},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 280000, 320000, 360000, 400000, 500000}},
    {0,
     10,
     L"SD bypass rate increase +",
     L"",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 500000}},
    {1,
     1,
     L"Wizardry increase +",
     L"",
     {6, 8, 10, 12, 14, 16, 17, 18, 19, 21, 23, 25, 27, 31},
     {100000, 110000, 120000, 130000, 140000, 150000, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {1,
     2,
     L"Required strength decrease +",
     L"",
     {6, 8, 10, 12, 14, 16, 20, 23, 26, 29, 32, 35, 37, 40},
     {100000, 110000, 120000, 130000, 140000, 150000, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {1,
     3,
     L"Required agility decrease +",
     L"",
     {6, 8, 10, 12, 14, 16, 20, 23, 26, 29, 32, 35, 37, 40},
     {100000, 110000, 120000, 130000, 140000, 150000, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {1,
     4,
     L"Skill attack increase +",
     L"",
     {0, 0, 0, 0, 0, 0, 7, 10, 13, 16, 19, 22, 25, 30},
     {0, 0, 0, 0, 0, 0, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {1,
     5,
     L"Critical damage increase +",
     L"",
     {0, 0, 0, 0, 0, 0, 10, 12, 14, 16, 18, 20, 22, 28},
     {0, 0, 0, 0, 0, 0, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {1,
     6,
     L"SD decrement increase +",
     L"",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 6, 8, 10, 13},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 280000, 320000, 360000, 400000, 500000}},
    {1,
     7,
     L"Attack success rate increase(PVP) +",
     L"",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 7, 9, 11, 14},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 280000, 320000, 360000, 400000, 500000}},
    {1,
     8,
     L"SD bypass rate increase +",
     L"",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 500000}},
    {2,
     1,
     L"Defense increase +",
     L"",
     {3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 22, 25},
     {100000, 110000, 120000, 130000, 140000, 150000, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {2,
     2,
     L"Max. AG increase +",
     L"",
     {0, 0, 0, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 25},
     {0, 0, 0, 130000, 140000, 150000, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {2,
     3,
     L"Max. HP increase +",
     L"",
     {0, 0, 0, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 30},
     {0, 0, 0, 130000, 140000, 150000, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {2,
     4,
     L"Life auto increment increase +",
     L"",
     {0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8},
     {0, 0, 0, 0, 0, 0, 200000, 220000, 240000, 280000, 320000, 360000, 400000, 500000}},
    {2,
     5,
     L"Mana auto increment increase +",
     L"",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 280000, 320000, 360000, 400000, 500000}},
    {2,
     6,
     L"Defense success rate increase(PVP) +",
     L"",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 5, 6, 8},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 280000, 320000, 360000, 400000, 500000}},
    {2,
     7,
     L"Damage decrement increase +",
     L"%",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 5, 6, 7},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 280000, 320000, 360000, 400000, 500000}},
    {2,
     8,
     L"SD ratio increase +",
     L"",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 500000}},
};

// An item of each kind: a Kris, a Skull Staff, a Dragon Helm.
constexpr std::array<int, 3> KindItems = {Data::Items::MakeItemType(0, 0), Data::Items::MakeItemType(5, 0),
                                          Data::Items::MakeItemType(7, 1)};

ITEM MakeHarmonyItem(int itemType, int itemLevel, int number, int harmonyLevel)
{
    ItemCreationParams params{};
    params.Group = itemType / MAX_ITEM_INDEX;
    params.Number = itemType % MAX_ITEM_INDEX;
    params.Level = static_cast<BYTE>(itemLevel);
    params.HasHarmonyOption = true;
    params.HarmonyOptionType = static_cast<BYTE>(number);
    params.HarmonyOptionLevel = static_cast<BYTE>(harmonyLevel);
    ITEM item;
    FillItemFromParameters(item, params);
    return item;
}
} // namespace

// The move into Harmony.json keeps every option: its value and Zen at each harmony level 0 to 13 and its English line
// as the old file gave them.
TEST_CASE("Every Jewel of Harmony option gives the old file's values, Zen and line [data][items]")
{
    ClientDataScope client(ClientDirectory);
    I18N::SetLocale("en");
    std::string errorMessage;
    REQUIRE(g_ItemDataHandler.Load(errorMessage));

    for (const OldHarmonyOption& old : OldHarmonyOptions)
    {
        CAPTURE(old.kind);
        CAPTURE(old.number);
        const Data::Items::ItemOption* option = GameLogic::Items::FindHarmonyOptionOf(KindItems[old.kind], old.number);
        REQUIRE(option != nullptr);
        CHECK(option->requiresItemLevel);
        for (int level = 0; level < 14; ++level)
        {
            CAPTURE(level);
            CHECK(option->value.At(level) == old.values[level]);
            CHECK(option->ZenAt(level) == old.zen[level]);
        }
        REQUIRE(option->texts.size() == 1);
        wchar_t line[256] = {};
        REQUIRE(Engine::Items::OptionLines::Write(option->texts.front(), {.value = old.values[13]}, line, 256));
        CHECK(std::wstring(line) == std::wstring(old.lineStart) + std::to_wstring(old.values[13]) + old.lineEnd);
    }
    // The numbers the old file had no option of.
    CHECK(GameLogic::Items::FindHarmonyOptionOf(KindItems[1], 9) == nullptr);
    CHECK(GameLogic::Items::FindHarmonyOptionOf(KindItems[2], 9) == nullptr);
    CHECK(GameLogic::Items::FindHarmonyOptionOf(KindItems[0], 11) == nullptr);
}

// The stat code adds an option by its attribute while it counts: from the item's + level equal to the harmony level.
TEST_CASE("A harmony option raises the stat part of its attribute while the item level reaches it [data][items]")
{
    ClientDataScope client(ClientDirectory);
    I18N::SetLocale("en");
    std::string errorMessage;
    REQUIRE(g_ItemDataHandler.Load(errorMessage));
    TooltipDataScope data;

    // A Kris with option 5 (minimum and maximum damage, from level 6) at level 9.
    ITEM kris = MakeHarmonyItem(KindItems[0], 9, 5, 9);
    StrengthenCapability attack{};
    g_pUIJewelHarmonyinfo->GetStrengthenCapability(&attack, &kris, 1);
    CHECK(attack.SI_isSP);
    CHECK(attack.SI_SP.SI_minattackpower == 11);
    CHECK(attack.SI_SP.SI_maxattackpower == 11);
    // The same at item level 8: it doesn't count.
    kris.Level = 8;
    StrengthenCapability lower{};
    g_pUIJewelHarmonyinfo->GetStrengthenCapability(&lower, &kris, 1);
    CHECK_FALSE(lower.SI_isSP);
    CHECK(lower.SI_SP.SI_minattackpower == 0);
    const std::optional<GameLogic::Items::HarmonyOption> found = GameLogic::Items::FindHarmonyOption(kris);
    REQUIRE(found);
    CHECK_FALSE(found->counts);
    CHECK(found->value == 11);

    // A Skull Staff's option 1 is wizardry; a Dragon Helm's option 1 defense.
    ITEM staff = MakeHarmonyItem(KindItems[1], 13, 1, 13);
    StrengthenCapability wizardry{};
    g_pUIJewelHarmonyinfo->GetStrengthenCapability(&wizardry, &staff, 1);
    CHECK(wizardry.SI_SP.SI_magicalpower == 31);
    ITEM helm = MakeHarmonyItem(KindItems[2], 13, 1, 13);
    StrengthenCapability defense{};
    g_pUIJewelHarmonyinfo->GetStrengthenCapability(&defense, &helm, 2);
    CHECK(defense.SI_SD.SI_defense == 25);
    // Its other parts stay empty.
    StrengthenCapability requirements{};
    g_pUIJewelHarmonyinfo->GetStrengthenCapability(&requirements, &helm, 0);
    CHECK(requirements.SI_NB.SI_force == 0);

    // A harmony level past the list's last (13) takes its last value.
    ITEM top = MakeHarmonyItem(KindItems[2], 15, 1, 15);
    StrengthenCapability topDefense{};
    g_pUIJewelHarmonyinfo->GetStrengthenCapability(&topDefense, &top, 2);
    CHECK(topDefense.SI_SD.SI_defense == 25);

    // Arrows (ammunition) link no harmony definition, so they take no Jewel of Harmony.
    CHECK_FALSE(GameLogic::Items::FindHarmonyDefinition(Data::Items::MakeItemType(4, 15)));
    CHECK(GameLogic::Items::FindHarmonyDefinition(KindItems[0]));
}
