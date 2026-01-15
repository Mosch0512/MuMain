#pragma once

#include "IDefenseCalculator.h"
#include "../ItemDefenseContext.h"
#include "../EnhancementBonus.h"

// ============================================================================
// StandardDefenseCalculator - Base defense for normal armor
// ============================================================================
// Applies standard enhancement bonus to normal armor pieces:
//   - Helm, Armor, Pants, Gloves, Boots
//   - NOT shields (handled separately)
//   - NOT wings/capes (handled separately)
//   - NOT excellent/ancient (additional bonuses handled separately)
//
// Formula: baseDefense + EnhancementBonus::CalculateStandard(level)
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
        // Only applies to normal armor (not shields, not wings, not excellent, not ancient)
        // Excellent and ancient items get their own special formulas instead
        return context.IsNormalArmor() &&
               !context.IsExcellent() &&
               !context.IsAncient() &&
               context.baseDefense > 0;
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
