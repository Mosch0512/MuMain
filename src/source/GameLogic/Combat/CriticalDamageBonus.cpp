#include "stdafx.h"

#include "GameLogic/Combat/CriticalDamageBonus.h"

#include "Data/GameData/ItemData/ItemOptionEffects.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "GameLogic/Items/CSItemOption.h"
#include "GameLogic/Items/ItemOptions.h"
#include "GameLogic/Skills/SkillManager.h"
#include "Network/Server/SocketSystem.h"

#include <array>
#include <cmath>
#include <optional>

namespace GameLogic::Combat
{
namespace
{
// Increase Critical Damage: 1 damage per 30 energy and per 25 command of its caster (OpenMU's
// CriticalDamageIncreaseEffectInitializer).
constexpr float EnergyPerDamage = 30.f;
constexpr float CommandPerDamage = 25.f;

// An excellent hit deals the maximum damage times this (OpenMU's AttackableExtensions).
constexpr double ExcellentDamageFactor = 1.2;

// Its buff states (OpenMU's magic effects 5 and 148; 149 is the client's extended one) and its skills: the hero casts
// one of them when the buff is its own.
constexpr std::array<eBuffState, 3> CriticalDamageBuffs{
    eBuff_AddCriticalDamage, EFFECT_GREATER_CRITICAL_DAMAGE_MASTERED, EFFECT_GREATER_CRITICAL_DAMAGE_EXTENDED};
constexpr std::array<ActionSkillType, 4> CriticalDamageSkills{AT_SKILL_ADD_CRITICAL, AT_SKILL_ADD_CRITICAL_STR1,
                                                              AT_SKILL_ADD_CRITICAL_STR2, AT_SKILL_ADD_CRITICAL_STR3};

// The item's harmony option when its attribute is the critical damage bonus (Harmony.json: physical option 6,
// wizardry option 5) and it counts; its value at the harmony level.
int HarmonyCriticalDamage(ITEM& item)
{
    if (!AreItemOptionsActive(&item))
    {
        return 0;
    }
    const std::optional<GameLogic::Items::HarmonyOption> harmony = GameLogic::Items::FindHarmonyOption(item);
    return harmony && harmony->counts && harmony->effect == Data::Items::HarmonyCriticalDamage ? harmony->value : 0;
}

bool HasCriticalDamageBuff()
{
    for (const eBuffState buff : CriticalDamageBuffs)
    {
        if (g_isCharacterBuff((&Hero->Object), buff))
        {
            return true;
        }
    }
    return false;
}

bool KnowsCriticalDamageSkill()
{
    for (const ActionSkillType skill : CriticalDamageSkills)
    {
        if (gSkillManager.FindHeroSkill(skill))
        {
            return true;
        }
    }
    return false;
}

// The hero's own buff: one the hero can cast, with the hero's energy and command. The client can't tell who cast a
// buff: one the hero can't cast adds what its caster's stats give and isn't counted, and one from a party member who
// can cast it too counts with the hero's values.
void AddOwnBuff(CriticalDamageBonus& bonus)
{
    if (Hero == nullptr || CharacterAttribute == nullptr || !HasCriticalDamageBuff() || !KnowsCriticalDamageSkill())
    {
        return;
    }
    const float energy = static_cast<float>(CharacterAttribute->Energy + CharacterAttribute->AddEnergy);
    const float command = static_cast<float>(CharacterAttribute->Charisma + CharacterAttribute->AddCharisma);
    bonus.buffSkill = AT_SKILL_ADD_CRITICAL;
    bonus.buff = energy / EnergyPerDamage + command / CommandPerDamage;
    const CSkillTreeInfo& powerUp = CharacterAttribute->MasterSkillInfo[AT_SKILL_ADD_CRITICAL_STR1];
    if (powerUp.GetSkillLevel() > 0)
    {
        bonus.buff += powerUp.GetSkillValue();
    }
}
} // namespace

int CriticalDamageBonus::Buff() const
{
    return static_cast<int>(std::floor(buff));
}

CriticalDamageBonus GetHeroCriticalDamageBonus()
{
    CriticalDamageBonus bonus;
    if (CharacterMachine == nullptr)
    {
        return bonus;
    }
    bonus.setOptions = g_csItemOption.GetSetOptionValue(AT_SET_OPTION_IMPROVE_CRITICAL_DAMAGE);
    for (ITEM& item : CharacterMachine->Equipment)
    {
        bonus.harmonyOptions += HarmonyCriticalDamage(item);
    }
    bonus.socketOptions = g_SocketItemMgr.m_StatusBonus.m_iCriticalDamageBonus;
    AddOwnBuff(bonus);
    return bonus;
}

int GetHeroExcellentDamageBonus()
{
    if (CharacterMachine == nullptr)
    {
        return 0;
    }
    return g_csItemOption.GetSetOptionValue(AT_SET_OPTION_IMPROVE_EXCELLENT_DAMAGE) +
           g_SocketItemMgr.m_StatusBonus.m_iExcellentDamageBonus;
}

int GetCriticalHitDamage(int maximumDamage, const CriticalDamageBonus& criticalBonus)
{
    return maximumDamage + criticalBonus.Total();
}

int GetExcellentHitDamage(int maximumDamage, int excellentBonus, const CriticalDamageBonus& criticalBonus)
{
    return static_cast<int>(maximumDamage * ExcellentDamageFactor + excellentBonus) + criticalBonus.Total();
}
} // namespace GameLogic::Combat
