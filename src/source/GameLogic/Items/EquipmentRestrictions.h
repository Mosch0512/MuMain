#pragma once

typedef struct tagITEM ITEM;

namespace GameLogic::Items
{
    // Whether the item in `equipmentSlot` may be taken off in `world`. In Icarus
    // the character must keep wings or a flying mount on, so the last one of
    // them cannot be taken off. The server has to enforce this as well; the
    // client check only spares the player the fall.
    bool CanTakeOff(int equipmentSlot, int world, const ITEM* pItemHelper, const ITEM* pItemWing);
}
