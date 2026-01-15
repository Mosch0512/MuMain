#pragma once

#include "IDefenseCalculator.h"
#include "../ItemDefenseContext.h"

// ============================================================================
// ExcellentDefenseCalculator - Excellent item defense bonuses
// ============================================================================
// Excellent items (green items) get additional defense based on item level.
//
// Formula: baseDefense * 12 / itemLevel + 4 + itemLevel / 5
//
// Applies to:
//   - Excellent armor (not shields, not wings)
//   - Items with excellentFlags > 0
//   - Items with itemLevel > 0
//
// Does NOT apply to:
//   - Shields (handled separately in ShieldDefenseCalculator)
//   - Wings/Capes (don't get excellent defense bonus)
//
// Example (Vine Armor, base=8, level=10, excellent):
//   bonus = 8 * 12 / 10 + 4 + 10 / 5
//        = 9 + 4 + 2
//        = 15
// ============================================================================
class ExcellentDefenseCalculator : public IDefenseCalculator
{
public:
    int Calculate(ItemDefenseContext& context) const override
    {
        if (context.itemLevel <= 0)
            return 0;

        int bonus = context.baseDefense * 12 / context.itemLevel + 4 + context.itemLevel / 5;
        return bonus;
    }

    bool AppliesTo(const ItemDefenseContext& context) const override
    {
        return context.IsExcellent() &&
               context.itemLevel > 0 &&
               !context.IsShield() &&
               !context.IsWing() &&
               context.baseDefense > 0;
    }

    int GetPriority() const override
    {
        return 80;  // After standard, before ancient
    }

    const char* GetName() const override
    {
        return "ExcellentDefense";
    }
};
