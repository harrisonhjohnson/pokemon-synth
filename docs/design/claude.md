# Pokémon SYNTH — Claude Code Project Guide

> **Read this file at the start of every session.**
> It is the orientation layer: environment, conventions, phases, and spec pointers.
> All detailed design decisions live in `specs/`. Never deviate from recorded decisions without explicit user confirmation.

---

## Project Identity

| Field | Value |
|---|---|
| **Name** | Pokémon SYNTH |
| **Base ROM** | Pokémon Fire Red (BPRE, USA v1.0) |
| **Engine** | [CFRU](https://github.com/Skeli789/Complete-Fire-Red-Upgrade) + [DPE](https://github.com/Skeli789/Dynamic-Pokemon-Expansion) |
| **Language** | C + ARM Assembly (GBA), compiled via devkitARM on Windows |
| **Build** | `python scripts/make.py` from repo root |
| **Emulator** | mGBA (with debugger) |
| **Map editor** | AdvanceMap 1.9.x — user operates manually |
| **Script editor** | XSE — eXtreme Script Editor |
| **Sprite editor** | Tile Molester / Graphics Gale / Aseprite |

**Claude Code role:** Writes all C, ASM hooks, data tables, and XSE scripts. User reviews, tests, and operates GUI tools.

---

## Spec Files — When to Read

| File | Contents | Read when |
|---|---|---|
| `specs/synth_system.md` | SynthData struct, stat budget rules, Resonance Point system, type system, ability system, evolution logic, ultimate form, archetype tables | Any Synth C work |
| `specs/synth_menu.md` | Three-tab menu design, UI flows, palette system | Synth menu implementation |
| `specs/story_and_scripts.md` | All story beats, gift events, rival teams, E4, narrative framing | Any XSE scripting session |
| `specs/gym_leaders.md` | All 18 leaders, locations, types, confirmed aces | Gym/trainer design sessions |
| `specs/world_content.md` | Wild encounters, trainer updates, item placement, TMs, Pokémarts, fossil dealer, starter NPCs | Content population sessions |
| `specs/postgame.md` | Sevii Islands pools, daily reset logic, Cerulean Cave chambers, fixed encounters, Navel Rock/Birth Island, Resonance Amplifier reward | Phase 4 work |
| `specs/qol_and_systems.md` | QoL table, day/night cycle, Tent item, Learn Moves, Move Tutor, Hard Mode AI, level caps, cheat console | Feature implementation |
| `specs/assets_and_polish.md` | Title screen, credits, sprite resources, overworld sprites needed | Art integration and polish |

---

## Phase Roadmap

### Phase 1 — Foundation (Weeks 1–6)
**Goal: Buildable ROM with QoL, full dex data loaded, Synth species registered**

- [ ] Fork CFRU and DPE repos, set up Windows build pipeline
- [ ] Configure `src/config.h` QoL flags (DexNav, reusable TMs, repel, expanded bag)
- [ ] Import Gen 7–9 species data, sprites, cries into DPE via community sprite repos
- [ ] Register all 5 Synth species (SYNTH, SYNTH2, SYNTH3, SYNTH3_ULTIMATE, SYNTH_LEGEND) in DPE with placeholder sprites, stats, and cries
- [ ] Implement HM replacement system (Key Items / auto field abilities)
- [ ] Extend trade-free evolution to all Gen 4–9 methods
- [ ] Implement day/night cycle RTC hook (`src/day_night.c`)
- [ ] Implement Tent Key Item (`src/item_use.c` callback)
- [ ] Implement Move Tutor NPC in all Pokémon Centers
- [ ] Implement "Learn Moves" party menu sub-option
- [ ] Implement hidden cheat console on player room console (`src/cheat_console.c`)
- [ ] Draft wild encounter table spreadsheet for user review

### Phase 2 — Synth Engine (Weeks 6–20)
**Goal: All Synth mechanics functional in a test ROM**

- [ ] Define `SynthData` struct in `include/synth.h`
- [ ] Implement save block allocation for 6 Synth slots (~216 bytes)
- [ ] Implement `SynthData` save/load hooks
- [ ] Implement EV → Resonance Point conversion hook (4:1 ratio, per-stage cap)
- [ ] Implement `IsSynthMon()` (all 5 species IDs) and `GetSynthBaseBudget()` helper, stat override hooks in `src/battle_main.c`
- [ ] Implement type override hooks in `src/battle_util.c`
- [ ] Implement `SynthUpdatePalette()` and type color tables in `src/synth_palettes.c`
- [ ] Implement Synth evolution logic
- [ ] Implement CFRU form change hook for SYNTH3 → SYNTH3_ULTIMATE (`src/synth_evolution.c`)
- [ ] Implement Resonance Amplifier item use callback
- [ ] Implement Form system (preset stat tables, stage-appropriate display, GetClosestForm() auto-assignment)
- [ ] Implement `SynthValidateStats()` (per-stage caps, RP pool, min 20)
- [ ] Implement Synth menu — Tabs 1, 2, 3 (editor always open from Stage 1)
- [ ] Implement party screen hook to open Synth menu
- [ ] Implement hard level cap system
- [ ] Implement Hard Mode flag + Options menu toggle
- [ ] Implement Hard Mode AI decision tree (`src/battle_ai_hard.c`)
- [ ] Test all Synth mechanics in isolation before Phase 3

### Phase 3 — Content (Weeks 16–40, overlaps Phase 2 later half)
**Goal: Full game playable start to finish with new story and systems**

- [ ] Finalize wild encounter tables (post-user-review)
- [ ] Update all standard trainer teams by class
- [ ] Full item placement audit — overworld, hidden items, all Poké Mart inventories
- [ ] Draft full TM list (Gen 1–9 curated) — user review and approval
- [ ] Implement TM placements, Celadon Dept. Store evolution items, Fossil Dealer NPC
- [ ] Implement three starter-gifting NPCs
- [ ] Script all 18 gym leaders — trainer data + event scripts + type unlock flags
- [ ] Script Hard Mode gym leader strategy sequences
- [ ] SS Anne full rescript as Silph investor showcase
- [ ] All story beats scripted (see `specs/story_and_scripts.md`)
- [ ] All 6 Synth gift events scripted
- [ ] Rival fight teams designed and entered (fights 1–5)
- [ ] E4 + Champion teams designed
- [ ] Level cap table tuned against gym leader ace levels
- [ ] Title screen custom assets integrated
- [ ] All NPC dialogue passes for narrative consistency

### Phase 4 — Postgame & Polish (Weeks 36+)
**Goal: Postgame complete, balanced, playtested**

- [ ] Postgame stat editor (BST cap removed, per-stat max 255, RP uncapped)
- [ ] Implement `legendary_pools.c` — all 9 island pool arrays
- [ ] Implement daily RTC date reset logic for all 12 imprinting sites (36 bytes in save block)
- [ ] Script Silph Resonance Device gift + Seagallop access to Navel Rock and Birth Island
- [ ] Enable Navel Rock flag `0x8B5` and rescript; enable Birth Island flag `0x8C2` and rescript
- [ ] Script all 12 imprinting sites (Islands 1–7, Navel Rock ×2, Birth Island ×2)
- [ ] Source/create overworld sprites for Mew, Deoxys, Genesect, Arceus
- [ ] Source/create SYNTH3_ULTIMATE front + back sprites (64×64, palette-slot-compliant)
- [ ] Source/create SYNTH_LEGEND front + back sprites (64×64, palette-slot-compliant)
- [ ] Source/create 5 icon sprites (32×32, one per Synth species)
- [ ] Script Cerulean Cave four-chamber E4 legend encounters
- [ ] Implement fixed Kanto replacements: Zeraora (Power Plant), Chien-Pao (Seafoam Islands)
- [ ] Script Resonance Amplifier rival gift scene (gate: all 12 sites used at least once)
- [ ] Full balance pass: rival stat budgets, gym curve, E4 difficulty
- [ ] Audit CFRU/DPE Credits.md + sprite thread contributors — update credits screen
- [ ] Playtest full run (user) → bug fix sprint

---

## New Files Claude Code Will Create

```
src/
  synth_core.c          — SynthData logic, save/load, IsSynthMon, validators, RP conversion
  synth_menu.c          — full Synth menu UI (3 tabs)
  synth_palettes.c      — gTypeColorRamps and gTypeColorHighlights tables
  synth_evolution.c     — stage-based evolution redirect logic + SYNTH3_ULTIMATE form change hook
  synth_ability.c       — party ability scanner (Trace always included; remembered ability per Synth)
  day_night.c           — RTC-driven day/night cycle, palette shift callbacks
  learn_moves_menu.c    — "Learn Moves" party menu option
  cheat_console.c       — player room console cheat code input and validation
  battle_ai_hard.c      — Hard Mode AI decision tree
  legendary_pools.c     — static species ID arrays for each island summoning pool
  title_screen.c        — custom title screen integration
  credits_screen.c      — updated credits roll

include/
  synth.h               — shared struct definitions, constants, function declarations

graphics/
  title_bg.png               — user-produced, 4bpp indexed
  title_logo.png             — user-produced
  synth_title_sprite.png     — user-produced
  synth3_ultimate_front.png  — user-produced, 64×64, palette-slot-compliant
  synth3_ultimate_back.png   — user-produced, 64×64, palette-slot-compliant
  synth_legend_front.png     — user-produced, 64×64, palette-slot-compliant
  synth_legend_back.png      — user-produced, 64×64, palette-slot-compliant
  synth_icons.png            — user-produced, 5 × 32×32 icons (SYNTH/SYNTH2/SYNTH3/SYNTH3_ULTIMATE/SYNTH_LEGEND)

scripts/
  synth_gift_[1-6].pks
  ss_anne_showcase.pks
  cerulean_rocket_ambush.pks
  bills_house_lore.pks
  lavender_tower_chamber.pks
  fossil_dealer.pks
  starter_gift_fire.pks / water.pks / grass.pks
  move_tutor_pokecenter.pks
  silph_resonance_device.pks
  resonance_amplifier_gift.pks
  imprinting_site_[1-7a/7b/navel_a/navel_b/birth_a/birth_b].pks
  cerulean_cave_[mew/deoxys/genesect/arceus].pks
  zeraora_encounter.pks / chien_pao_encounter.pks
  type_unlock_[type].pks (×18)
  rival_fight_[1-5].pks
  rocket_hideout_data_terminal.pks
  cinnabar_lab_logs.pks
  giovanni_confrontation.pks
```

---

## Constants Reference

```c
// Synth species IDs (assign after last Gen 9 species in DPE)
#define SPECIES_SYNTH           (LAST_GEN9_SPECIES + 1)
#define SPECIES_SYNTH2          (LAST_GEN9_SPECIES + 2)
#define SPECIES_SYNTH3          (LAST_GEN9_SPECIES + 3)
#define SPECIES_SYNTH3_ULTIMATE (LAST_GEN9_SPECIES + 4)  // form change only — starter (synthIndex==0)
#define SPECIES_SYNTH_LEGEND    (LAST_GEN9_SPECIES + 5)  // 6th gift pseudo-legendary — standalone species

// Archetype IDs
#define ARCHETYPE_PHYSICAL_SWEEPER  0
#define ARCHETYPE_SPECIAL_SWEEPER   1
#define ARCHETYPE_DUAL_ATTACKER     2
#define ARCHETYPE_DEFENSIVE_WALL    3
#define ARCHETYPE_BULKY_SWEEPER     4
#define ARCHETYPE_SUPPORT           5

// Synth-specific flags (assign unused flag IDs)
#define FLAG_CHAMPION_DEFEATED            (assign unused flag ID)

// Item IDs (assign unused item IDs)
#define ITEM_TENT                  (assign unused item ID)
#define ITEM_RESONANCE_AMPLIFIER   (assign unused item ID)

// Secondary type sentinel
#define TYPE_NONE 0xFF  // Synth is single-typed

// Resonance Point conversion
#define RP_CONVERSION_RATIO  4   // 4 EVs = 1 Resonance Point
```

---

## Key Architectural Rules

1. **Never deviate from a decision in the Decisions Log** (`specs/synth_system.md`) without explicit user confirmation.
2. **Always check `IsSynthMon()`** before any stat or type lookup in battle code. `IsSynthMon()` must check all five species IDs including `SPECIES_SYNTH3_ULTIMATE` and `SPECIES_SYNTH_LEGEND`. Always use `GetSynthBaseBudget()` to resolve BST — never derive from `stage` alone, as `SYNTH_LEGEND` sits outside the stage progression.
3. **`SynthValidateStats()` must be called** before saving any manual stat edit.
4. **No Synth evolution can be cancelled** — biological, not optional. Do not expose a cancel path.
5. **The stat editor is always open.** There is no unlock gate — do not add one.
6. **Resonance Points are invisible to the player as EVs.** The conversion runs silently; only the combined budget total and the Resonance indicator are shown.
7. **The Resonance Amplifier is starter-only.** Always verify `synthIndex == 0`, `stage == 2`, and `FLAG_CHAMPION_DEFEATED` before allowing use.
8. **Tent changes RTC hour only, not date.** Imprinting site daily reset is date-gated — the Tent cannot be used to farm sites.
9. **Synth menu has no move tab.** Moves use the standard Learn Moves party option.
10. **Sprite repos:** Gen 1–7 → pokedex-data by Touched. Gen 7–9 → DS-style Gen VII+ PokéCommunity repo. Do NOT generate AI sprites for standard Pokémon.
11. **All credits** must reference community repo contributors. See `specs/assets_and_polish.md`.
