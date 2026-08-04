# specs/assets_and_polish.md
# Assets & Polish — Title Screen, Credits, Sprite Resources, Overworld Sprites

Read this spec for art integration and polish sessions.

---

## Sprite Resources

### Standard Pokémon (Do NOT generate AI sprites for these)

| Source | Coverage | Notes |
|---|---|---|
| [pokedex-data by Touched](https://github.com/Touched/pokedex-data) | Gen 1–7 sprites + data | Pre-formatted 64×256 PNGs (front/shiny-front/back/shiny-back), cries, species data ready for DPE insertion |
| [DS-style Gen VII+ Sprite Repository](https://www.pokecommunity.com/threads/ds-style-gen-vii-and-beyond-pok%C3%A9mon-sprite-repository-in-64x64.368703/) | Gen 7–9 battle sprites + icons | PokéCommunity thread, Dropbox updated April 2026. Covers full Gen 9 including DLC |
| WAH forum (whackahack.com) | Gen 9 overflow | Additional Gen 9 sprites not yet QC'd into main repo |

Always credit sprite contributors in the project's `Credits.txt`.

### Synth Sprites (AI-generated + manual cleanup)

Each stage of the main line has one sprite per Form. SYNTH_LEGEND and SYNTH3_ULTIMATE are standalone designs with no per-Form variants.

| Group | Species | Count |
|---|---|---|
| Stage 1 (3 Forms) | SYNTH | 3 front + 3 back |
| Stage 2 (6 Forms) | SYNTH2 | 6 front + 6 back |
| Stage 3 (6 Forms) | SYNTH3 | 6 front + 6 back |
| Pseudo-legendary | SYNTH_LEGEND | 1 front + 1 back |
| Ultimate form | SYNTH3_ULTIMATE | 1 front + 1 back |
| **Total** | | **38 sprite assets** |

Each species also requires a **32×32 icon sprite** (party/box UI) — 5 icons total (one per species; Form variants within a stage share that stage's icon). Icons are produced after battle sprites are locked.

Each sprite uses a 16-color indexed GBA palette. Palette slot convention is in `specs/synth_menu.md`.

---

## Pokédex Scope

Full National Dex through Gen 9, including:
- All regional forms (Galarian, Hisuian, Paldean)
- All Paradox Pokémon
- Gen 9 DLC Pokémon (Teal Mask + Indigo Disk)
- Synth species: SYNTH, SYNTH2, SYNTH3, SYNTH3_ULTIMATE, SYNTH_LEGEND (5 entries at end of dex)

Battle engine is already Gen 8 via CFRU. Verify Gen 9 moves and abilities are added via DPE data tables.

---

## Overworld Sprites Needed (Phase 4)

GBA overworld sprites are small (16×32 or 16×16 px). Required for Cerulean Cave scripted encounters.

| Pokémon | Priority | Notes |
|---|---|---|
| Mew | Check repos first | Small, Gen 1 — likely exists in community resources |
| Deoxys | Check repos first | Use Normal Forme — may exist in community resources |
| Genesect | Likely needs creation | Mechanical design — less common in repos |
| Arceus | Likely needs creation | Large — may need scaling down |

Check Fire Red hack community resources before creating. Claude Code can generate placeholder sprites for testing if needed.

---

## Title Screen (Phase 3)

Default Fire Red title screen (Charizard + logo) must be fully replaced.

### Required Assets (user produces via AI art + cleanup)

| Asset | Description |
|---|---|
| **Title logo** | "Pokémon SYNTH" in a custom typeface — clean, slightly futuristic sans-serif with circuit/DNA motif. Should feel like a Silph Co. product name. |
| **Title screen background** | Abstract/atmospheric art — candidate: silhouette of Synth (Stage 3) against the four source legendaries' DNA helices; or a clean dark laboratory aesthetic with the logo centred. |
| **Title screen Pokémon sprite** | Animated Synth (Stage 3) idle cycle — replaces Charizard's animated sprite. |

### Implementation (Claude Code)

- Hook into CFRU's title screen rendering code to replace the background tilemap, logo graphics, and animated sprite.
- Title screen palette must fit within GBA constraints: 16-color indexed per layer.
- Animate Synth sprite using CFRU's existing sprite animation framework.
- "Press Start" prompt unchanged.

**Workflow:** User generates and cleans up art → converts to GBA-compatible format (4bpp indexed PNG → bin via tools) → Claude Code integrates into title screen code.

**File:** `src/title_screen.c`

Graphics inputs:
```
graphics/
  title_bg.png           — user-produced, 4bpp indexed
  title_logo.png         — user-produced
  synth_title_sprite.png — user-produced (animated idle cycle)
```

---

## Credits Screen (Phase 4)

End credits must be updated to properly credit all contributors. Claude Code will:
1. Audit the existing Fire Red / CFRU credits sequence.
2. Replace or append a new credits roll.

### Required Credits Entries

| Role | Credit |
|---|---|
| Engine base | Skeli789 — Complete Fire Red Upgrade (CFRU) |
| Pokémon Expansion | Skeli789 — Dynamic Pokémon Expansion |
| Gen 7–9 sprites | DS-style Gen VII+ Sprite Repository contributors (PokéCommunity) — KingOfthe-X-Roads, QDLym, AxelLoquondo, Lykeron, and all thread contributors |
| Gen 1–7 data | Touched — pokedex-data repository |
| Game design & development | [User name / studio name — TBD] |
| Synth character art | [Artist credit — TBD] |

> **Before finalizing credits:** Audit CFRU and DPE repos' own `Credits.md` files — add anyone credited there not already listed. Also check the DS-style sprite thread's Dropbox readme for a full contributor list at time of completion.

**Implementation:** Rewrite the credits script to display the new roll, adjusting timing and text for additional entries. If the credits system is too limited, implement as a custom scrolling text screen triggered post-Champion.

**File:** `src/credits_screen.c`
