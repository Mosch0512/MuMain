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
| ML1 | Where a monster names its looks (Q-ML1a, 2026-11-13) | **The model names its usual looks, a monster can name others.** A model entry in `Models.json` names the looks every monster of that model gets, as an item model entry names its render style; a monster entry, or one of its numbers (D61), can name another look in its place. So the 17 monsters that today get a variant on a shared model through the `ExtraMon` number (Metal Balrog on the Balrog model, Great Drakan on the Drakan model, …) name their own, and the others name nothing. Monster Setup shows the model's look in its dropdown ("(model: balrogRedChrome)") until a monster picks another. Rejected: looks only on the monster entries (monsters sharing a model and a look repeat the name), and one look per monster holding all its parts (an item's render style could not be put on a monster as it is, D25, and every variant would be a copy). |
| ML2 | The kinds of looks (Q-ML1b, 2026-11-13) | **Two kinds, as for the items, and an effect's parts say when they run.** A **render style** draws (L13); an **effect** spawns, in the update (ADR 0003). Each part of an effect has a **when**: always (every frame or by its own timing, what an item effect does), at an attack (an attack animation and tick, Q-ML3), at death. So an item effect is an effect whose parts are all "always", and a monster names at most one render style and one effect, on its model or itself (ML1). The kind "item effect" is called **effect** for items too (L15 left the name to this design): the Style Editor's root and the Data Checks area become Effects; whether the file and the items' field `itemEffect` are renamed with it is settled in the target design. Rejected: four kinds by moment (alive, attack, death effects as kinds and fields of their own: up to four names per monster and four roots), and monsters reusing the item kinds as they are with attack and death effects on the monster entry (they could not be shared as looks). |
| ML3 | Conditions (Q-ML2, 2026-11-13) | **Whole looks by the entry, details in the look.** What picks a whole look is the monster entry's: its `mapVariants` and `stateVariants` (MN4, MN5) and its numbers (D61) name another render style or effect, so Crywolf's green chrome is one look, `crywolfGreenChrome`, that the map variants of its eight monsters name, the Guardian Statue's siege glow is a state variant (the siege a named state like `crywolfElfTime`), and Zaikan's look is Tantallos' entry with its own number. What changes inside a look while the monster lives is the look's: L11's **only when** on a layer or an effect part gains the animation (and "not"), a frame range of it, and buffs on the monster (poison, freeze, the Hellas defense buffs, …), so the Castle Gate's layers are "not while dying" and Destler's meshes 3 and 4 "not from frame 2.5 of attack 2". A look then knows no maps or event states and can be put on an item or another monster as it is. Rejected: every condition in the look (looks would know maps and states), and every condition as a variant of the entry naming a whole look (a look copied per animation and per buff, the freeze tint for every monster). |
| ML4 | When an attack effect fires (Q-ML3, 2026-11-13) | **At a time, as today, or at a pose.** Today an attack starts two clocks: the animation at the model's speed (`animations` in `Models.json`, editable in Monster Model) and the attack counter, 1 to 15 at one step per 1/25 s at every speed (`ZzzCharacter.cpp:4076`); the effects fire at a step of the counter ("tick 11" is 0.4 s after the start), the sound cues (MNS) at a frame of the animation. At the original speeds the two meet; when an animation's speed changes, a tick effect stays at its time and leaves the swing, a cue follows it. An attack part's when says **tick** (moved effects, so the move is exact and the recorder matches) or **frame** of its attack animation (a pose, like a cue), and the Style Editor switches a part from one to the other at the frame where the tick lands at the model's speed. Monster Preview shows both on the frame ruler, tick parts where they land at the current speed. Rejected: ticks only (effects never follow a changed animation speed), and frames only (the move would shift timings by rounding, and effects after an animation's end need a rule). |
| ML5 | Things on bones (Q-ML4, 2026-11-13) | **Full detail in the data, recipes in the editor.** An effect part stores what it creates as it is: a sprite, particle or effect by its FX1 name, with subtype, size, colour (fixed, random or one of a list), turning and pulsing (L9's shapes), on bones (numbers or names, lists, ranges, "all but", a random bone) with an offset and jitter; a link (a joint between bones, from a point near a bone, to the origin) or a trail between two bones; things around the monster (at a random point within a radius, terrain light). So everything the code does can be written, and nothing needs new code to add. The Style Editor shows these parts as recipes, as L35 does for layers: **Add effect** offers a gallery (lights on bones, eye lights, thunder chain, fire on bones, walk dust, trail, …) that writes the parts, the list names them back in designer words ("Lights on bones: 13-26, blue"), and the fields are the advanced view. The same parts serve the items' things on bones (looks pull request 4). Rejected: the parts in engine terms in the editor too (hard to read), and only named recipes in the data (every odd case a new recipe in code). |
| ML6 | Logic that is more than a look (Q-ML5, 2026-11-13) | **As much as possible becomes data.** Of the about 40 rows of "Logic that is more than a look", only what talks to the server or runs a whole scene stays a hook (D49): the Kanturu gateway NPC's enter request and the Maya battle (with the falling monsters of Kanturu 3rd), and any row the target design finds that cannot be written without code, each named with its reason. Everything else becomes blocks of the effect kind (ML2), which runs in the update (ADR 0003: the draw stays read-only): presets that create several things (the inferno ring of eight bombs, a ring of N copies), values from the attack tick (the Hydra laser's light), counters and once-per-loop or once-per-action latches, alternating bones, copies of the model at offsets, screen shake, breaking into meshes, and also the blocks that act on the monster: start an animation (the Ballista's own attack, the Doppelganger's attack after appearing, the Butchers walking instead of standing), set a flag a later part reads (the Empire Guardian bosses' rage), set a buff the client shows (the Hellas skills), change height, selectability or the hidden mesh by a named state (the Battle Castle objects). Because looks then change game state, the target design gives these acting blocks their own Data Checks (an action on a monster that has no such animation, a flag nothing reads) and the monster recorder records their effects on the object, not only what is drawn. Rejected: what only shows becomes data and what changes the game stays a hook (about 15 hooks left), and all 40 rows staying hooks. |

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
| Q-ML1a | *Answered: ML1.* Where a monster names its looks. |
| Q-ML1b | *Answered: ML2.* Which kinds of looks. |
| Q-ML2 | *Answered: ML3.* Conditions. |
| Q-ML3 | *Answered: ML4.* When an attack effect fires. Where the trigger names the attack (animation, skill) is part of the target design. |
| Q-ML4 | *Answered: ML5.* Things on bones. |
| Q-ML5 | *Answered: ML6.* Logic that is more than a look. |
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
