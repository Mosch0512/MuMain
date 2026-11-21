#include "stdafx.h"

#include "doctest.h"

// The looks monsters name (MN8, ML1 in docs/superpowers/specs/2026-11-13-monster-looks-design.md): a render style and
// an effect on a model entry, a monster entry, a number, a map variant and a state variant.

#include "Data/GameData/ItemData/ItemDataIssue.h"
#include "Data/GameData/MonsterData/MonsterDataValidation.h"
#include "Data/GameData/MonsterData/MonsterDefinition.h"
#include "Data/GameData/MonsterData/MonsterGroup.h"
#include "Data/GameData/MonsterData/MonsterModelsJson.h"
#include "Data/GameData/MonsterData/MonsterSetup.h"
#include "Data/GameData/MonsterData/MonstersJson.h"

#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using Data::Items::ItemDataIssue;
using Data::Items::ItemDataIssueSeverity;
using Data::Monsters::MonsterDefinition;
using Data::Monsters::MonsterGroup;
using Data::Monsters::MonsterModelDefinition;
using Data::Monsters::MonsterSetup;

namespace
{
std::string Messages(const std::vector<ItemDataIssue>& issues)
{
    std::string text;
    for (const ItemDataIssue& issue : issues)
        text += issue.ToString() + "\n";
    return text;
}

bool Has(const std::vector<ItemDataIssue>& issues, ItemDataIssueSeverity severity, const std::string& field)
{
    return std::ranges::any_of(issues, [&](const ItemDataIssue& issue)
                               { return issue.severity == severity && issue.field == field; });
}

std::vector<MonsterGroup> ReadMonsters(const std::string& monster, std::vector<ItemDataIssue>& issues)
{
    std::vector<MonsterGroup> monsters;
    Data::Monsters::ReadMonstersJson(R"({ "formatVersion": 2, "monsters": [ )" + monster + " ] }", "Monsters.json",
                                     monsters, issues);
    return monsters;
}

std::vector<MonsterModelDefinition> ReadModels(const std::string& model, std::vector<ItemDataIssue>& issues)
{
    std::vector<MonsterModelDefinition> models;
    Data::Monsters::ReadMonsterModelsJson(R"({ "formatVersion": 1, "models": [ )" + model + " ] }", "Models.json",
                                          models, issues);
    return models;
}

bool Known(std::string_view name)
{
    return name == "balrogBlueChrome" || name == "glowPulse";
}
} // namespace

TEST_CASE("Monster looks: a monster, its numbers and its variants name a render style and an effect (ML1, ML3) "
          "[data][monsters][looks]")
{
    std::vector<ItemDataIssue> issues;
    const std::vector<MonsterGroup> read = ReadMonsters(R"({
        "name": "Metal Balrog", "kind": "monster", "model": "MONSTER_MODEL_BALROG",
        "renderStyle": "balrogBlueChrome", "effect": "glowPulse",
        "mapVariants": [{"maps": [34], "renderStyle": "crywolfGreenChrome",
                         "stateVariants": [{"state": "crywolfReady", "renderStyle": "none"}]}],
        "numbers": [{"number": 63}, {"number": 64, "effect": "none"}] })",
                                                        issues);
    CHECK_MESSAGE(issues.empty(), Messages(issues));
    REQUIRE(read.size() == 1);
    REQUIRE(read[0].numbers.size() == 2);
    const MonsterSetup& first = read[0].numbers[0].setup;
    CHECK(first.renderStyle == "balrogBlueChrome");
    CHECK(first.effect == "glowPulse");
    CHECK(read[0].numbers[1].setup.effect == "none");
    REQUIRE(read[0].numbers[0].mapVariants.size() == 1);
    const MonsterSetup& onMap = read[0].numbers[0].mapVariants[0].setup;
    CHECK(onMap.renderStyle == "crywolfGreenChrome");
    REQUIRE(onMap.stateVariants.size() == 1);
    CHECK(onMap.stateVariants[0].setup.renderStyle == "none");

    // Written back, it reads the same, the looks after the render tier.
    const std::string text = Data::Monsters::WriteMonstersJson(std::span<const MonsterGroup>(read));
    INFO(text);
    CHECK(text.find(R"("renderStyle": "balrogBlueChrome")") != std::string::npos);
    std::vector<ItemDataIssue> again;
    CHECK(ReadMonsters(text.substr(text.find('[') + 1, text.rfind(']') - text.find('[') - 1), again) == read);
    CHECK_MESSAGE(again.empty(), Messages(again));
}

