# specs/synth_system.md
# Synth System — Data Structures, Stats, Types, Abilities, Evolution

Read this spec for any C work touching SynthData, stat/type/ability logic, or evolution.

---

## SynthData Struct

Stored in CFRU's expanded save block, indexed by party slot (0–5) and box slot for boxed Synths.
6 total Synth slots. Allocate ~204 bytes in save expansion (6 × 34 bytes).

```c
typedef struct {
    u8  isSynth;             // 1 if this mon is a Synth, 0 otherwise
    u8  synthIndex;          // which Synth (0-5, for story tracking — 6 total gifts)
    u8  stage;               // 0=Synth1, 1=Synth2, 2=Synth3
    u8  primaryType;         // runtime type ID (overrides species table)
    u8  secondaryType;       // 0xFF = single-typed (TYPE_NONE)
    u8  archetype;           // archetype ID (0-5)
    u8  isUltimateForm;      // 1 if starter Synth has used Resonance Amplifier (synthIndex==0 only)
    u8  postGameUnlocked;    // 1 after Champion flag — removes BST cap entirely
    u16 baseStats[6];        // [HP, Atk, Def, SpA, SpD, Spe]
    u16 statBudget;          // base BST for current stage (300/400/500/600/680)
    u16 resonancePoints;     // bonus BST earned through battle (converts at 4 EVs : 1 RP)
    u16 resonanceCap;        // max resonancePoints for current stage (50/75/100/100/uncapped)
    u8  statCaps[6];         // per-stat max: [80..] S1 / [100..] S2 / [120..] S3 / [150..] 6th/ultimate / [255..] postgame
    u16 selectedAbilityId;   // ability ID currently in use
    u16 rememberedAbilityId; // last confirmed ability — 0 if never assigned; always available in pool even if source Pokémon leaves party
    u8  activeFormId;        // current Form preset index; updated by GetClosestForm() on stat edit and by manual Form selection
    u8  unlockedTypeFlags[3];// bitmask, 1 bit per type (18 types used of 24 bits)
    u8  padding[2];          // align to 4 bytes
} SynthData;                 // total: ~36 bytes per Synth slot — save block allocation: 6 × 36 = 216 bytes
```

---

## Species IDs

```c
// Assigned after last Gen 9 species in DPE
#define SPECIES_SYNTH           (LAST_GEN9_SPECIES + 1)
#define SPECIES_SYNTH2          (LAST_GEN9_SPECIES + 2)
#define SPECIES_SYNTH3          (LAST_GEN9_SPECIES + 3)
#define SPECIES_SYNTH3_ULTIMATE (LAST_GEN9_SPECIES + 4)  // form change only — starter Synth (synthIndex==0)
#define SPECIES_SYNTH_LEGEND    (LAST_GEN9_SPECIES + 5)  // 6th gift pseudo-legendary — standalone species, not part of evolution line
```

| Species | Internal Name | Evolves At | Notes |
|---|---|---|---|
| Stage 1 | SYNTH | Level 16 | |
| Stage 2 | SYNTH2 | Level 36 | |
| Stage 3 | SYNTH3 | — | |
| Ultimate | SYNTH3_ULTIMATE | Item (Resonance Amplifier) | Starter only. Form change, not evolution — same Pokédex entry as SYNTH3 |
| Pseudo-legendary | SYNTH_LEGEND | — | 6th gift. Standalone species — own sprite, stats, cry, Pokédex entry. Does not evolve from or into anything. synthIndex == 5 |

---

## Evolution Logic

- Uses CFRU's existing evolution screen and animation.
- If `isSynth` is set, evolve to `stage + 1` species, **not** a standard species table lookup.
- `SynthData.stage` (u8) tracks current stage (0/1/2). Stage does not increment for the ultimate form — `isUltimateForm` is the flag.
- **All SynthData fields carry forward** on evolution — types, stats, resonance points, ability pool, unlock flags all persist. Bonus resonance points are NOT reset on evolution.
- **Evolution cannot be cancelled.** Do not expose a cancel path in code or UI. It is biological, not optional.
- The ultimate form is a **form change, not an evolution** — see Ultimate Form section below.

---

## Archetype System (displayed as "Form" in UI)

