# Pokémon SYNTH — Engineering Log & Handoff

**Last updated:** 2026-07-21 (slice session 1)
**Workspace:** `~/ventures/005-pokemon-synth/`
**Design bible:** `~/Downloads/Pokemon Synth/Claude/` (9 spec files — `claude.md` there is the project guide)
**Playable ROM:** `~/ventures/005-pokemon-synth/synth-base.gba` (open in mGBA)

---

## Engine decision

**CFRU (master) + DPE (Unbound branch)** on a source-built vanilla Fire Red base.

- Deciding criterion was "all generations": CFRU+DPE ships Gen 1–8 **including Hisui/Legends Arceus mons** (that's the Unbound branch's contribution). Gen 9 (~120 species) remains as a scripted-import project — sprites exist in community repos; the design needs Gen 9 *species*, not Gen 9 *mechanics*.
- FRLG-Plus (pokefirered decomp) was evaluated and builds/boots on this machine (`FRLG-Plus/`) — kept as reference, rejected as base because it's Gen 3 engine/dex only.
- The base ROM is **compiled from source** via pret's `pokefirered` decomp (`pokefirered/`) — byte-identical to retail BPRE v1.0 (sha1 `41cb23d8dccc8ebd7c649cd8fbb58eeace6e2fdc`). No cartridge dump needed, no downloads.

## Build protocol (memorize this)

```sh
cd ~/ventures/005-pokemon-synth && source env.sh   # devkitARM + tools + local bin on PATH

# Order is DPE first, then CFRU on DPE's output:
cd DPE  && rm -f generatedrepoints && python3 scripts/make.py          # BPRE0.gba -> test.gba
cd ../CFRU && rm -f generatedrepoints && cp ../DPE/test.gba BPRE0.gba \
        && python3 scripts/make.py                                     # -> test.gba
cp test.gba ../synth-base.gba
```

- `DPE/BPRE0.gba` = copy of `pokefirered/pokefirered.gba` (the source-built vanilla ROM).
- After **any header or config change** in CFRU or DPE: `rm -rf build` in that repo — the incremental build silently reuses objects compiled with stale `NUM_SPECIES`/flags.
- **Always `rm generatedrepoints` before insertion.** It's a repoint cache; stale or dummy-ROM-poisoned caches carpet-bomb the output ROM (see Gotchas).
- Verify every build: `xxd -s 0xA0 -l 12 test.gba` must read `POKEMON FIRE`.

## State after slice session 1

