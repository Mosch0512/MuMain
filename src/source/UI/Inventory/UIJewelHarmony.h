#pragma once

// The Jewel of Harmony's options as the stat code adds them: the item's harmony option
// (GameLogic::Items::FindHarmonyOption, from Data/Items/Options/Harmony.json) split into the stat parts the callers
// read.

struct NaturalAbility
{
    int SI_force;
    int SI_activity;
    NaturalAbility() : SI_force(0), SI_activity(0) {}
};

struct StrikingPower
{
    int SI_minattackpower;
    int SI_maxattackpower;
    int SI_magicalpower;
    int SI_attackpowerRate;
    int SI_skillattackpower;

    StrikingPower()
        : SI_minattackpower(0), SI_maxattackpower(0), SI_magicalpower(0), SI_attackpowerRate(0), SI_skillattackpower(0)
    {
    }
};

struct StrengthenDefense
{
    int SI_defense;
    int SI_AG;
    int SI_HP;
    int SI_defenseRate;

    StrengthenDefense() : SI_defense(0), SI_AG(0), SI_HP(0), SI_defenseRate(0) {}
};

struct StrengthenCapability
{
    NaturalAbility SI_NB;
    bool SI_isNB;

    StrikingPower SI_SP;
    bool SI_isSP;

    StrengthenDefense SI_SD;
    bool SI_isSD;

    StrengthenCapability() : SI_isNB(false), SI_isSP(false), SI_isSD(false) {}
};

class JewelHarmonyInfo
{
public:
    virtual ~JewelHarmonyInfo();
    static JewelHarmonyInfo* MakeInfo();

    // Adds the item's harmony option to `pitemSC`'s part `index` (0 the requirement reductions, 1 the attack, 2 the
    // defense) by its effect, while it counts (requiresItemLevel); the part's flag says the item has one that counts.
    void GetStrengthenCapability(StrengthenCapability* pitemSC, const ITEM* pitem, const int index);

private:
    JewelHarmonyInfo() = default;
};