In the Synth Menu, archetypes are labelled **"Form"** — the internal constant names remain `ARCHETYPE_*` in code. Forms are preset stat spreads used as **starting points** for manual editing. Selecting a Form loads its preset into the stat editor and updates the front sprite to the Form's variant sprite. Stage 1 shows only Forms 0–2. Stage 2+ shows all 6. Form selection is always available.

The active Form is tracked in `SynthData.activeFormId`. After every manual stat adjustment, `GetClosestForm()` runs and may update `activeFormId` automatically — see **Form Auto-Assignment** below.

```c
#define ARCHETYPE_PHYSICAL_SWEEPER  0
#define ARCHETYPE_SPECIAL_SWEEPER   1
#define ARCHETYPE_DUAL_ATTACKER     2
#define ARCHETYPE_DEFENSIVE_WALL    3
#define ARCHETYPE_BULKY_SWEEPER     4
#define ARCHETYPE_SUPPORT           5
```

**Stage 1 archetypes (300 BST base, per-stat max 80):**

| ID | Name | HP | Atk | Def | SpA | SpD | Spe | BST |
|---|---|---|---|---|---|---|---|---|
| 0 | Physical Attacker | 50 | 80 | 40 | 20 | 40 | 70 | 300 |
| 1 | Special Attacker | 50 | 20 | 40 | 80 | 40 | 70 | 300 |
| 2 | Balanced | 50 | 50 | 50 | 50 | 50 | 50 | 300 |

**Stage 2 archetypes (400 BST base, per-stat max 100):**

| ID | Name | HP | Atk | Def | SpA | SpD | Spe | BST |
|---|---|---|---|---|---|---|---|---|
| 0 | Physical Sweeper | 60 | 100 | 55 | 30 | 55 | 100 | 400 |
| 1 | Special Sweeper | 60 | 30 | 55 | 100 | 55 | 100 | 400 |
| 2 | Dual Attacker | 65 | 80 | 60 | 80 | 60 | 55 | 400 |
| 3 | Defensive Wall | 95 | 45 | 100 | 45 | 100 | 15 | 400 |
| 4 | Bulky Sweeper | 85 | 100 | 55 | 45 | 55 | 60 | 400 |
| 5 | Speedy Support | 80 | 35 | 80 | 35 | 80 | 90 | 400 |

**Stage 3 archetypes (500 BST base, per-stat max 120):** Scale proportionally from Stage 2 values. Exact presets TBD in Phase 2 — must respect the 120 per-stat cap.

**6th gift archetypes (600 BST base, per-stat max 150):** Scale proportionally. Exact presets TBD in Phase 2 — must respect the 150 per-stat cap.

---

## Form Auto-Assignment

Whenever the player adjusts a stat in the Synth Menu, `GetClosestForm()` runs and updates `SynthData.activeFormId` (and the displayed sprite) to whichever Form preset best matches the current stat spread.

### Distance function

```c
// Sum of absolute differences across all six stats
int FormDistance(FormPreset *form, SynthStats *current) {
    return abs(current->hp  - form->hp)
         + abs(current->atk - form->atk)
         + abs(current->def - form->def)
         + abs(current->spa - form->spa)
         + abs(current->spd - form->spd)
         + abs(current->spe - form->spe);
}
```

### Tiebreaker priority

1. **Lowest total distance** wins.
2. **Current form sticky** — if `SynthData.activeFormId` is among the tied Forms, it stays. No change.
3. **Closest Spe stat** — whichever tied Form has the smaller `|current->spe - form->spe|` wins.
4. **Lower preset index** — deterministic fallback; should never fire in practice.

### Rules
- Auto-assignment fires only during stat editing. Manually selecting a Form from the dropdown always overrides it and sets `activeFormId` directly.
- There is no "Custom" state — `activeFormId` always resolves to a valid preset index.
- `activeFormId` is initialised to `0` on Synth receipt.

---

## Stat Budget Rules

The stat editor is available from the moment the player receives their first Synth. Two pools contribute to effective stats: **base BST** (allocated freely in the editor) and **Resonance Points** (earned through battle, added automatically to the budget).

### Base BST and Per-Stat Caps by Stage

