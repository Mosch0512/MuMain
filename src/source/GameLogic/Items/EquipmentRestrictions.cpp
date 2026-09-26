#include "stdafx.h"
#include "GameLogic/Items/EquipmentRestrictions.h"

#include "GameLogic/Items/ItemCategories.h"
#include "World/MapInfra/MapManager.h"

namespace GameLogic::Items
{
    bool CanTakeOff(int equipmentSlot, int world, const ITEM* pItemHelper, const ITEM* pItemWing)
    {
        if (world != WD_10HEAVEN)
            return true;

        if (equipmentSlot == EQUIPMENT_HELPER)
            return IsWingItem(pItemWing);
        if (equipmentSlot == EQUIPMENT_WING)
            return IsFlyingMount(pItemHelper);
        return true;
    }
}
