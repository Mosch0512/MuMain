#pragma once

#include "IDefenseCalculator.h"
#include "../ItemDefenseContext.h"
#include "../EnhancementBonus.h"

// ============================================================================
// StandardDefenseCalculator - Standard enhancement bonus for ALL armor
// ============================================================================
// Applies standard enhancement bonus to all armor pieces:
//   - Helm, Armor, Pants, Gloves, Boots
//   - Including excellent and ancient (they get this PLUS their special bonuses)
//   - NOT shields (handled separately with different formula)
//   - NOT wings/capes (handled separately with different formula)
//
// Formula: EnhancementBonus::CalculateStandard(level)
//   Levels 0-9:  +3 per level
//   Levels 10-15: +3*9 + (4+5+6+7+8+9)
//
// Example: Armor +15 = +66 defense
// ============================================================================
class StandardDefenseCalculator : public IDefenseCalculator
{
public:
    int Calculate(ItemDefenseContext& context) const override
    {
        // Base defense is already in currentDefense, just add enhancement bonus
        return EnhancementBonus::CalculateStandard(context.enhancementLevel);
    }

    bool AppliesTo(const ItemDefenseContext& context) const override
    {
        // Applies to ALL normal armor (including excellent and ancient)
        // NOT shields (they use +1 per level)
        // NOT wings (they use tier-specific formulas)
        return context.IsNormalArmor() && context.baseDefense > 0;
    }

    int GetPriority() const override
    {
        return 100;  // Highest priority - base calculation
    }

    const char* GetName() const override
    {
        return "StandardDefense";
    }
};
