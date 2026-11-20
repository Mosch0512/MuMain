#include "stdafx.h"

#include "GameLogic/Buffs/BuffValues.h"

#include "Data/GameData/BuffData/BuffDatabase.h"
#include "GameLogic/Items/ItemOptions.h"

namespace GameLogic::Buffs
{
namespace
{
// A buff has no item level: its values are constants (Buffs.json rejects a list).
constexpr int NoLevel = 0;
} // namespace

int GetPowerUpValue(const Data::Buffs::BuffDefinition& buff, size_t index)
{
    return index < buff.powerUps.size() ? Items::ComputeOptionValue(buff.powerUps[index].value, NoLevel) : 0;
}

int GetPowerUpValue(int buffNumber, size_t index)
{
    const Data::Buffs::BuffDefinition* buff = g_BuffDatabase.Find(buffNumber);
    return buff != nullptr ? GetPowerUpValue(*buff, index) : 0;
}

int GetDuration(const Data::Buffs::BuffDefinition& buff)
{
    // Buffs.json allows no terms here, so the value is never stopped at an option slot's 65535.
    return buff.duration ? buff.duration->At(NoLevel) : 0;
}
} // namespace GameLogic::Buffs
