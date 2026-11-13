# Monster and NPC Looks in Data (MN8) Design (WIP)

> **Draft, 2026-11-13.** Phase MN8 of the
> [monsters design](2026-10-23-data-driven-monsters-design.md) and decision D25
> of the [data-driven content roadmap](2026-09-29-data-driven-content-roadmap-design.md):
> one look format for items, monsters and NPCs. It builds on the item looks of
> the [looks design](2026-11-01-looks-in-data-design.md) (items phase 13),
> whose render styles, item effects and Style Editor are on main since
> sven-n/MuMain#1022 and #1050. Written from the code of MuMain `upstream/main`
> @ `1211f0313`, after the monsters' PR 3c (#1103); line numbers are those of
> that commit.
>
> This document is temporary. Its last task deletes it, after what stays true
> has moved into `docs/monster-data.md`, `docs/effect-data.md` and the code.

## Goal

What makes a monster or NPC look the way it does, beyond its model, size and
equipment (which are data since MN3 and MN4), moves from code into data, and
is edited in MuEditor: its glow and shine, the effects on its bones, what its
attacks create, what it leaves when it dies, its shadow and its cloth. A look
made for an item can be put on a monster and the other way round (D25).

As in every phase before: nothing changes in game when a look moves from code
into data, each move is compared with the old code before the old code goes
(D50), and known bugs are fixed in commits of their own (D51).

## Scope

- **In:** the draw-time looks of monsters and NPCs (body light, tints,
  layers, extra models, outlines, ground decals, shadows), the per-frame
  things they spawn while alive (things on bones, links between bones,
  dust, terrain light, material values), their attack effects, the trap
  effects of `SetPlayerAttack`, death effects and debris, cloth, the item
  render styles only monsters use (`helperNpcPlate`, `monsterBattleBow`) and
  `RenderLinkObject`'s monster check (monsters design, MN8 row).
- **Also in:** the sounds those visual functions still play with `PlayBuffer`
  (about 210 calls in 15 files), as sound events of the sound sets (MNS, MNA:
  `during`, `once` and cues), as the monsters design's MN8 row says.
- **Out:** effect behaviour (how an effect moves and fades) stays code until
  FX2; looks name FX1 effect types. Mount looks (`mountLook`, the Dark Horse)
  are the roadmap's "pets and mounts" area. Cutscenes (Crywolf, Kanturu 3rd
  scripts) are their own later area (monsters design Q-MN58). Player
  characters' looks are not monsters.

## Decisions

Inherited, not asked again:

| # | Decision | From |
|---|---|---|
| D25 | One look format and one look editor for all areas; looks are named and referenced by name. | roadmap |
| D26 | Shared definitions: "used by", a warning before a shared change, copies for variants, no delete while used. | roadmap |
| D49 | Data first; code stays only as a named hook the entry names, where logic hands over to code. | monsters design |
| D50 | Each move compared with the old code before it goes; MN8 with the look recorder of items phase 13. | monsters design |
| D51 | Known bugs move as they are and are fixed in commits of their own. | monsters design |
| L15 | Monsters use the same look files; renaming the "item effect" kind for them is this design's call. | looks design |

New decisions (ML1, ML2, …) are added here as the open questions below are
answered.

| # | Topic | Decision |
|---|---|---|

## Current state

Three surveys of the code (2026-11-13, at `1211f0313`) found about **500
places** where code decides how a monster or NPC looks. This section gives
their shape; the pull requests list each case they move.

### Where it happens