| Piece | Status |
|---|---|
| SPECIES_SYNTH (0x50E, dex #906, sprite idx 1260) | ✅ registered, in-game verified |
| SPECIES_SYNTH2 (0x50F, dex #907, idx 1261) | ✅ registered; untested in-game (evolution at Lv16) |
| Sprites | PixelLab teal dino = SYNTH, mane-fox = SYNTH2; 64×64 4bpp, shared 15-color palette; shinies are hue-shifts (purple). Sources: `~/Downloads/Pokemon Synth/Art/Pixel Lab Sprites/` |
| Starter | All 3 lab balls + `sStarterSpecies` (0x3F5D2C) give SYNTH — patched via CFRU `bytereplacement` |
| Base stats | Placeholder: 300 BST flat 50s, Normal-type, ability **Trace** (per spec), genderless, no-breed. Runtime override engine comes later |
| Learnset | Scratch/Growl @1, Quick Atk @7, Bite @12, Slash @18, Crunch @25; SYNTH2 shares it |
| Evolution | SYNTH → SYNTH2 at Lv16; cannot-be-cancelled ENFORCED 2026-08-18 (spec rule #4, `SynthUncancellableEvoHook`; byte-verified, in-game QA pending) |
| Cry | Borrowed (Grookey's) — placeholder |
| Icon | Placeholder; shared-palette mapping means colors may render off |

**Open verification item — CLOSED 2026-08-17 (byte-level, no code change needed).** Player's Synth
showed nameplate "Mon". **Verdict: user-entered nickname. The species-name table is correct.** Proof:
`gSpeciesNames` is repointed to `0x0965C268` (pointer stored at ROM `0x144`), file offset `0x165C268`,
stride 11 (`POKEMON_NAME_LENGTH + 1`). Entry `0x50E` = `cd ed e2 e8 dc ff` = "Synth", entry `0x50F` =
`cd ed e2 e8 dc a3 ff` = "Synth2". Anchors confirm indexing end-to-end: idx 0 = "??????",
idx 1 = "Bulbasaur", idx `0x509`–`0x50D` = Alcremie/Copperajah/Duraludon/Urshifu/Urshifu — matching
`strings/Pokemon_Name_Table.string`'s tail entry-for-entry. **Decisive:** across all 1296 entries no
entry equals "Mon"; the only "Mon*" entry is `0x1BC` "Monferno". A misindexed table read still yields a
complete terminated entry, so no indexing error can produce "Mon" — it had to be typed. (Nicknames are
stamped into `BoxPokemon` at creation and the nameplate reads the *stored* nickname, so a table bug at
creation time was the real competing hypothesis; it is ruled out by the above.)
Reproduce with `docs/name_table_scan.py` (see its docstring).
Note: `synth-base.sav` is 131072 bytes of `0xFF` — a blank save. The original "Mon" mon is not
recoverable from the on-disk artifacts; the argument above is from the ROM, which is stronger anyway.

## Gotchas (each cost real time — do not relearn)

1. **`generatedrepoints` cache poisoning.** DPE/CFRU insertion caches repoint locations in `generatedrepoints` at repo root. Running insertion against a dummy/zero ROM filled it with ~8M bogus offsets; the next real build stamped one pointer value over the entire ROM (header destroyed, mGBA refuses to load). Fix/prevention: delete the file before insertions; never run insertion against a fake ROM.
2. **`bytereplacement` ends inside an unclosed `#ifdef UNBOUND`.** Anything appended at the tail is silently skipped in our build. Insert patches **above** that block, and always verify the actual bytes in the output ROM (`xxd -s <offset>`), never trust a clean insert log.
3a. **Dual-ownership audit COMPLETE (2026-07-21, agent-run).** Full inventory: only TWO dual-ownership points exist. `gLevelUpLearnsets` (EXPAND_MOVESETS — off, fixed) and `gBaseExpBySpecies` (GEN_7_BASE_EXP_YIELD — ON; CFRU's exp table ended before SYNTH → OOB read on defeating a SYNTH; fixed by adding SYNTH=60/SYNTH2=140 entries to `CFRU/src/Tables/experience_tables.c`, ships next build). Every other per-species table (base stats, evos, names, pics, palettes, icons, cries, dex, TM/tutor, egg moves) reads through vanilla pointers that DPE repoints — DPE wins, SYNTH covered. When adding future species: touch DPE's tables + CFRU's species.h + these TWO CFRU tables.
3b. **CFRU/DPE dual-ownership tables (original incident).** CFRU `src/config.h` `#define EXPAND_MOVESETS` made CFRU link its **own** learnset table, shadowing DPE's (symptom: SYNTH had zero moves while DPE's data sat correct in the ROM). Commented out per the flag's own comment. ⚠️ **Audit for other dual-ownership tables before building the stat-override engine** — this is the #1 slice risk (gBaseStats read sites) showing up in real life.
4. **Branch pairing:** DPE **must** be on the `Unbound` branch to match CFRU master's species numbering (master diverges at 0x397; Urshifu-R-Giga is 0x4F3 vs 0x50D). DPE master + CFRU master silently corrupts all species data ≥0x397. `DPE/scripts/make.py` ROM_NAME was changed from "Pokemon Unbound.gba" to "BPRE0.gba"; insert offset on this branch is 0x1650000.
5. **DPE table files can contain multiple arrays** (Unbound branch especially). Never append entries with a blind "insert before last `};`" — verify which array the insertion landed in (Pokedex_Data_Table.c has trailing `gAlternateDex*` tables that ate our dex entries).
6. **Moves are stamped at mon creation.** Fixing a learnset does not retroactively give an existing save's mon its moves — New Game (or level-up/relearner) required.
7. **Battle sprite orientation:** front sprites face left (toward player), back sprites face right (toward enemy). PixelLab sheet poses mostly face left — back-sprite tiles need a horizontal flip.
8. **Icons are 32×64** (two stacked 32×32 animation frames), 4bpp uncompressed, and use one of 3 **shared** palettes (`gMonIconPaletteIndices`) — the PNG's own palette is ignored at runtime. **A new icon MUST be authored in the shared palette's exact index layout** or it renders as rainbow garbage in the party menu (QA run 1 finding). Protocol: take a donor icon PNG with the same palette index (Squirtle=pal 0, Grookey=pal 1), nearest-map every color to the donor's 16 slots (never slot 0 = transparent), save with the donor's palette verbatim. Done for SYNTH (pal 0) + SYNTH2 (pal 1), 2026-07-21.
9. **Back-sprite PNG doubles as the shiny palette source** (`gBackShinySprite*` = back tiles + shiny palette). Front PNG's palette = normal palette. They must share index layout.
10. **`scripts/tm_tutor.py` hardcodes `SPECIES_COUNT`** — does not read `NUM_SPECIES`; bump manually when adding species (done: now `0x50F + 1`).
11. **`ldr rX, =<small constant>` in CFRU hook asm can silently emit Thumb-2 `movw`** — invalid on the GBA's ARM7TDMI (executes as a stray BL-prefix + random store). devkitARM gas isn't pinned to armv4t in this build; the `ldr=` pseudo only uses a literal pool when the constant *can't* be a `movw` immediate (which is why every `=0x8xxxxxx | 1` address in the tree assembles fine and a small constant like `=0x50F` doesn't). Build small constants with `mov`/`lsl`/`add` instead, and byte-verify any new routine's encoding in the output ROM (caught 2026-08-18, N-04, SynthUncancellableEvoHook).
12. **Adding ANY code to CFRU shifts the whole inserted blob** — expect a ~1MB diff at/above 0x900000 plus ~1KB of ≤4-byte pointer-word fixups at vanilla hook sites. That is NOT Gotcha-1 carpet-bombing. To tell them apart: revert the change, `mv generatedrepoints` away, rebuild — a byte-identical ROM proves the chain is deterministic and the big diff is legit relink shift (control-run technique, N-04 2026-08-18).

## Toolchain facts

- devkitARM 16.1.0 native arm64 (no Rosetta), via devkitPro pacman. mGBA 0.10.5 (brew cask).
- `wav2agb` + `mid2agb` built from source in `bin/` (mid2agb came from the pokeemerald-mac checkout).
- agbcc built + installed into both decomp checkouts (`agbcc/`).
- Python 3.14 works for all repos' build scripts despite "downgrade to 3.7" warnings — the one real 3.x hazard was behavioral, not syntax (see Gotcha 1 for what actually bit).
- CFRU insert offset 0x900000; DPE (Unbound) 0x1650000; no collision.

## Art pipeline division of labor (settled by experiment, 2026-07-21)

- **PixelLab (creator): 64×64 battle sprite origination ONLY** — the 38-sprite Form set. This is now the sole external art dependency.
- **In house (Claude): everything at 32×32 and below + all derivation/compliance** — palettes, shinies, type recolors, icon idle animations (classic 1px-bounce frame 2 shipped), overworld sprites (16×16 hand-authored dino passed eyeball QA in 2 iterations), sheet slicing, format packaging.
- Icon base art TODO: hand-draw the 32×32 (current one is a muddy downscale — the experiment showed authored beats downscaled at small sizes).
- Method for authored pixel art: palette-letter grid in Python → render → Read the PNG → iterate. 2–3 rounds typical.

## Repeatable build/QA loop

Skill: **`/synth-build`** — full chain build + verification + mGBA launch (+ optional fresh save). QA findings go into this doc's state table + Gotchas after each run.

## Slice acceptance test (creator's QA checklist, 2026-07-21)

1. Starter has the Synth Menu ← **not built yet** (next mountain)
2. A battle works ← passing as of session 1
3. Menu pieces functional: stat editor / type selector / ability selector ← **not built yet**; build order: SynthData+save block → menu shell from party screen → stat editor → type selector → ability selector

QA run 1 result: species name SYNTH ✅ ("Mon" was a nickname — **now proven at byte level, 2026-08-17**; see the closed verification item above), battle ✅, icon anim ✅ (art fixed via shared-palette remap).

**Slice session 2 (2026-07-21): Synth Menu MVP shipped.**
- `CFRU/include/new/synth.h` + `src/synth_core.c`: SynthData (40B×6), personality-keyed, lazy-allocated on first menu open; SynthValidateStats with clamp + overspend clawback.
- **Save storage:** carved former PC Box 22's region — `gSynthSaveData` @ 0x203D8DC (240B used, 0x6CC reserved; see ram_locs.h). TOTAL_BOXES_COUNT 25→24 (3 pointer tables edited in pokemon_storage_system.c). Persists via flash sectors 30/31 automatically; wiped on New Game automatically. Caveat: NOT persisted by link saves (sector 30/31 skipped there).
- **Party menu:** "Synth" action (MENU_SYNTH=60) appears after Summary for Synth species only; strings in `strings/synth.string`.
- **`src/synth_menu.c`:** fullscreen stat editor (DexNav-skeleton): d-pad row select, left/right adjust, live budget line (used/total), caps + budget enforced, B exits. MVP notes: exits to start menu (not back into party menu); edits SynthData only — **battle stats not yet overridden** (hooks are next); menu opened via replaced CB2 leaks party-menu heap allocs (benign short-term).
- Audit fix shipped: gBaseExpBySpecies SYNTH entries.

## Stat override engine — WIRED 2026-08-12

`SynthGetBaseStat` was written in slice session 2 and left with **zero callers**. Now wired:

- **The narrow waist is `CalculateMonStatsNew` (`src/build_pokemon.c`), not 109 call sites.** CFRU already hooks it over vanilla `CalculateMonStats` (`hooks:333` → `803E47C`), and every *displayed and battle* stat flows through it. Two wraps done there: `baseHP` (~L4459) and the normal per-stat loop (~L4518). The ~107 other `gBaseStats[]` reads are direct-base-stat consumers (AI damage prediction, exp yield, catch rate, dex display) — refinements, not blockers.
- **Deliberately NOT wired** into the Scalemons / AverageMons / 350 Cup branches — those formats normalize base stats on purpose.
- **`SynthGetBaseStat` had a latent index bug, fixed the same session.** `SynthData->baseStats` is authored in MENU order `[HP,Atk,Def,SpA,SpD,Spe]`; the engine indexes in CANONICAL order `[HP,Atk,Def,Spe,SpA,SpD]` (the `STAT_*` enum and `struct BaseStats` field layout both). They agree on 0-2 and disagree on 3-5, and the function indexed straight through — so wiring it as-written would have fed the Speed slot into Sp. Atk. Fixed with `sStatIndexToSynthSlot[6] = {0,1,2,5,3,4}` in synth_core.c; callers pass a natural `STAT_*` index. **Never "fix" this by reordering `SynthData->baseStats`** — that array is persisted in the save block, so reordering silently corrupts every existing Synth mon.

**GOTCHA (same class as "moves are stamped at mon creation"): stats only recompute when `CalculateMonStatsNew` runs** — level-up, evolution, a few build paths. Editing sliders alone leaves the summary screen showing pre-edit numbers, which reads exactly like a broken hook. `Task_SynthMenuInput`'s B-exit now calls `CalculateMonStatsNew(sSynthMenuMon)` before clearing the pointer. Any future path that mutates SynthData must do the same.

## Next sessions (vertical slice order)

1. ~~**Audit dual-ownership tables**~~ — done 2026-07-21 (see Gotcha 3a).
2. ~~**SynthData struct + save block**~~ — done slice session 2.
3. ~~Stat override hooks~~ — done 2026-08-12 (above). **Type overrides still unwired**: `SynthGetType` has zero callers; the read sites are `gBaseStats[species].type1/type2` (~L2575 build_pokemon.c, damage_calc.c, and the Camomons paths, which are the closest existing analogue to what Synth needs).
4. 18-type starter picker in Oak's lab script (CFRU scrolling multichoice).
5. Synth menu (3-tab editor per `specs/synth_menu.md`).
6. Rival fight 1 with fixed-budget Synth.
7. ~~Uncancellable evolution enforcement (spec decision #21)~~ — **DONE 2026-08-18 (N-04, unattended).**
   Implemented as scoped below, but via a CFRU hook rather than a bytereplacement: `SynthUncancellableEvoHook`
   (`CFRU/assembly/hooks/general_hooks.s` tail + `hooks` entry `08015AE4 0`) intercepts `TryEvolvePokemon`'s
   argument setup for `EvolutionScene(mon, species, 0x81, i)` (call site found at `0x08015AEE` by ROM scan —
   statics aren't in pokefirered.map) and passes `0x80` (bit 0 = TASK_BIT_CAN_STOP cleared) when
   postEvoSpecies == SPECIES_SYNTH2, `0x81` otherwise. Battle level-up path only — the Rare Candy path
   (`CFRU/src/party_menu.c:1789`) already passes `canStopEvo=FALSE`, and no stone evolves SYNTH.
   Byte-verified in output ROM incl. full routine disassembly. **See new Gotchas 11 and 12** — both were hit
   implementing this. In-game B-button QA still pending (needs a Lv15→16 SYNTH; Harrison's step).
   Original scoping note kept below for reference. Scoped 2026-08-17:
   The cancel gate is NOT the B-button test itself. `pokefirered/src/evolution_scene.c:653` reads
   `gMain.heldKeys == B_BUTTON && ... && gTasks[taskId].tBits & TASK_BIT_CAN_STOP` — so cancellation is
   already opt-in per invocation via `TASK_BIT_CAN_STOP` (`1 << 0`, L169). **Do not patch the B-button
   check** — that is global and would make every evolution in the game uncancellable. Patch the caller
   instead: the post-battle level-up path is `pokefirered/src/battle_main.c:3900`,
   `EvolutionScene(&gPlayerParty[i], species, 0x81, i)` — `0x81` has bit 0 set (= can stop). Clearing
   bit 0 *when the mon's species is SYNTH* is the species-scoped fix rule #4 wants. (Other callers, for
   reference: `party_menu.c:5175` passes TRUE — the item/evo-stone path; `pokemon.c:4409` passes FALSE.)
   Confirm whether CFRU overrides this call site before patching; if not, it needs a `bytereplacement`
   patch — and per Gotcha 2 it must go **above** the unclosed `#ifdef UNBOUND`, not at the file tail.

## QA bug log

- **Synth Menu black screen / crash (QA runs 2-3) — ROOT CAUSE FOUND via diagnostic-backdrop build:** `InitWindows` fails (red backdrop) when the Synth screen is launched directly from a `CursorCb` while the party menu is still alive — its heap allocations starve the window system, and (pre-guards) drawing through invalid windows corrupted memory → "Jumped to invalid address" on next input. **Fix: never `SetMainCallback2` from a party-menu cursor callback — use the official teardown: `sPartyMenuInternal->exitCallback = CB2_YourScreen; Task_ClosePartyMenu(taskId);`** (pattern from CursorCb_Summary). Diagnostic-backdrop technique worth reusing: backdrop palette writes work even when windows fail — use color as a status channel QA can report.
- **Cosmetic, open:** party menu bottom-left message strip shows garbled glyphs when the action menu is open (see QA screenshot 2026-07-21) — possibly pre-existing CFRU quirk, possibly fallout from our menu string; check before/after removing MENU_SYNTH.
- **DexNav ungate: DELIBERATELY RETAINED (2026-08-11, Harrison's call).** The two `//SYNTH QA` edits in `CFRU/src/start_menu.c` (~L149 `CanSetUpSecondaryStartMenu`, ~L226 `BuildPokeToolsMenu`) stay in for now as a **rendering canary** — DexNav is the known-good screen, so if it ever renders wrong, the window/VRAM pipeline broke tree-wide rather than in our screen. Still a release blocker: restore both flag checks (`FLAG_SYS_DEXNAV`, `FLAG_SYS_POKEDEX_GET`) before shipping.
- **Removed 2026-08-11:** `CB2_SynthMenuBootDebug` deleted from `synth_menu.c`. Its hook was already unwired, so it was dead code, but it was the one line standing between the repo and a debug-boot release build. Re-add from git history (`6e66e5f`) if a boot-straight-to-menu loop is needed again.
- **Process rule: never drive the game via OS keystroke automation (osascript System Events) while Harrison is using the machine** — keystrokes land in whatever window has focus (nearly disrupted an unrelated terminal session). Future self-serve driving: mGBA's Lua scripting (GUI Tools menu — injects into the emu core, no OS focus) or the GDB stub. Until then, in-game verification is Harrison's step.

## Synth Menu rendering bug — RESOLVED 2026-08-04 ✅

**ROOT CAUSE: template array alignment.** `sSynthWinTemplates` landed at a 2-aligned ROM address (0x089DB872). Vanilla `InitWindows` copies WindowTemplates with 32-bit word loads, and **ARM7TDMI rotates unaligned LDRs by 16 bits** — every field scrambled (GDB-verified byte-for-byte: baseBlock became 0x1C00 → tile DMA landed past VRAM's end; top became 99 → map writes outside the tilemap; DMA queue showed 6 requests queued AND processed — destinations were garbage). `struct WindowTemplate`'s natural alignment is only 2 (u8s + u16), so layout luck decides — DexNav's array happened to be 4-aligned, which is why it worked and why weeks of bisects produced chaotic symptom shifts (each edit re-rolled the layout lottery).

**Fix:** `__attribute__((aligned(4)))` on `sSynthWinTemplates` (synth_menu.c). **RULE: every `struct WindowTemplate` array in CFRU src MUST carry `__attribute__((aligned(4)))`.** Verify after build: `arm-none-eabi-nm build/linked.o | grep WinTemplates` — address must end in 0/4/8/C.

**RULE NOW ENFORCED TREE-WIDE (2026-08-11).** The rule had only ever been applied to the file where the bug bit; an audit found five other `WindowTemplate` arrays with no attribute, all 4-aligned *by luck* at the time (any layout-shifting edit re-rolls that). Attribute added to all of them: `src/frontier_records.c` (`sFrontierRecordsWinTemplates`), `src/raid_intro.c` (`sRaidBattleIntroWinTemplates`), `include/new/dexnav_data.h` (`sDexNavWinTemplates`), `include/move_reminder.h` (`sMoveRelearnerWindowTemplates`), `include/new/move_reminder_data.h` (`sMoveRelearnerExpandedTemplates`). When adding any new screen, the attribute goes on at declaration time — do not rely on the nm check catching it.

Verified rendering (debug-boot screenshot): title, budget line, six stat rows with selection highlight, help bar, teal backdrop. Menu shipped on the DexNav chassis (see divergence list above) — the chassis rewrite was likely not strictly necessary for the fix but is kept: it matches the known-good screen pattern.

**REAL-PATH QA CONFIRMED (Harrison, pre-2026-08-11):** menu opens correctly on a real Synth mon via party menu → Synth. This is the route the debug-boot hook bypassed, so the party-menu teardown fix (`exitCallback` + `Task_ClosePartyMenu`) is proven too, not just the renderer. **Nothing beyond "it loads" has been exercised yet** — d-pad row select, left/right stat adjust, budget clamp/clawback, cap enforcement, and B-exit are all still untested. That is the next QA pass.

Debug loop notes for next time: boot hook = `CB2_SynthMenuBootDebug 80EC820 0` in CFRU `hooks` (80EC820 = CB2_InitCopyrightScreenAfterBootup; REMOVE before release build — currently removed). mGBA GDB stub: `mGBA -g` + `arm-none-eabi-gdb -batch`; use **hbreak** (sw breaks in ROM don't stick), one fresh mGBA per gdb session (stub can't re-attach after detach). Useful fixed addrs: gWindows 0x020204B4, dma3 queue 0x030000C8 (locked/cursor 0x030008C8/C9), bg configs 0x030008D0.

## Synth Menu rendering bug — investigation state (2026-07-21, superseded by fix above)

Solo debug loop established: `CB2_SynthMenuBootDebug` hook (hooks file, currently REMOVED from release; re-add to boot straight into the menu with dummy data — scratchpad ROM `synth-menudebug.gba`). GDB stub works: `mGBA -g` + `arm-none-eabi-gdb -batch` with breakpoints from offsets.ini (addresses shift every build!).

**Established facts (GDB-verified):** DISPCNT/BG0CNT correct; palettes+backdrop path works (VBlank alive); windows allocate correctly (gWindows populated, buffers valid); window DRAWS complete; but window tile+map data NEVER reaches VRAM (0x600xxxx empty). Raw CPU writes to VRAM DO display (full-map fill = white screen ✓). Scroll ruled out. COPYWIN_BOTH=3 correct. CFRU's BPRE.ld addresses verified against pokefirered.map — all correct.

**Contradiction unresolved:** every precondition in the decomp source passes, yet CopyWindowToVram's transfers vanish. DexNav-chassis clone made InitWindows FAIL outright (red) — worse, and bisects (bg3 buffer, multi-bg windows) didn't isolate it. **Next test (needs Harrison, 1 click): open DexNav from the start menu in the real game — if DexNav's GUI is ALSO blank/broken in OUR build, the problem is systemic to our build config, not synth_menu.c.** Suspects then: build flags, insertion clobbering vanilla code near the window/DMA system, or the CFRU master branch itself.

Debug-loop lessons: backdrop-color diagnostics work great; solo debug ROM + screenshot ≈ 40s/iteration; `pkill -f mGBA` kills Harrison's session too — use targeted PIDs.

**Session 2 continued — STILL UNRESOLVED after ~10 more iterations. New verified facts:**
- Found + fixed a REAL latent bug (not the renderer): CFRU's linker.ld places .bss/COMMON in ROM, so ALL mutable `static` vars in new .c files are silently non-functional. Moved menu state to fixed EWRAM (`gSynthMenuState` @ 0x203D9CC, in the carved Box-22 region). **RULE: never use a mutable static in CFRU src — use a fixed address in ram_locs.h/synth.h.** (This did NOT fix rendering but was a genuine bug that would have caused chaos later.)
- Direct CPU blit of window tileData→VRAM (same addresses the raw-probe white-screen used) ALSO doesn't display. So: raw full-map probe shows white ✓, but per-window tile+map writes don't. Contradiction still unexplained. gWindows[0].tileData is a valid EWRAM ptr (0x2000820) per GDB.
- **STOPPED iterating — thrash without a model. NEXT SESSION MUST START WITH THE DEXNAV DATAPOINT (never actually obtained):** open DexNav in the real game (Start menu / PokéTools). Blank→systemic build problem (compare vs stock CFRU, suspect our insertions/flags). Works→isolate what synth_menu does differently, possibly rebuild against a hello-world CFRU custom-screen that's known to work. Consider: ask CFRU community/docs directly, or diff a minimal working CFRU fullscreen example.
- Current release build: menu opens to known-broken teal, B exits cleanly, no corruption. Everything else (species, sprites, starter, save carve, party option) works.
- **2026-08-04: DexNav UNGATED for the datapoint.** `CFRU/src/start_menu.c` — both flag checks (`FLAG_SYS_DEXNAV && FLAG_SYS_POKEDEX_GET`) removed at the two sites (`CanSetUpSecondaryStartMenu`, `BuildPokeToolsMenu`), marked `//SYNTH QA`. DexNav now appears in Start→PokéTools from game start, shipped in current synth-base.gba. REVERT before release (restore both flag checks). **DATAPOINT OBTAINED (2026-08-04, Harrison QA): DexNav GUI RENDERS CORRECTLY in our build.** → The window/VRAM pipeline is healthy; the renderer bug is specific to synth_menu.c's screen setup, NOT systemic.
- **Chassis clone SHIPPED same session (awaiting QA):** synth_menu.c rebuilt as a verbatim CB2_DexNav clone. Divergences found in the diff and fixed: (1) partial GPU reset — homemade ClearVramOamPlttRegs only reset BG0; now full DexNav version (all 4 BG CNT+scroll regs); (2) **windows were on bg0/SBG_TEXTBOX, which DexNav never shows at init — moved to SBG_TEXT (bg2)**; (3) all 4 BGs now inited from templates (was bg0-only) + Calloc'd BG_BACKGROUND tilemap buffer, freed after copy, ptr carried in gSynthMenuState->tilemapPtr (synth.h struct grew 4B); (4) added DexNav's free_temp_tile_data_buffers_if_possible wait-state before ShowBg; (5) **CommitSynthWindow = PutWindowTilemap + CopyWindowToVram(COPYWIN_BOTH)** (canonical order; old direct-VRAM-blit hack deleted — the old code NEVER called PutWindowTilemap, likely the core bug: bg tilemap buffer stayed empty so the map in VRAM pointed every cell at tile 0); (6) palette fade-in via Task_SynthMenuFadeIn (input task starts after fade). QA: party menu → Synth on the starter; expect title/stats/help windows on dark teal. Red backdrop still = InitWindows failed.

## Verification protocol

Build → header check (`POKEMON FIRE` at 0xA0) → patched-byte spot-checks → boot in mGBA → **play the actual path** (title → New Game → lab → ball → battle). "Inserted OK" proves nothing; screenshots or it didn't happen.
