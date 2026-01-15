#pragma once

#include <vector>
#include <memory>
#include "ItemDefenseContext.h"
#include "Calculators/IDefenseCalculator.h"

// ============================================================================
// ItemDefense - Main defense calculation orchestrator
// ============================================================================
// Manages the collection of defense calculators and coordinates their
// execution in priority order.
//
// Usage:
//   ItemDefense defenseCalc;
//   int defense = defenseCalc.CalculateDefense(baseDefense, itemType, ...);
//
// The calculator chain:
//   1. StandardDefenseCalculator - Normal armor enhancement bonus
//   2. ShieldDefenseCalculator - Shield-specific bonuses
//   3. ExcellentDefenseCalculator - Excellent item bonuses
//   4. AncientDefenseCalculator - Ancient/Set item bonuses
//   5. WingDefenseCalculator - Wing/Cape bonuses
//
// Each calculator:
//   - Checks if it applies to the item (AppliesTo)
//   - Calculates its bonus (Calculate)
//   - Bonus is added to context.currentDefense
//
// Future: Add configuration file support for formula tuning
// ============================================================================
class ItemDefense
{
public:
    ItemDefense();
    ~ItemDefense() = default;

    // Main defense calculation entry point
    // @param baseDefense: Base defense from ItemAttribute
    // @param itemType: Item type constant (ITEM_HELM, ITEM_SHIELD, etc.)
    // @param enhancementLevel: Enhancement level (+0 to +15)
    // @param excellentFlags: Excellent item flags (bitmask)
    // @param ancientDiscriminator: Ancient/Set item type (0 = not ancient)
    // @param itemLevel: Item level from ItemAttribute
    // @return: Total calculated defense
    int CalculateDefense(int baseDefense, int itemType, int enhancementLevel,
                        int excellentFlags, int ancientDiscriminator, int itemLevel);

private:
    // Initialize the calculator chain
    void InitializeCalculators();

    // Collection of calculators (ordered by priority)
    std::vector<std::unique_ptr<IDefenseCalculator>> calculators;
};