| Stage | Base BST | Per-stat min | Per-stat max | Notes |
|---|---|---|---|---|
| Stage 1 | 300 | 20 | 80 | Editor open immediately |
| Stage 2 | 400 | 20 | 100 | Cap lifts on evolution |
| Stage 3 | 500 | 20 | 120 | Cap lifts on evolution |
| 6th gift (pseudo-legendary) | 600 | 20 | 150 | Separate Synth; editor open on receipt |
| Ultimate form (starter only) | 680 | 20 | 150 | Form change via Resonance Amplifier; see below |
| Postgame (all Synths, Champion flag) | Uncapped | 20 | 255 | BST cap removed; per-stat max raised to 255 |

### Resonance Points (Battle Progression)

Synths earn EVs through battle normally. EVs convert to Resonance Points (RP) at a **4:1 ratio** — every 4 EVs gained adds 1 RP to the Synth's budget. RP is a separate bonus on top of base BST; the player distributes it freely across stats in the editor alongside base allocation.

| Stage | Max RP from battle | EVs to reach max |
|---|---|---|
| Stage 1 | 50 | 200 |
| Stage 2 | 75 | 300 |
| Stage 3 | 100 | 400 |
| 6th gift | 100 | 400 |
| Ultimate form | 100 | 400 |
| Postgame | Uncapped | — |

**Key rules:**
- RP carries forward through evolution unchanged — earning RP on a Stage 1 Synth does not reset on evolution to Stage 2. The cap simply rises.
- RP is subject to the same per-stat cap as base allocation. A Stage 1 Synth cannot exceed 80 in any stat regardless of whether the points are base or RP.
- The editor shows a single combined budget: `base BST + current RP earned`. No separate EV counter is shown — EVs are invisible once converted. The party screen or Synth summary shows a "Resonance" indicator (e.g. "Resonance: 34/50") so the player can track battle progress outside the editor.
- Standard EV vitamins (Protein, Iron, etc.) work normally and contribute to the conversion pool.
- EVs are capped at 252 per stat and 510 total per the standard GBA engine — the conversion runs off the total EVs earned, not a separate counter.

### `SynthValidateStats()`

Must be called on every edit before saving. Enforces:
- Sum of `baseStats[]` == `statBudget + resonancePoints` (or ≤ total when postgame-uncapped)
- Per-stat minimum: 20
- Per-stat maximum: from `statCaps[]` for current stage
- On postgame unlock: update `statCaps[6]` to `{255,255,255,255,255,255}` and remove budget ceiling

---

## Type System

- Synth starts with exactly **one freely chosen type** (any of 18) at game start. No type is pre-assigned.
- Secondary type is optional. Synth can remain single-typed permanently (`secondaryType = TYPE_NONE = 0xFF`).
- Types unlock by defeating the corresponding type gym leader: flag bit set in `unlockedTypeFlags[3]`.
- Starting type is the one exception — chosen before any gym, no unlock needed.
- Type changes are free and unlimited in the Synth menu, restricted only by unlock flags.
- **Primary type** drives main sprite color. **Secondary type** drives highlight/accent color.

**Unlock flag storage:** `u8 unlockedTypeFlags[3]` — 3 bytes = 24 bits, 18 used. Bit position matches type ID from `src/types.h`.

---

## Ability System

- Synth's ability pool is drawn from **party Pokémon only** (not full PC box).
- **Trace** is always present in the pool as a baseline option, regardless of party composition.
- The pool is deduplicated — duplicate abilities from multiple party members appear once.
- Stored value is the ability ID, not the source Pokémon. Ability is **copied at assignment time**; no persistent link to the source Pokémon.
- **All Synths start with Trace** as their default ability (`selectedAbilityId = ABILITY_TRACE` on receipt).
- **Remembered ability** (`rememberedAbilityId`): populated when the player confirms an ability selection in the Synth Menu. Remains in the pool even if the source Pokémon leaves the party. Hidden from the pool if never assigned (value == 0).
- Signature abilities of legendaries (e.g. Multitype, Pressure) are available if that Pokémon is in the party.

---

## Stat / Type Override Hooks

These hooks intercept normal stat and type lookup paths in battle code.