The draw and the update were split by ADR 0003 (#917 and after): the draw
only reads, spawns happen in the update.

| Stage | Entry point | Map hooks | Rows |
|---|---|---|---|
| Draw: body passes | `RenderCharacter` (`ZzzCharacter.cpp:9614`), `Draw_RenderObject` (`ZzzObject.cpp:537`) | `RenderObjectMesh` / `RenderMonsterObjectMesh` of 22 map and event files | about 150 |
| Draw: overlays after the weapons | `Render*MonsterVisual` (5 maps, then `TheMapProcess().RenderMonsterVisual`) | Battle Castle, Crywolf, Kanturu, Doppelganger, Empire Guardian 3 | 10 |
| Update: things spawned while alive | `UpdateDrawnCharacterEffects` (`ZzzCharacter.cpp:12149`) → `UpdateMonsterBodyEffects`, `UpdateCharacterTypeEffects`, `UpdateRenderObjectEffects`, `UpdateCharacterCloth`, the map `Update*MonsterEffects` | 20 map files | about 110 |
| Update: movement visuals | `MoveCharacterVisual` (`:5501`) and the map `MoveMonsterVisual` chain | 18 map files | about 60 |
| Weapon trails | `ZzzCharacter.cpp:3780-3954` and the map `MoveBlurEffect` functions | 15 map files | about 45, all but 2 during attacks |
| Attack effects | `AttackEffect` (`:1586`, 87 monster cases) and its chain of 15 map hooks | Hellas (53 cases), Kanturu, Crywolf, Aida, … | about 80 |
| Traps | `SetPlayerAttack` (`:1240-1270`), `SetPlayerShock` (`:1429`) | – | 4 |
| Death | `SetPlayerDie` (`:1448`), `UpdateRenderObjectEffects` debris, the `deathEffect` consumers in 9 map files | – | about 30 |

A map hook claims a monster by returning true; the first map that claims it
wins. Many hooks are **not limited to their own map** (Hunting Ground, Aida,
Kanturu 1st to 3rd, Swamp of Quiet, the Balgass barracks and the monster part
of Hellas), so their looks show wherever the model appears; others are
(Battle Castle, Crywolf, Cursed Temple, Elbeland, Doppelganger, Karutan, PK
Field, Raklion).

### What is data already

`Monsters.json` and `Models.json` carry the values the setup applies
(`MonsterSetupApply.cpp`): `glowMesh` (27 entries), `glowLight`, `hiddenMesh`
(15), `lookLevel` (16), `light`, `shadow: false` (37), `outline` (32),
`alphaTarget`, `pkColor`, `helperPlate`, `subType` and `randomLook`,
`spawnEffects` (25) and the `appear` and `teleport` setups that replaced
`AppearMonster`, `deathEffect` (19, which only arms a flag), and on the
models `streamMesh`, `noneBlendMeshes`, `linkBones`, `namedBones`, `parts`
and `bitmaps`. There is no `renderStyle` or other look name on a monster.

The item looks are data: `Data/Effects/RenderStyles.json` and
`ItemEffects.json`, read by `LookJson.cpp` into `LookDefinition`
(`Data/GameData/LookData/LookDefinition.h`) and drawn by `LookDrawing.cpp`.
Their drawing takes any model (`DrawRenderStyle(style, BMD*, OBJECT*, …)`);
what is item-specific is how a look is found (by the item model entry), the
conditions (worn, inventory, ground, item level) and the per-item values.
Things on bones have no data format yet; for items they come with looks
pull request 4 (FX1.7c and the 12 looks with things on bones).

### What the looks are made of

From the surveys, by kind. "Fits" means an existing block of the item looks
does it once the monster can name a look; "new" means a block or condition
the look format does not have.

**Draw-time (about 160 rows).**

- Fits: layers of texture, reflection (all chrome motions), glow, scroll,
  pulse (wave, bounce), tint and light, per mesh (about 45 %).
- New conditions: the map or map group (about 45 rows: Crywolf draws
  Tantallos, Zaikan, Bloody Wolf, Beam Knight and the dragons in green
  chrome), the animation and frame (about 25: hidden while dying, glow while
  attacking), `SubType` (6), buffs on the monster (8: poison, freeze, the
  Hellas defense buffs, the Crywolf altar states), the map event's state (7:
  the siege, Selupan's pattern, the Maya battle, the Life Stone's build step).
- New blocks: a different look per monster on one shared model (17 rows,
  today the `ExtraMon` number passed into the drawing), an extra model drawn
  with the monster (6: the Red Dragon's boss head and princess on bones, the
  Phoenix of Darkness' second model and its two enlarged copies, Spider Eggs
  drawn three times), an outline shell (5), ground decals and auroras (10:
  the Archangel's ring, the Wolf Altars' circles, the Maya battle rings), an
  afterimage (Dreadfear), a shadow without a mesh (3), the blends dark
  (subtract), wave (vertex wobble), Doppelganger and no depth test, texture
  frames from time, an opacity cap.

