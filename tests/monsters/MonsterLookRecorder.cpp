#include "stdafx.h"

#include "MonsterLookRecorder.h"

#include "Core/Globals/_define.h"
#include "Core/Globals/_enum.h"
#include "Core/Utilities/Random.h"
#include "Data/DataHandler/LookData/LookDataHandler.h"
#include "Data/GameData/ItemData/ItemDataIssue.h"
#include "Data/GameData/LookData/LookStore.h"
#include "Data/GameData/MonsterData/MonsterAnimations.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Items/LookDrawing.h"
#include "Render/Models/ZzzBMD.h"

#include "doctest.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

extern double WorldTime;

namespace MonsterRecorder
{
namespace
{
const std::filesystem::path DataDirectory = MU_TEST_DATA_DIR;
const std::filesystem::path Baseline = MU_MONSTER_LOOK_BASELINE;

// A time well into a session, with a fraction, as the item looks' recorder pins it.
constexpr double PinnedWorldTime = 1234567.25;

// The key Random::DrawRand takes for the drawn monster: an address no object has, the same in every run.
const void* FixedDrawKey()
{
    return reinterpret_cast<const void*>(static_cast<std::uintptr_t>(0x20000));
}

constexpr const char* BaselineHeader =
    "# How the monster looks moved from code into Data/Effects draw (MN8): one line per monster number, map and\n"
    "# situation, then the number of mesh draws and five digests of them and of what the drawing left on the model\n"
    "# (tests/data/LookRecorder.h). Written from the old code before each look moved (MU_MONSTER_LOOK_WRITE=1), so a\n"
    "# moved look still draws as its code did.\n";

std::string Number(double value)
{
    char text[32];
    std::snprintf(text, sizeof(text), "%g", value);
    return text;
}
} // namespace

std::string DrawSituation::Describe() const
{
    std::string text = Data::Monsters::GetMonsterAnimationName(action) + " at frame " + Number(frame) + ", " +
                       Number(worldTime) + " ms";
    if (alpha < 1.f)
        text += ", alpha " + Number(alpha);
    if (buff)
        text += ", buff " + std::to_string(*buff);
    return text;
}

std::vector<DrawSituation> DrawSituations(int slot)
{
    const int actions = Models[slot].NumActions;
    std::vector<DrawSituation> situations;
    for (const int action : {MONSTER01_STOP1, MONSTER01_WALK, MONSTER01_ATTACK1, MONSTER01_DIE})
    {
        if (action < actions)
            situations.push_back(
                {.action = action, .frame = action == MONSTER01_STOP1 ? 0.f : 1.5f, .worldTime = PinnedWorldTime});
    }
    situations.push_back({.worldTime = PinnedWorldTime + 700.5});
    situations.push_back({.worldTime = PinnedWorldTime, .buff = eDeBuff_Poison});
    situations.push_back({.worldTime = PinnedWorldTime, .alpha = 0.5f});
    return situations;
}

LookRecorder::Record RecordMonster(CHARACTER& character, const DrawSituation& situation)
{
    OBJECT& object = character.Object;
    BMD& model = Models[object.Type];
    const OBJECT keptObject = object;
    const char keptStreamMesh = model.StreamMesh;
    vec3_t keptBodyLight;
    VectorCopy(model.BodyLight, keptBodyLight);
    const bool keptLightEnable = model.LightEnable;
    const double previousWorldTime = WorldTime;

    // The key of the drawn object's randomness stays the same from run to run.
    const Random::DrawnAs fixedKey(&object, FixedDrawKey());
    WorldTime = situation.worldTime;
    object.CurrentAction = static_cast<unsigned short>(situation.action);
    object.PriorAction = static_cast<unsigned short>(situation.action);
    object.AnimationFrame = situation.frame;
    object.PriorAnimationFrame = situation.frame;
    object.Alpha = situation.alpha;
    object.Visible = true;
    object.m_BuffMap.ClearBuff();
    if (situation.buff)
        object.m_BuffMap.RegisterBuff(static_cast<eBuffState>(*situation.buff));

    LookRecorder::Record record = LookRecorder::RecordDrawing(model, [&] { RenderCharacter(&character, &object, 0); });

    WorldTime = previousWorldTime;
    object = keptObject;
    model.StreamMesh = keptStreamMesh;
    VectorCopy(keptBodyLight, model.BodyLight);
    model.LightEnable = keptLightEnable;
    return record;
}

struct ShippedLooks::State
{
    std::vector<Data::Looks::LookDefinition> renderStyles;
    std::vector<Data::Looks::LookDefinition> effects;
};

ShippedLooks::ShippedLooks() : m_state(new State)
{
    using Data::Looks::g_LookStore;
    using Data::Looks::LookKind;
    m_state->renderStyles.assign(g_LookStore.GetRenderStyles().begin(), g_LookStore.GetRenderStyles().end());
    m_state->effects.assign(g_LookStore.GetEffects().begin(), g_LookStore.GetEffects().end());
    for (const LookKind kind : {LookKind::RenderStyle, LookKind::Effect})
    {
        Data::Looks::RenderStylesLoadResult shipped = Data::Looks::LoadLookFile(DataDirectory / "Effects", kind);
        REQUIRE_FALSE(Data::Items::HasFatal(shipped.issues));
        g_LookStore.SetLooks(kind, std::move(shipped.looks));
    }
    Render::Items::Looks::PrepareTextures();
}

ShippedLooks::~ShippedLooks()
{
    Data::Looks::g_LookStore.SetRenderStyles(std::move(m_state->renderStyles));
    Data::Looks::g_LookStore.SetEffects(std::move(m_state->effects));
    delete m_state;
}

std::map<std::string, std::string> ReadBaseline()
{
    std::map<std::string, std::string> lines;
    std::ifstream in(Baseline);
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        const size_t tab = line.rfind('\t');
        if (line.empty() || line[0] == '#' || tab == std::string::npos)
            continue;
        lines[line.substr(0, tab)] = line.substr(tab + 1);
    }
    return lines;
}

void WriteBaseline(const std::map<std::string, std::string>& lines)
{
    std::filesystem::create_directories(Baseline.parent_path());
    std::ofstream out(Baseline, std::ios::binary | std::ios::trunc);
    out << BaselineHeader;
    for (const auto& [key, digest] : lines)
        out << key << '\t' << digest << '\n';
}

std::string BaselineKey(int number, const Data::Maps::MapRef& map, const DrawSituation& situation)
{
    std::string text = std::to_string(number) + " on map " + std::to_string(map.map);
    if (map.stage != 0)
        text += " stage " + std::to_string(map.stage);
    return text + ", " + situation.Describe();
}
} // namespace MonsterRecorder
