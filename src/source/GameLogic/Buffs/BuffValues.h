#pragma once

#include "Data/GameData/BuffData/BuffDefinition.h"

#include <cstddef>

// The values and durations of the buffs (Data/Buffs/Buffs.json, buffs design B13) as the hero has them now: the
// character stats, the buff strip and the tooltips of the items that give a buff read them here.
namespace GameLogic::Buffs
{
// The value of the buff's power-up at `index` with its terms on the hero's attributes (the auras' per level); 0 when
// the buff has no power-up there.
int GetPowerUpValue(const Data::Buffs::BuffDefinition& buff, size_t index);

// The same for the buff of that number; 0 for a number without a buff.
int GetPowerUpValue(int buffNumber, size_t index);

// How long the buff lasts in seconds when the server sends no time; 0 when only the server knows it.
int GetDuration(const Data::Buffs::BuffDefinition& buff);
} // namespace GameLogic::Buffs
