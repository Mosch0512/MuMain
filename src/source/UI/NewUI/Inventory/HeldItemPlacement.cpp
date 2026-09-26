#include "stdafx.h"

#include "UI/NewUI/Inventory/HeldItemPlacement.h"

#include "Audio/DSPlaySound.h"
#include "Engine/Object/ZzzInfomation.h"
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

bool AutoMoveItemAtCursor(CNewUIInventoryCtrl* source, STORAGE_TYPE sourceType, CNewUIInventoryCtrl* target,
                          STORAGE_TYPE targetType, const std::function<bool(ITEM*)>& accepts)
{
    if (CNewUIInventoryCtrl::GetPickedItem() != nullptr || source == nullptr || target == nullptr)
        return false;

    ITEM* item = source->FindItemAtPt(MouseX, MouseY);
    if (item == nullptr || !accepts(item))
        return false;

    const ITEM_ATTRIBUTE& attribute = ItemAttribute[item->Type];
    const int targetIndex = target->FindEmptySlot(attribute.Width, attribute.Height);
    if (targetIndex < 0 || !target->CanMove(targetIndex, item))
        return false;

    // Picked up and hidden, so the item is off both grids until the server
    // answers; the answer puts it into the target or back.
    if (!CNewUIInventoryCtrl::CreatePickedItem(source, item))
        return false;
    CNewUIPickedItem* pickedItem = CNewUIInventoryCtrl::GetPickedItem();
    source->RemoveItem(item);
    pickedItem->HidePickedItem();

    if (!SendRequestEquipmentItem(sourceType, pickedItem->GetSourceLinealPos(), pickedItem->GetItem(), targetType,
                                  targetIndex))
    {
        CNewUIInventoryCtrl::BackupPickedItem();
        return false;
    }

    PlayBuffer(SOUND_GET_ITEM01);
    return true;
}
} // namespace UI::Items::Placement