**Files to modify/create:**
- `src/synth_core.c` — all Synth logic (new file)
- `src/synth_palettes.c` — type color tables (new file)
- `include/synth.h` — shared header (new file)
- Hook `src/battle_main.c` at stat calculation: check `IsSynthMon()` before `gBaseStats[species]` lookup
- Hook `src/battle_util.c` at type effectiveness: redirect type read to `SynthData` fields

```c
// IsSynthMon checks species against all five Synth IDs
bool IsSynthMon(struct Pokemon *mon) {
    u16 species = GetMonData(mon, MON_DATA_SPECIES);
    return (species == SPECIES_SYNTH
         || species == SPECIES_SYNTH2
         || species == SPECIES_SYNTH3
         || species == SPECIES_SYNTH3_ULTIMATE
         || species == SPECIES_SYNTH_LEGEND);
}

// GetSynthBaseBudget maps species ID to base BST budget.
// Use this instead of deriving from SynthData.stage — SYNTH_LEGEND sits
// outside the 0/1/2 stage progression and must be handled explicitly.
u16 GetSynthBaseBudget(struct Pokemon *mon) {
    u16 species = GetMonData(mon, MON_DATA_SPECIES);
    switch (species) {
        case SPECIES_SYNTH:           return 300;
        case SPECIES_SYNTH2:          return 400;
        case SPECIES_SYNTH3:          return 500;
        case SPECIES_SYNTH3_ULTIMATE: return 680;
        case SPECIES_SYNTH_LEGEND:    return 600;
        default:                      return 0;
    }
}
```

---

## Ultimate Form (Silph Resonance Amplifier)

The starter Synth (`synthIndex == 0`) can undergo a one-time permanent form change into `SPECIES_SYNTH3_ULTIMATE` after the Champion is defeated. This is a postgame reward, not an evolution.

### Item

```c
#define ITEM_RESONANCE_AMPLIFIER  (assign unused item ID)
```

- Key Item. Cannot be used on any Synth other than the starter (`synthIndex == 0`). Cannot be used before `FLAG_CHAMPION_DEFEATED` is set. Cannot be used before the Synth is at Stage 3. One-time use — consumed on activation.
- Use message: *"The Amplifier resonates with [Synth name]... something fundamental is changing."*

### How to Obtain

Gifted by the rival in a postgame scene. After the player completes all 12 island summoning sites, the rival contacts the player and meets them at Pallet Town. He presents the Amplifier — a Silph prototype he retrieved from the Cinnabar Mansion — with a short scene acknowledging the journey. This ties the item reward to completing the postgame content rather than gating it behind a single fixed moment.

Script file: `scripts/resonance_amplifier_gift.pks`

### Form Change Implementation

Uses CFRU's form change system, not the evolution system. On item use:

