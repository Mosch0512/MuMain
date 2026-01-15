#pragma once

#include "_enum.h"  // For ITEM_ constants

// ============================================================================
// ItemDefenseContext - Contains all data needed for defense calculations
// ============================================================================
// This struct is passed through the calculator chain, allowing each
// calculator to inspect item properties and modify the accumulated defense.
//
// The context pattern allows:
//   - Stateless calculators (no member variables needed)
//   - Easy testing (just create a context with test data)
//   - Flexible calculator ordering
//   - Calculators that depend on previous calculations
// ============================================================================
struct ItemDefenseContext
{
    // Input: Item properties
    int baseDefense;           // Base defense from ItemAttribute
    int itemType;              // Item type (ITEM_HELM, ITEM_SHIELD, etc.)
    int enhancementLevel;      // Enhancement level (+0 to +15)
    int excellentFlags;        // Excellent item flags (bitmask)
    int ancientDiscriminator;  // Ancient/Set item type (0 = not ancient)
    int itemLevel;             // Item level from ItemAttribute

    // Output: Accumulated defense
    int currentDefense;        // Running total of defense calculations

    // ========================================================================
    // Helper Methods
    // ========================================================================

    // Check if this is a shield
    bool IsShield() const
    {
        return itemType >= ITEM_SHIELD && itemType < ITEM_SHIELD + MAX_ITEM_INDEX;
    }

    // Check if this is a wing or cape
    bool IsWing() const
    {
        return (itemType >= ITEM_WING && itemType < ITEM_WING + MAX_ITEM_INDEX) ||
               (itemType >= ITEM_HELPER + 30);  // Capes start at ITEM_HELPER + 30
    }

    // Check if this is normal armor (Helm, Armor, Pants, Gloves, Boots)
    // NOT shields or wings
    bool IsNormalArmor() const
    {
        return (itemType >= ITEM_HELM && itemType < ITEM_BOOTS + MAX_ITEM_INDEX) &&
               !IsShield() &&
               !IsWing();
    }

    // Check if this is an excellent item
    bool IsExcellent() const
    {
        return excellentFlags > 0;
    }

    // Check if this is an ancient/set item
    bool IsAncient() const
    {
        return ancientDiscriminator > 0;
    }

    // Get drop level (item level + 30) - used in ancient/set calculations
    int GetDropLevel() const
    {
        return itemLevel + 30;
    }

    // Check if this is a specific wing tier (for WingDefenseCalculator)
    bool IsEarlyWing() const
    {
        // Elf, Heaven, Satan, etc.
        return itemType >= ITEM_WING && itemType < ITEM_WINGS_OF_SPIRITS;
    }

    bool IsMidTierWing() const
    {
        // Spirits, Soul, Dragon, Darkness
        return itemType >= ITEM_WINGS_OF_SPIRITS && itemType <= ITEM_WINGS_OF_DARKNESS;
    }

    bool IsLevel3Wing() const
    {
        // Storm through Dimension, plus Despair
        return (itemType >= ITEM_WING_OF_STORM && itemType <= ITEM_CAPE_OF_EMPEROR) ||
               (itemType >= ITEM_WINGS_OF_DESPAIR && itemType <= ITEM_WING_OF_DIMENSION);
    }

    bool IsBasicCape() const
    {
        return itemType == ITEM_CAPE_OF_LORD || itemType == ITEM_CAPE_OF_FIGHTER;
    }

    bool IsOverruleCape() const
    {
        return itemType == ITEM_CAPE_OF_OVERRULE;
    }
};
