#include "stdafx.h"
#include "ItemDefense.h"
#include "Calculators/StandardDefenseCalculator.h"
#include "Calculators/ShieldDefenseCalculator.h"
#include "Calculators/ExcellentDefenseCalculator.h"
#include "Calculators/AncientDefenseCalculator.h"
#include "Calculators/WingDefenseCalculator.h"
#include <algorithm>

// ============================================================================
// ItemDefense Implementation
// ============================================================================

ItemDefense::ItemDefense()
{
    InitializeCalculators();
}

void ItemDefense::InitializeCalculators()
{
    // Create all calculators
    // Order doesn't matter here - they'll be sorted by priority
    calculators.push_back(std::make_unique<StandardDefenseCalculator>());
    calculators.push_back(std::make_unique<ShieldDefenseCalculator>());
    calculators.push_back(std::make_unique<ExcellentDefenseCalculator>());
    calculators.push_back(std::make_unique<AncientDefenseCalculator>());
    calculators.push_back(std::make_unique<WingDefenseCalculator>());

    // Sort by priority (highest first)
    std::sort(calculators.begin(), calculators.end(),
        [](const std::unique_ptr<IDefenseCalculator>& a, const std::unique_ptr<IDefenseCalculator>& b) {
            return a->GetPriority() > b->GetPriority();
        });
}

int ItemDefense::CalculateDefense(int baseDefense, int itemType, int enhancementLevel,
                                   int excellentFlags, int ancientDiscriminator, int itemLevel)
{
    // Early exit if no base defense
    if (baseDefense == 0)
        return 0;

    // Create context with input data
    ItemDefenseContext context;
    context.baseDefense = baseDefense;
    context.itemType = itemType;
    context.enhancementLevel = enhancementLevel;
    context.excellentFlags = excellentFlags;
    context.ancientDiscriminator = ancientDiscriminator;
    context.itemLevel = itemLevel;
    context.currentDefense = baseDefense;  // Start with base

    // Apply each calculator that matches
    for (const auto& calculator : calculators)
    {
        if (calculator->AppliesTo(context))
        {
            int bonus = calculator->Calculate(context);
            context.currentDefense += bonus;

            // Debug logging (can be enabled for troubleshooting)
            // printf("[ItemDefense] %s: +%d (total: %d)\n",
            //        calculator->GetName(), bonus, context.currentDefense);
        }
    }

    return context.currentDefense;
}