**Per frame while alive (about 170 rows, plus 45 attack trails).**

- Things on bones: sprites (about 95 rows), particles (75), effect models
  (14), with offsets, sizes, colours (fixed, random, from a list), turning
  and pulsing; bones by number or by name, lists, ranges, "all but", a
  random bone.
- Links: joints between two bones (8), from a random point near a bone, a
  weapon trail between two bones (2 always on, 45 while attacking).
- Around the monster: walk dust at a random point near it (30), terrain light
  (12), a ground decal.
- Material values: texture scroll, glow-mesh strength pulsing, flickering or
  ramping while attacking (15), eye positions (`MoveEye`, 17).
- Timing: every frame, once per reference frame, a chance of 1 in N, frames
  a to b of an animation, once per animation loop.

**Attacks (about 80 rows).** Attack effects do not follow animation frames:
they run while `c->AttackTime` counts from 1 to 15 (`ZzzCharacter.cpp:4076`),
once per tick (`CheckAttackTime`). So the trigger is "at tick N" or "every
tick from N", filtered by the attack animation and by the skill
(`AT_SKILL_BOSS` for 13 cases, the energy ball, the lightning family, the
Hellas skills, the monster skills 51 and 52 of Dark Ghost). The positions:
the monster, a bone with an offset, a weapon bone, the target, a line from a
bone to the target (joints), a ring of N copies, a random point on the
ground within a radius.

**Death (about 30 rows).** Bones and stones thrown at death (Death Cow, Stone
Golem), parts at bones that take over the body (`deathEffect`: Kundun,
Snowman, Lion Gate, Condra, Totem Golem, the Shadow family, the Ice Giants),
mesh debris (Statue of Saint, Castle Gate), a body hidden while its debris
shows, fades over frames.

**Cloth (7).** The Magic Skeleton's cape (from its mesh 2), the robes of the
Crust family, the Death King and Nightmare, the Dark Phoenix's hair on its
second model, Fred's cloth; the cloak rules (frozen while stunned, none in
Chaos Castle).

### Logic that is more than a look

