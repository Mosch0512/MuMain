#pragma once

#include "IDefenseCalculator.h"
#include "../ItemDefenseContext.h"
#include <algorithm>

// ============================================================================
// WingDefenseCalculator - Wing and Cape defense bonuses
// ============================================================================
// Wings and capes have different enhancement bonuses based on their tier:
//
// Tier 1 - Early Wings (Elf, Heaven, Satan):
//   +0 to +9:  level * 3
//   +10 to +15: arithmetic series starting at 25
//
// Tier 2 - Mid-Tier Wings (Spirits, Soul, Dragon, Darkness):
//   +0 to +9:  level * 2
//   +10 to +15: fallthrough (4+5+6+7+8+9)
//
// Tier 3 - Level 3 Wings (Storm through Dimension) + Overrule Cape:
//   +0 to +9:  level * 4
//   +10 to +15: arithmetic series starting at 5
//
// Tier 4 - Basic Capes (Lord, Fighter):
//   +0 to +9:  level * 2
//   +10 to +15: fallthrough (4+5+6+7+8+9)
//
// Note: Wings do NOT get standard enhancement bonus
//       They get these special multipliers instead
// ============================================================================
class WingDefenseCalculator : public IDefenseCalculator
{
public:
    int Calculate(ItemDefenseContext& context) const override
    {
        int bonus = 0;
        int level = context.enhancementLevel;

        // Determine wing tier and get multiplier
        int multiplier_0_to_9 = GetTierMultiplier(context);
        int bonus_10_start = GetTierBonusStart(context);

        // +0 to +9: level * multiplier
        bonus += std::min(9, level) * multiplier_0_to_9;

        // +10 to +15: Progressive bonus
        if (level > 9)
        {
            int levelsAbove9 = level - 9;

            if (context.IsLevel3Wing() || context.IsOverruleCape())
            {
                // Level 3 wings: arithmetic series from 5 to (levelsAbove9 + 4)
                // At +15: 5+6+7+8+9+10 = 45
                int first = bonus_10_start;
                int last = levelsAbove9 + 4;
                bonus += levelsAbove9 * (first + last) / 2;
            }
            else if (context.IsEarlyWing())
            {
                // Early wings: arithmetic series from 25 to (levelsAbove9 + 3)
                // This seems like a bug in original code (first=25 is very high)
                // But keeping it for compatibility
                int first = 25;
                int last = levelsAbove9 + 3;
                bonus += levelsAbove9 * (first + last) / 2;
            }
            else
            {
                // Mid-tier wings and basic capes: fallthrough pattern
                // Adds: 4 at +10, 5 at +11, 6 at +12, 7 at +13, 8 at +14, 9 at +15
                for (int i = 10; i <= level && i <= 15; i++)
                {
                    bonus += (i - 6);  // Adds 4, 5, 6, 7, 8, 9
                }
            }
        }

        return bonus;
    }

    bool AppliesTo(const ItemDefenseContext& context) const override
    {
        return context.IsWing() && context.baseDefense > 0;
    }

    int GetPriority() const override
    {
        return 60;  // After ancient
    }

    const char* GetName() const override
    {
        return "WingDefense";
    }

private:
    int GetTierMultiplier(const ItemDefenseContext& context) const
    {
        if (context.IsLevel3Wing() || context.IsOverruleCape())
            return 4;
        else if (context.IsMidTierWing() || context.IsBasicCape())
            return 2;
        else  // Early wings
            return 3;
    }

    int GetTierBonusStart(const ItemDefenseContext& context) const
    {
        if (context.IsLevel3Wing() || context.IsOverruleCape())
            return 5;
        else if (context.IsEarlyWing())
            return 25;
        else  // Mid-tier and basic capes
            return 4;
    }
};
