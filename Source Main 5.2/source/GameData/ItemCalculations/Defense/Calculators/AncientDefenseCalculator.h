#pragma once

#include "IDefenseCalculator.h"
#include "../ItemDefenseContext.h"

// ============================================================================
// AncientDefenseCalculator - Ancient/Set item defense bonuses
// ============================================================================
// Ancient items (cyan items) get additional defense based on item level.
//
// Formula: baseDefense + (currentDefense * 3 / dropLevel + 2 + dropLevel / 30)
//   where dropLevel = itemLevel + 30
//
// Applies to:
//   - Ancient armor (not shields - handled separately)
//   - Items with ancientDiscriminator > 0
//
// Note: This uses currentDefense, meaning it stacks with excellent bonus
//       if the item is both excellent and ancient!
//
// Example (Vine Armor, base=8, level=10, ancient, no excellent):
//   currentDefense = 8 + enhancement (e.g., 35 at +9)
//   dropLevel = 10 + 30 = 40
//   bonus = 8 + (35 * 3 / 40 + 2 + 40 / 30)
//        = 8 + (2 + 2 + 1)
//        = 8 + 5
//        = 13
// ============================================================================
class AncientDefenseCalculator : public IDefenseCalculator
{
public:
    int Calculate(ItemDefenseContext& context) const override
    {
        int dropLevel = context.GetDropLevel();
        int bonus = context.baseDefense + (context.currentDefense * 3 / dropLevel + 2 + dropLevel / 30);
        return bonus;
    }

    bool AppliesTo(const ItemDefenseContext& context) const override
    {
        // Ancient shields are handled in ShieldDefenseCalculator
        return context.IsAncient() &&
               !context.IsShield() &&
               context.baseDefense > 0;
    }

    int GetPriority() const override
    {
        return 70;  // After excellent, before wings
    }

    const char* GetName() const override
    {
        return "AncientDefense";
    }
};