1. Verify `synthIndex == 0`, `stage == 2` (Stage 3), `FLAG_CHAMPION_DEFEATED` set.
2. Play a brief transformation animation (reuse or adapt CFRU's form change animation).
3. Set `GetMonData(mon, MON_DATA_SPECIES)` → `SPECIES_SYNTH3_ULTIMATE`.
4. Set `SynthData.isUltimateForm = 1`.
5. Update `SynthData.statBudget` to 680 (from 500).
6. Update `SynthData.statCaps[6]` to `{150,150,150,150,150,150}` (same as 6th gift, unless postgame already set to 255).
7. Update `SynthData.resonanceCap` to 100 (unchanged from Stage 3 unless postgame).
8. Call `SynthValidateStats()` — the player's existing base stat allocation is preserved as-is. The extra 180 BST (680 − 500) is added to the budget as unallocated points, prompting the editor to open automatically after the scene so the player can spend them immediately.
9. Update sprite to `SPECIES_SYNTH3_ULTIMATE` front/back sprites via `SynthUpdatePalette()`.
10. Consume the item.

### Stats and Caps

| Property | Value |
|---|---|
| BST | 680 |
| Per-stat min | 20 |
| Per-stat max | 150 (255 if postgame-unlocked) |
| Max RP from battle | 100 (uncapped if postgame) |
| Pokédex entry | Shared with SYNTH3 — same entry, alternate form sprite |

### Sprite Requirements

Two additional sprites needed: `SPECIES_SYNTH3_ULTIMATE` front (64×64) and back (64×64). Visually distinct from Stage 3 base — more defined, more imposing. Follows the same palette slot convention as all Synth sprites so `SynthUpdatePalette()` applies correctly. See `specs/assets_and_polish.md`.

### Narrative Note

The ultimate form represents the Synth reaching the biological ceiling that Silph designed it to approach but never actually achieve — the form the prototype in the Cinnabar Mansion was theorised to reach. The rival found the theoretical data in Silph's files and built the Amplifier. It's not something Silph ever completed. Your starter Synth is the first to actually get there.

---

## Rival's Synth Stat Budgets (Fixed, Not Player-Controlled)

| Fight | Synth count | BST per Synth |
|---|---|---|
| Fight 1 (Route 1 area) | 1 | 300 |
| Fight 2 (Cerulean City) | 2 | 400 each |
| Fight 3 (SS Anne) | 3 | 400 |
| Fight 4 (Pokémon Tower) | 3 at 400 + 1 at 500 | Mixed |
| Fight 5 (Silph Co.) | 5 | 500 |
| Fight 6 (Route 22) | 5 at 500 + 1 at 600 | Mixed |
| Championship | 4 at 500 + 1 at 600 + 1 at 680 | Mixed |

---

## Decisions Log (Synth System)

All decisions final unless explicitly overridden with user confirmation.

| # | Decision |
|---|---|
| 1 | 6 Synths gifted to player total. No late-game gate on stat editor — editor open from first Synth receipt |
| 2 | Each Synth independently customizable |
| 3 | BST progression: Stage 1=300, Stage 2=400, Stage 3=500, 6th gift=600, Ultimate=680, postgame=uncapped |
| 4 | Per-stat caps scale with stage: 80 / 100 / 120 / 150 (6th gift & ultimate) / 255 (postgame) |
| 5 | Per-stat minimum is 20 across all stages |
| 6 | Resonance Points earned through battle at 4 EVs : 1 RP. Max RP: 50 (S1) / 75 (S2) / 100 (S3+). Uncapped postgame |
| 7 | RP carries forward through evolution unchanged — no reset on stage change |
| 8 | Editor shows one combined budget (base BST + current RP). No separate EV counter in editor |
| 9 | Postgame stat editor: BST cap removed, per-stat max 255, per-stat min 20 |
| 10 | Ability pool: party only (not full PC box). Trace always available. All Synths default to Trace on receipt |
| 11 | Remembered ability stored per Synth; populated on confirm in Synth Menu; stays in pool after source Pokémon leaves party; hidden if never assigned |
| 12 | Types unlock via gym defeats; starting type freely chosen at game start without needing a gym badge |
| 13 | Locked types are hidden from the type dropdown entirely — not shown as greyed out |
| 14 | Type 2 dropdown always shows "None" at the top; selecting it clears the secondary type |
| 15 | Synth can be single or dual typed — secondary optional |
| 16 | Stage 1: 3 Forms. Stage 2+: all 6. Forms are starting points for manual editing, not locks |
| 17 | Archetypes are labelled "Form" in the Synth Menu UI |
| 18 | Form auto-assignment: GetClosestForm() runs after every stat adjustment using summed absolute stat differences. Tiebreaker: current form sticky → closest Spe → lower preset index |
| 19 | No "Custom" Form state — activeFormId always resolves to a named preset |
| 20 | activeFormId stored in SynthData (u8); initialised to 0 on receipt |
| 21 | No cancelling Synth evolution |
| 22 | Synth Menu not accessible during battle |
| 23 | Ultimate form is a form change (not evolution) via Resonance Amplifier item — starter Synth (synthIndex==0) only |
| 24 | Ultimate form: 680 BST, 150 per-stat cap, same Pokédex entry as Stage 3 |
| 25 | Resonance Amplifier gifted by rival after all 12 island summoning sites completed |
| 26 | On ultimate form activation: existing stat allocation preserved, 180 bonus BST added as unallocated, editor opens immediately |
| 27 | 6th gift pseudo-legendary is its own species (SYNTH_LEGEND) — own sprite, stats, cry, Pokédex entry. Does not share species with the three-stage line. synthIndex == 5 |
| 28 | Save block: SynthData is 36 bytes per slot (added rememberedAbilityId u16 + activeFormId u8 + adjusted padding). Total: 6 × 36 = 216 bytes |
