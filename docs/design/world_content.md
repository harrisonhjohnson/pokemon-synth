# specs/world_content.md
# World Content — Encounters, Trainers, Items, TMs, Shops, Fossils, Starters

Read this spec for content population sessions (Phase 3).

---

## Wild Encounter Tables

All 89 Fire Red encounter tables (routes, caves, water, fishing) must be updated to distribute Gen 1–9 Pokémon.

**Workflow:**
1. Claude Code audits all existing encounter tables and exports a draft CSV / markdown table.
2. User reviews and annotates which later-gen mons fit which zones thematically.
3. Claude Code writes final encounter data files.

**Guiding principles:**
- Modern Pokémon (Gen 4–9) spread evenly across **all** routes — no generation gated to late-game only.
- Each route's pool has representation from multiple generations.
- Types loosely fit the environment (cave mons in caves, water mons on water routes, etc.).
- Water/fishing tables refreshed heavily — Fire Red's water routes are notorious for Tentacool spam.
- **Ability tiering:** Encounter tables should be informed by the quality of each Pokémon's ability. S-tier abilities (e.g. Speed Boost, Huge Power, Guts, Magic Guard, Regenerator, Protean, weather summoners) appear at 1–5% encounter rates, typically on weak Pokémon that won't break the game (e.g. Yanma/Speed Boost, Marill/Huge Power, Rattata/Guts). Most routes should have at most one such S-tier ability encounter. This creates hidden prizes for knowledgeable players without disrupting balance. Full ability tiering and route assignments are a Phase 3 design task.

---

## Trainer Battle Updates

All trainer classes need review. Workflow:
1. Claude Code audits all trainer data files.
2. Draft updated teams — modern Pokémon spread evenly across all trainer levels.
3. **Trainer type theme takes priority over generation** (a Swimmer uses Water-types from any gen; a Hiker uses Rock/Ground from any gen).
4. User reviews and approves.
5. Claude Code writes final trainer data.

Story-critical trainers (Gym Leaders, Rival, E4, Champion) are designed individually — see `specs/gym_leaders.md` and `specs/story_and_scripts.md`.

---

## Item Placement & Poké Mart Review

Full audit required. Workflow:
1. Audit all overworld item ball placements and hidden items.
2. Audit all Poké Mart and department store inventories.
3. Draft revised item distribution for user review.
4. Implement approved changes.

---

## TM List & Placement

Full TM list is a **Phase 3 design task** — Claude Code drafts a proposed list for user approval.

**Guidelines:**
- Retain iconic Fire Red TMs where possible (Earthquake, Ice Beam, Flamethrower, etc.)
- Add key Gen 4–9 moves not in Fire Red (Stealth Rock, U-turn, Scald, Dazzling Gleam, Moonblast, etc.)
- All 18 types represented in the TM pool.
- TM count expands beyond 50 — CFRU supports a larger TM table.
- **Placement principle:** Strong/powerful moves reserved for late-game routes, Victory Road, and dungeons.
- **No hidden TMs. No TMs in shops.** Discovery-focused only.

---

## Celadon Department Store — Evolution Items

All evolution items must be purchasable at the Celadon Dept. Store. Covers every item used in trade-free evolution and all held-item evolutions through Gen 9.

**Items to stock (non-exhaustive — produce full list during Phase 3 audit):**
- All elemental stones: Fire, Water, Thunder, Leaf, Moon, Sun, Shiny, Ice, Dusk, Dawn, Oval
- All held-item evolution items through Gen 9: Metal Coat, King's Rock, Dragon Scale, Upgrade, Dubious Disc, Protector, Electirizer, Magmarizer, Razor Claw, Razor Fang, Prism Scale, Sweet Apple, Tart Apple, Cracked Pot, Chipped Pot, Galarica Cuff, Galarica Wreath, Auspicious Armor, Malicious Armor, etc.
- Prices: steep but not prohibitive — evolution items are not meant to be a grind gate.

---

## Fossil Dealer — Pewter City Museum

Remove the Old Amber item ball from behind the Pewter Museum. Replace with a shady NPC ("Relic Dealer") near the museum back entrance who sells any fossil for a steep price. All fossil Pokémon accessible without specific locations.

**Fossil inventory (all generations):**

| Fossil | Pokémon | Gen |
|---|---|---|
| Old Amber | Aerodactyl | 1 |
| Dome Fossil | Kabuto | 1 |
| Helix Fossil | Omanyte | 1 |
| Root Fossil | Lileep | 3 |
| Claw Fossil | Anorith | 3 |
| Skull Fossil | Cranidos | 4 |
| Armor Fossil | Shieldon | 4 |
| Cover Fossil | Tirtouga | 5 |
| Plume Fossil | Archen | 5 |
| Jaw Fossil | Tyrunt | 6 |
| Sail Fossil | Amaura | 6 |
| Gen 8 fossil pairs (two items combined → revival) | Dracozolt / Arctozolt / Dracovish / Arctovish | 8 |

**Price:** 10,000–20,000 PokéDollars each. Gen 8 fossil combinations cost the sum of two fossil prices. Standard shop script with custom inventory. Shady dealer dialogue. No quest required.

Script file: `scripts/fossil_dealer.pks`

---

## Starter-Gifting NPCs (Yellow-Style)

Three NPCs in Yellow Pokémon gift locations. Each opens a scrollable selection menu. Player picks one starter; receives it at level 5 with its hidden ability. One-time gift per NPC (flag set on receipt).

| Location | Yellow original | SYNTH gift | NPC |
|---|---|---|---|
| Route 24 (post-Nugget Bridge) | Charmander | Any Fire starter (player picks) | Male Trainer |
| Vermilion City (outside Pokémon Center) | Squirtle | Any Water starter (player picks) | Officer Jenny |
| Cerulean City (house) | Bulbasaur | Any Grass starter (player picks) | Female Trainer |

**Starter pools:**

- **Fire:** Charmander, Cyndaquil, Torchic, Chimchar, Tepig, Fennekin, Litten, Scorbunny, Fuecoco
- **Water:** Squirtle, Totodile, Mudkip, Piplup, Oshawott, Froakie, Popplio, Sobble, Quaxly
- **Grass:** Bulbasaur, Chikorita, Treecko, Turtwig, Snivy, Chespin, Rowlet, Grookey, Sprigatito

**Implementation:** Menu script in XSE + C gift logic. Level 5, hidden ability, no held item.

Script files: `starter_gift_fire.pks`, `starter_gift_water.pks`, `starter_gift_grass.pks`

---

## Trade-Free Evolutions

CFRU has partial support. Extend to **all Gen 4–9 trade evolutions** using held item or level method. No trading required for any evolution in the game.
