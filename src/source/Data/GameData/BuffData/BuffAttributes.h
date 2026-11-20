#pragma once

#include <algorithm>
#include <array>
#include <string_view>

// The attributes a buff's power-up can raise (BuffPowerUp::attribute): OpenMU's Stats names in camelCase where OpenMU
// has the attribute, and names of Webzen's item add options (ADD_OPTION_*) where it has none. A name the list lacks is
// an error when Buffs.json loads (ValidateBuffs). See docs/buff-data.md, "Values".
namespace Data::Buffs
{
inline constexpr auto BuffAttributes = std::to_array<std::string_view>({
    // OpenMU's Stats.
    "attackSpeedAny",
    "baseDamageBonus",
    "defenseBase",
    "maximumHealth",
    "maximumMana",
    "maximumAbility",
    "maximumShield",
    "experienceRate",
    "bonusItemDropRate",
    "vulcanusExperienceRate",
    "totalStrength",
    "totalAgility",
    "totalVitality",
    "totalEnergy",
    "totalLeadership",
    "physicalBaseDmg",
    "wizardryBaseDmg",
    "healthRecoveryMultiplier",
    "manaRecoveryMultiplier",
    "criticalDamageChance",
    "excellentDamageChance",
    "deathPenaltyProtection",
    "itemDurabilityProtection",
    // Webzen's item add options which OpenMU has no attribute for.
    "disableExperience",
    "mobility",
    "vulcanusItemDropRate",
    "partyExperienceBonus",
    "vitalityBonus",
});

constexpr bool IsBuffAttribute(std::string_view name)
{
    return std::find(BuffAttributes.begin(), BuffAttributes.end(), name) != BuffAttributes.end();
}
} // namespace Data::Buffs
