#include "stdafx.h"

#include "UIJewelHarmony.h"

#include "Data/GameData/ItemData/ItemOptionEffects.h"
#include "GameLogic/Items/ItemOptions.h"

JewelHarmonyInfo* JewelHarmonyInfo::MakeInfo()
{
    return new JewelHarmonyInfo;
}

JewelHarmonyInfo::~JewelHarmonyInfo() = default;

void JewelHarmonyInfo::GetStrengthenCapability(StrengthenCapability* pitemSC, const ITEM* pitem, const int index)
{
    const std::optional<GameLogic::Items::HarmonyOption> harmony = GameLogic::Items::FindHarmonyOption(*pitem);
    if (!harmony || !harmony->counts)
    {
        return;
    }
    const int value = harmony->value;
    switch (index)
    {
    case 0:
        pitemSC->SI_isNB = true;
        if (harmony->effect == Data::Items::HarmonyStrengthRequirement)
        {
            pitemSC->SI_NB.SI_force = value;
        }
        else if (harmony->effect == Data::Items::HarmonyAgilityRequirement)
        {
            pitemSC->SI_NB.SI_activity = value;
        }
        break;
    case 1:
        pitemSC->SI_isSP = true;
        switch (harmony->effect)
        {
        case Data::Items::HarmonyMinimumDamage:
            pitemSC->SI_SP.SI_minattackpower = value;
            break;
        case Data::Items::HarmonyMaximumDamage:
            pitemSC->SI_SP.SI_maxattackpower = value;
            break;
        case Data::Items::HarmonyDamage:
            pitemSC->SI_SP.SI_minattackpower = value;
            pitemSC->SI_SP.SI_maxattackpower = value;
            break;
        case Data::Items::HarmonyWizardryDamage:
            pitemSC->SI_SP.SI_magicalpower = value;
            break;
        case Data::Items::HarmonySkillDamage:
            pitemSC->SI_SP.SI_skillattackpower = value;
            break;
        case Data::Items::HarmonyAttackRate:
            pitemSC->SI_SP.SI_attackpowerRate = value;
            break;
        default:
            break;
        }
        break;
    case 2:
        pitemSC->SI_isSD = true;
        switch (harmony->effect)
        {
        case Data::Items::HarmonyDefense:
            pitemSC->SI_SD.SI_defense = value;
            break;
        case Data::Items::HarmonyMaximumAg:
            pitemSC->SI_SD.SI_AG = value;
            break;
        case Data::Items::HarmonyMaximumHealth:
            pitemSC->SI_SD.SI_HP = value;
            break;
        case Data::Items::HarmonyDefenseRate:
            pitemSC->SI_SD.SI_defenseRate = value;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}
