#pragma once

#include "IDefenseCalculator.h"
#include "../ItemDefenseContext.h"
#include "../EnhancementBonus.h"

// ============================================================================
// ShieldDefenseCalculator - Shield-specific defense bonuses
// ============================================================================
// Shields have special defense calculation:
//   1. Base defense
//   2. +1 defense per enhancement level (simpler than normal armor)
//   3. If ancient: additional ancient formula bonus
//
// Note: Shields do NOT get the standard enhancement bonus
//       They get +1 per level instead of +3 per level
//
// Formula:
//   defense = baseDefense + enhancementLevel
//   if (ancient): defense += defense * 20 / dropLevel + 2
// ============================================================================
class ShieldDefenseCalculator : public IDefenseCalculator
{
public:
    int Calculate(ItemDefenseContext& context) const override
    {
        int bonus = 0;

        // Shields get +1 defense per enhancement level
        bonus += EnhancementBonus::CalculateShield(context.enhancementLevel);

        // Ancient shields get additional bonus based on current defense
        if (context.IsAncient())
        {
            int dropLevel = context.GetDropLevel();
            int currentTotal = context.currentDefense + bonus;
            bonus += currentTotal * 20 / dropLevel + 2;
        }

        return bonus;
    }

    bool AppliesTo(const ItemDefenseContext& context) const override
    {
        return context.IsShield() && context.baseDefense > 0;
    }

    int GetPriority() const override
    {
        return 90;  // High priority - special case
    }

    const char* GetName() const override
    {
        return "ShieldDefense";
    }
};
