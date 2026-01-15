#pragma once

// ============================================================================
// Enhancement Bonus Calculation Utilities
// ============================================================================
// Centralized enhancement bonus calculations to eliminate code duplication
// between ZzzInfomation.cpp and ZzzInventory.cpp
//
// These functions calculate the bonus provided by item enhancement levels (+0 to +15)
// for various item attributes (defense, attack, blocking, etc.)
// ============================================================================

namespace EnhancementBonus
{
    // ========================================================================
    // Standard Enhancement Bonus (Defense, Attack, Blocking, Magic)
    // ========================================================================
    // Formula: +3 per level from 0-9, then progressive bonus for 10-15
    //
    // Levels 0-9:  bonus = level * 3
    // Levels 10-15: bonus = 27 + arithmetic series (4+5+6+7+8+9)
    //
    // Results:
    //   +0:  0
    //   +1:  3
    //   +9:  27
    //   +10: 31  (27 + 4)
    //   +11: 36  (27 + 4 + 5)
    //   +15: 66  (27 + 4 + 5 + 6 + 7 + 8 + 9)
    //
    // Used by:
    //   - Normal armor defense (Helm, Armor, Pants, Gloves, Boots)
    //   - Weapon attack damage
    //   - Successful blocking
    //   - Magic defense
    //   - Magic power
    // ========================================================================
    inline int CalculateStandard(int enhancementLevel)
    {
        if (enhancementLevel < 0) return 0;
        if (enhancementLevel > 15) enhancementLevel = 15;

        // +3 per level for levels 0-9
        int bonus = (enhancementLevel < 9 ? enhancementLevel : 9) * 3;

        // Progressive bonus for levels 10-15
        // Adds: 4 at +10, 5 at +11, 6 at +12, 7 at +13, 8 at +14, 9 at +15
        if (enhancementLevel > 9)
        {
            // Arithmetic series: sum from 4 to (enhancementLevel - 6)
            // Formula: n * (first + last) / 2
            int first = 4;
            int last = enhancementLevel - 6;
            int count = enhancementLevel - 9;
            bonus += count * (first + last) / 2;
        }

        return bonus;
    }

    // ========================================================================
    // Shield Enhancement Bonus
    // ========================================================================
    // Formula: +1 defense per level (much simpler than normal armor)
    //
    // Results:
    //   +0:  0
    //   +9:  9
    //   +15: 15
    //
    // Used by:
    //   - Shield defense only
    // ========================================================================
    inline int CalculateShield(int enhancementLevel)
    {
        if (enhancementLevel < 0) return 0;
        if (enhancementLevel > 15) enhancementLevel = 15;

        return enhancementLevel;
    }

    // ========================================================================
    // Wing Defense Enhancement Bonus (by tier)
    // ========================================================================
    // Different wing tiers have different multipliers:
    //   - Early wings (Elf, Heaven, Satan): *3 for 0-9
    //   - Mid-tier (Spirits, Soul, Dragon, Darkness): *2 for 0-9
    //   - Level 3 wings (Storm through Dimension): *4 for 0-9
    //   - Basic capes: *2 for 0-9
    //
    // For levels 10-15, each tier has a different progressive bonus pattern
    //
    // Note: This is handled in CalculateDefenseValue() currently
    //       Could be refactored here in the future
    // ========================================================================

    // ========================================================================
    // Helper: Calculate bonus using fallthrough pattern (for reference)
    // ========================================================================
    // This is what the original code used (switch with fallthrough cases)
    // Kept here for reference and verification
    // ========================================================================
    inline int CalculateStandardFallthrough(int enhancementLevel)
    {
        if (enhancementLevel < 0) return 0;
        if (enhancementLevel > 15) enhancementLevel = 15;

        int bonus = (enhancementLevel < 9 ? enhancementLevel : 9) * 3;

        // Fallthrough switch (intentional!)
        switch (enhancementLevel - 9)
        {
        case 6: bonus += 9;  // +15: adds 9
        case 5: bonus += 8;  // +14: adds 8
        case 4: bonus += 7;  // +13: adds 7
        case 3: bonus += 6;  // +12: adds 6
        case 2: bonus += 5;  // +11: adds 5
        case 1: bonus += 4;  // +10: adds 4
        default: break;
        }

        return bonus;
    }
}
