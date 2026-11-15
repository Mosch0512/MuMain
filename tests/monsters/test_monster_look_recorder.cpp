#include "stdafx.h"

#include "doctest.h"

#include "MonsterCreationScope.h"
#include "MonsterLookRecorder.h"

#include "Core/Globals/_define.h"
#include "Core/Globals/_enum.h"
#include "Data/GameData/MonsterData/MonsterDatabase.h"
#include "Data/GameData/MonsterData/MonsterDefinition.h"
#include "Data/GameData/MonsterData/MonsterModelDatabase.h"
#include "Data/GameData/MonsterData/MonsterModelSlots.h"
#include "Engine/Object/w_CharacterInfo.h"

#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
using Data::Maps::MapRef;
using Data::Monsters::MonsterDefinition;
using Data::Monsters::MonsterSetup;

// The maps the number is drawn on: those its entry places it on (D61), or map 0 for none.
std::vector<MapRef> MapsOf(const MonsterDefinition& monster)
{
    return monster.maps.empty() ? std::vector<MapRef>{MapRef{}} : monster.maps;
}

bool NamesLook(const MonsterSetup& setup)
{
    if (setup.renderStyle || setup.effect)
        return true;
    return std::ranges::any_of(setup.stateVariants, [](const auto& state) { return NamesLook(state.setup); });
}

// Whether the number draws with a look of the data: its entry, a map variant, a state variant or its model names one.
bool HasLook(const MonsterDefinition& monster)
{
    if (NamesLook(monster.setup) ||
        std::ranges::any_of(monster.mapVariants, [](const auto& variant) { return NamesLook(variant.setup); }))
        return true;
    const std::optional<int> slot = Data::Monsters::FindMonsterModelReference(monster.model);
    const Data::Monsters::MonsterModelDefinition* model = slot ? g_MonsterModelDatabase.Find(*slot) : nullptr;
    return model != nullptr && (!model->renderStyle.empty() || !model->effect.empty());
}

std::set<int> SplitNumbers(const char* text)
{
    std::set<int> numbers;
    std::stringstream in(text);
    std::string number;
    while (std::getline(in, number, ','))
        numbers.insert(std::stoi(number));
    return numbers;
}

// The baseline lines of a number as it draws now, on each of its maps.
std::map<std::string, std::string> RecordNumber(MonsterRecorder::CreationScope& scope, const MonsterDefinition& monster)
{
    std::map<std::string, std::string> lines;
    for (const MapRef& map : MapsOf(monster))
    {
        scope.SetServerMap(map.map, map.stage);
        CHARACTER* character = scope.Create(monster.number);
        REQUIRE(character != nullptr);
        for (const MonsterRecorder::DrawSituation& situation : MonsterRecorder::DrawSituations(character->Object.Type))
        {
            lines[MonsterRecorder::BaselineKey(monster.number, map, situation)] =
                LookRecorder::Digest(MonsterRecorder::RecordMonster(*character, situation));
        }
    }
    scope.SetServerMap(0, 0);
    return lines;
}
} // namespace

TEST_CASE("The monster look recorder draws a monster the same way twice, and its draws reach the renderer "
          "[monsters][looks][recorder]")
{
    MonsterRecorder::CreationScope scope(0);
    const MonsterRecorder::ShippedLooks looks;
    CHARACTER* character = scope.Create(MONSTER_BALROG);
    REQUIRE(character != nullptr);
    for (const MonsterRecorder::DrawSituation& situation : MonsterRecorder::DrawSituations(character->Object.Type))
    {
        CAPTURE(situation.Describe());
        const LookRecorder::Record first = MonsterRecorder::RecordMonster(*character, situation);
        const LookRecorder::Record second = MonsterRecorder::RecordMonster(*character, situation);
        CHECK_FALSE(first.meshDraws.empty());
        CHECK_FALSE(first.gpuCalls.empty());
        CHECK(LookRecorder::FirstDifference(first, second).empty());
    }
}

// The baseline (tests/monsters/baseline/MonsterLookDraws.txt) holds every situation of every number with a look of
// the data, on each map the number is placed on, as its old code drew it. MU_MONSTER_LOOK_WRITE=<numbers,
// comma-separated> records those numbers as they draw now into the file: run it before their looks move.
TEST_CASE("Monster looks moved into data draw as their old code did (the baseline) [monsters][looks][recorder]")
{
    MonsterRecorder::CreationScope scope(0);
    const MonsterRecorder::ShippedLooks looks;
    std::map<std::string, std::string> baseline = MonsterRecorder::ReadBaseline();
    if (const char* write = std::getenv("MU_MONSTER_LOOK_WRITE"))
    {
        for (const int number : SplitNumbers(write))
        {
            const MonsterDefinition* monster = g_MonsterDatabase.Find(number);
            REQUIRE(monster != nullptr);
            const std::string prefix = std::to_string(number) + " on map ";
            std::erase_if(baseline, [&](const auto& line) { return line.first.rfind(prefix, 0) == 0; });
            baseline.merge(RecordNumber(scope, *monster));
        }
        MonsterRecorder::WriteBaseline(baseline);
        MESSAGE("Wrote the monster look baseline");
        return;
    }

    std::set<int> withLines;
    for (const auto& [key, digest] : baseline)
        withLines.insert(std::stoi(key.substr(0, key.find(' '))));
    for (int number = 0; number < Data::Monsters::MonsterNumberCount; ++number)
    {
        const MonsterDefinition* monster = g_MonsterDatabase.Find(number);
        if (monster == nullptr || !HasLook(*monster))
            continue;
        CAPTURE(number);
        CHECK(withLines.contains(number));
        for (const auto& [key, digest] : RecordNumber(scope, *monster))
        {
            CAPTURE(key);
            const auto line = baseline.find(key);
            REQUIRE(line != baseline.end());
            INFO(digest << " drawn, " << line->second << " kept");
            CHECK(LookRecorder::SameDigest(digest, line->second));
        }
    }
}
