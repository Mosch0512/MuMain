#include "stdafx.h"

#include "UI/NewUI/Inventory/HeldItemPlacement.h"

#include "Engine/Object/ZzzInventory.h"

using SEASON3B::CNewUIInventoryCtrl;
using SEASON3B::CNewUIPickedItem;

namespace UI::Items::Placement
{
std::optional<HeldItemMove> FindHeldItemMove(CNewUIInventoryCtrl* target, STORAGE_TYPE targetType)
{
    CNewUIPickedItem* pickedItem = CNewUIInventoryCtrl::GetPickedItem();
    if (target == nullptr || pickedItem == nullptr || pickedItem->GetItem() == nullptr)
        return std::nullopt;

    HeldItemMove move;
    move.item = pickedItem->GetItem();
    move.sourceIndex = pickedItem->GetSourceLinealPos();
    move.targetType = targetType;
    move.targetIndex = pickedItem->GetTargetLinealPos(target);
    // Within one grid the source is the target's storage; the grid's own
    // type can differ (the chaos machine and lucky item grids take theirs
    // from the recipe or the window).
    const bool withinTarget = pickedItem->GetOwnerInventory() == target;
    move.sourceType = withinTarget ? targetType : pickedItem->GetSourceStorageType();

    if (move.sourceType == STORAGE_TYPE::UNDEFINED || move.targetIndex < 0 ||
        !target->CanMove(move.targetIndex, move.item))
    {
        return std::nullopt;
    }

    if (withinTarget && move.targetIndex == move.sourceIndex)
    {
        CNewUIInventoryCtrl::BackupPickedItem();
        return std::nullopt;
    }
    return move;
}

bool SendHeldItemMove(const HeldItemMove& move)
{
    return SendRequestEquipmentItem(move.sourceType, move.sourceIndex, move.item, move.targetType, move.targetIndex);
}
} // namespace UI::Items::Placement
