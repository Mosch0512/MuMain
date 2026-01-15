#pragma once

// Forward declaration
struct ItemDefenseContext;

// ============================================================================
// IDefenseCalculator - Interface for defense calculation strategies
// ============================================================================
// Each calculator implements a specific aspect of defense calculation:
//   - StandardDefenseCalculator: Base defense (no special modifiers)
//   - ShieldDefenseCalculator: Shield-specific bonuses
//   - ExcellentDefenseCalculator: Excellent item bonuses
//   - AncientDefenseCalculator: Ancient/Set item bonuses
//   - WingDefenseCalculator: Wing/Cape defense bonuses
//
// Calculators are applied in priority order, allowing earlier calculators
// to affect the context for later ones.
// ============================================================================
class IDefenseCalculator
{
public:
    virtual ~IDefenseCalculator() = default;

    // Calculate the defense bonus this calculator contributes
    // @param context: Current item and calculation state
    // @return: Defense bonus to add to the total
    virtual int Calculate(ItemDefenseContext& context) const = 0;

    // Check if this calculator applies to the given item
    // @param context: Current item and calculation state
    // @return: true if this calculator should be used
    virtual bool AppliesTo(const ItemDefenseContext& context) const = 0;

    // Get priority for ordering (higher = calculated first)
    // Suggested priorities:
    //   100 = Base/Standard (first)
    //   90  = Shields (special case, may early-exit)
    //   80  = Excellent items
    //   70  = Ancient items
    //   60  = Wings/Capes
    // @return: Priority value
    virtual int GetPriority() const = 0;

    // Get calculator name for debugging/logging
    // @return: Human-readable calculator name
    virtual const char* GetName() const = 0;
};