About 40 rows do more than "create X at Y when Z" (D49's hooks): the Battle
Castle objects that read the siege state, the Kanturu machines and the Maya
battle, Crywolf's Ballista starting its own attack, the Kanturu gateway NPC
that sends the enter request, Kundun's two-stage death with screen shake,
the Cursed Santa's staged death, `CreateInferno` (a ring of eight bombs), the
Hellas skill helpers that also set buffs, the Doppelganger appearing, the
Empire Guardian bosses' rage flags, Dark Phoenix's second-model bones,
`MoveHumming` (particles aimed from bone to bone).

## Problems found

Kept as they are in the move and fixed in commits of their own (D51), unless
a question below decides otherwise.

| # | Problem | Where |
|---|---|---|
| ML-P1 | The Blood Castle red light can never be on in castle 3: the clause compares one number with three (`==` && `==`). Castles 1 and 2 get no light either (`nCastle / 3` is 0). This is the monsters design's P25. | `ZzzCharacter.cpp:12276-12283` |
| ML-P2 | The tints of Alquamos, Drakan and the Cherry Blossom Spirit are dead: `Light` is overwritten at 9907 before the body reads it. | `ZzzCharacter.cpp:9817, 9831, 9883` |
| ML-P3 | The golden monsters' body light swap is a no-op (overwritten by `RenderPartObjectBodyColor`); the golden colours for 493–502 in `ZzzObject.cpp:7026` are never reached. | `ZzzCharacter.cpp:9784-9815` |
| ML-P4 | The shadow list names Gate to Kalima 1 only; gates 2–7 cast shadows (the monsters design's P19). The list is kept twice. | `ZzzCharacter.cpp:9739`, `12229` |
| ML-P5 | The Battle Castle gate is drawn twice while alive and still drawn while dying (`success` is never set). | `GMBattleCastle.cpp:1898` |
| ML-P6 | The Wolf Altars' circles read an uninitialised light and rotation when no buff matches. | `GMCrywolf1st.cpp:1990-2025` |
| ML-P7 | Death Spirit and Satyros leave `StreamMesh` set after drawing. | `GMCrywolf1st.cpp:1430`, `GM3rdChangeUp.cpp:666`, `GM_Kanturu_1st.cpp:741` |
| ML-P8 | Dead cases shadowed by an earlier hook: Kanturu 3rd's Persona, Twin Tail and Dreadfear; the Barracks' Balram, Soram and Death Spirit on Crywolf; Empire Guardian 4's Death Angel 3; `GMUnitedMarketPlace::RenderMonster` is never called. | several |
| ML-P9 | Little Santa and Satyros use the red channel for all three (grey). | `ZzzObject.cpp:2150`, `GM_Kanturu_1st.cpp:747` |
| ML-P10 | Crywolf's Beam Knight joints never show (`to` is always null), and Tantallos and Beam Knight fall through into the generic attack effects. | `GMCrywolf1st.cpp:936` |
| ML-P11 | Hell Maine's loop of five storms fires once. | `GMAida.cpp:1110` |
| ML-P12 | Effects and sounds that run every frame where once was meant: the Hellas skill helpers, Dark Ghost's meteor, the Cursed Temple heartbeat, Queen Rainer's joints. | `GMHellas.cpp:700`, `GMEmpireGuardian2.cpp:1057`, `w_CursedTemple.cpp:160`, `ZzzCharacter.cpp:5693` |
| ML-P13 | Random chances not scaled to the frame rate: the Little Santas' particle, the Cursed King's blood, the Fire Golem's step sound. | `ZzzObject.cpp:2545`, `ZzzCharacter.cpp:5752`, `GMHuntingGround.cpp` |
| ML-P14 | Medusa, Moss and Zyro arm `deathEffect`, which nothing reads for them. | `Monsters.json` |
| ML-P15 | Uninitialised values: Alquamos' attack angle, Aticles Head's sprite offset. | `ZzzCharacter.cpp:2147`, `GMEmpireGuardian2.cpp` |
| ML-P16 | The monsters design's P10 (`ZzzCharacter.cpp:10054`) is no longer at that line; it may be ML-P1. To check. | – |

## Open questions

Asked one at a time, each with a mockup of the data and the editor, and
answered here as decisions.

| # | Question |
|---|---|
| Q-ML1 | **Where a monster names its looks, and which kinds.** On the monster entry, on its model entry with the monster able to override, or both; the item kinds (render style, item effect) reused, or kinds of their own (body, every frame, attack, death). |
| Q-ML2 | **Conditions.** How a look says "only on Crywolf", "only while dying", "only from frame 2.5 of attack 2", "only with SubType 1", "only with the freeze buff", "only during the siege". |
| Q-ML3 | **Attack effects.** Where they live (in the look, or on the entry's `attacks` of MN2), and how "at tick 14 of attack 1 with the boss skill, create 36 spirit joints around the monster" is written. |
| Q-ML4 | **Things on bones.** The format for sprites, particles, effects, joints and trails on bones, shared with looks pull request 4 for the items. |
| Q-ML5 | **Logic that is more than a look.** Which of the about 40 rows become blocks and which stay hooks. |
| Q-ML6 | **Map hooks that are not limited to their map.** Keep the looks showing everywhere, or limit them to their map. |
| Q-ML7 | **Cloth.** A cloth block in the look, or cloth stays code and the entry names it. |
| Q-ML8 | **The sounds of the visual code.** Into the sound sets as `during`, `once` and cue events now, or later. |
| Q-ML9 | **The editor.** The Style Editor with a Monsters root, the preview in Monster Preview, attack effects in Monster Attacks. |
| Q-ML10 | **Which problems are fixed** in this work, and how the fixes are shown (recorder differences, in-game before and after). |
| Q-ML11 | **The pull requests.** How the move is cut, and in which order. |

## Tasks

1. Answer Q-ML1 to Q-ML11 and write the decisions.
2. Write the target design: the format, the drawing, the checks, the editor,
   the comparison, the pull requests.
3. Link this document from the roadmap and the monsters design's MN8 row.
4. The design as a docs pull request, reviewed before code (as the looks
   design was).
5. Last: what stays true into `docs/monster-data.md` and
   `docs/effect-data.md`; this document deleted.
