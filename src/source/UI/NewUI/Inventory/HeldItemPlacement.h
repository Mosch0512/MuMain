#pragma once

#include "UI/NewUI/Inventory/NewUIInventoryCtrl.h"

#include <optional>

// Putting the item on the mouse cursor down into an item window (trade, chaos
// machine, lucky item window). The window decides whether it accepts the item;
// where the item came from (inventory, an inventory extension, equipment or
// the window's own grid) and where it goes is the same for all of them.
namespace UI::Items::Placement
{
// A move the server is asked to do.
struct HeldItemMove
{
    STORAGE_TYPE sourceType = STORAGE_TYPE::UNDEFINED;
    int sourceIndex = -1;
    ITEM* item = nullptr;
    STORAGE_TYPE targetType = STORAGE_TYPE::UNDEFINED;
    int targetIndex = -1;
};

// The move that puts the held item down into `target` at the squares under it,
// with `targetType` as the target storage. Nothing when no item is held, the
// item does not fit there or its source is unknown. Put back on the squares it
// came from, the item simply returns and there is no move.
[[nodiscard]] std::optional<HeldItemMove> FindHeldItemMove(SEASON3B::CNewUIInventoryCtrl* target,
                                                           STORAGE_TYPE targetType);

// Asks the server to do the move. Returns false when the request was not sent.
bool SendHeldItemMove(const HeldItemMove& move);
} // namespace UI::Items::Placement