TEST_CASE("Monster looks: a look that is no name is fatal [data][monsters][looks]")
{
    for (const char* value : {R"("")", "3", "true", R"(["a"])"})
    {
        CAPTURE(value);
        std::vector<ItemDataIssue> issues;
        ReadMonsters(std::string(R"({ "name": "X", "kind": "monster", "renderStyle": )") + value +
                         R"(, "numbers": [{"number": 538}] })",
                     issues);
        CHECK(Has(issues, ItemDataIssueSeverity::Fatal, "monster 538 renderStyle"));
    }
}

TEST_CASE("Monster looks: a model names the looks of its monsters, written after its unblended meshes "
          "[data][monsters][looks]")
{
    std::vector<ItemDataIssue> issues;
    const std::vector<MonsterModelDefinition> models = ReadModels(
        R"({"slot": "MONSTER_MODEL_BALROG", "file": "Data/Monster/Monster12.bmd", "textureFolders": ["Monster"],
            "noneBlendMeshes": [1], "renderStyle": "balrogBlueChrome", "effect": "glowPulse"})",
        issues);
    CHECK_MESSAGE(issues.empty(), Messages(issues));
    REQUIRE(models.size() == 1);
    CHECK(models[0].renderStyle == "balrogBlueChrome");
    CHECK(models[0].effect == "glowPulse");

    const std::string text = Data::Monsters::WriteMonsterModelsJson(models);
    INFO(text);
    CHECK(text.find("noneBlendMeshes") < text.find("renderStyle"));
    CHECK(text.find("renderStyle") < text.find(R"("effect")"));

    std::vector<ItemDataIssue> empty;
    ReadModels(R"({"slot": "MONSTER_MODEL_BALROG", "file": "Data/Monster/Monster12.bmd", "textureFolders": ["Monster"],
                   "renderStyle": ""})",
               empty);
    CHECK(Has(empty, ItemDataIssueSeverity::Fatal, "model MONSTER_MODEL_BALROG renderStyle"));
}

TEST_CASE("Monster looks: a look no look file has is an error of the monster or model that names it, none names none "
          "[data][monsters][looks]")
{
    std::vector<ItemDataIssue> readIssues;
    const std::vector<MonsterGroup> read = ReadMonsters(R"({
        "name": "Metal Balrog", "kind": "monster", "model": "MONSTER_MODEL_BALROG",
        "renderStyle": "balrogBlueChrome", "effect": "nosuchEffect",
        "mapVariants": [{"maps": [34], "renderStyle": "nosuchStyle",
                         "stateVariants": [{"state": "crywolfReady", "renderStyle": "none"}]}],
        "numbers": [{"number": 63}] })",
                                                        readIssues);
    REQUIRE(read.size() == 1);
    const MonsterDefinition monster = [&]
    {
        MonsterDefinition entry;
        entry.number = 63;
        entry.setup = read[0].numbers[0].setup;
        entry.mapVariants = read[0].numbers[0].mapVariants;
        return entry;
    }();
    const MonsterDefinition* monsters[] = {&monster};
    std::vector<ItemDataIssue> issues;
    Data::Monsters::ValidateMonsterLooks(monsters, Known, Known, "Monsters.json", issues);
    INFO(Messages(issues));
    CHECK(issues.size() == 2);
    CHECK(Has(issues, ItemDataIssueSeverity::Error, "monster 63 effect"));
    CHECK(Has(issues, ItemDataIssueSeverity::Error, "monster 63 mapVariants[0] renderStyle"));

    MonsterModelDefinition model;
    model.slot = "MONSTER_MODEL_BALROG";
    model.renderStyle = "nosuchStyle";
    const MonsterModelDefinition* models[] = {&model};
    std::vector<ItemDataIssue> modelIssues;
    Data::Monsters::ValidateMonsterModelLooks(models, Known, Known, "Models.json", modelIssues);
    CHECK(modelIssues.size() == 1);
    CHECK(Has(modelIssues, ItemDataIssueSeverity::Error, "model MONSTER_MODEL_BALROG renderStyle"));
}
