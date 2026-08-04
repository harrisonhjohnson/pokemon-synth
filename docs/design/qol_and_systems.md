# specs/qol_and_systems.md
# QoL & Systems — Learn Moves, Move Tutor, Hard Mode AI, Level Caps, Cheat Console

Read this spec for any feature implementation session.

---

## QoL Feature Table

Enable/configure in `src/config.h` and via event scripting.

| Feature | Implementation |
|---|---|
| No HMs required | CFRU field move system — Surf/Fly/Waterfall/etc. as Key Items or auto-abilities |
| Trade-free evolutions | CFRU partial support — extend to all Gen 4–9 trade evos (held item or level method) |
| Enhanced Pokédex | CFRU base stats on summary — extend to show locations, evolutions, full learnset |
| DexNav | Already in CFRU — enable in `config.h` |
| Reusable TMs | Already in CFRU — enable in `config.h` |
| Exp. Share | Given by Oak at game start alongside starter/Synth. CFRU supports natively — add item to Oak's delivery script |
| Scalable Exp. | Pokémon earn less Exp. when higher level than defeated Pokémon, more when lower level |
| Upgraded bag | Already in CFRU — enable expanded bag in `config.h` |
| Repel system update | Already in CFRU — enable in `config.h` |
| Day/night cycle | Real-time clock driving overworld palette shifts — see section below |
| Tent (Key Item) | Toggles between day and night on use — given at game start — see section below |
| Ability Capsule / Ability Patch | Available for purchase at Celadon Department Store — see section below |

---

## Celadon Department Store — Ability Items

Ability Capsules and Ability Patches are available for purchase at the Celadon Department Store.

- **Ability Capsule** — switches a Pokémon between its two standard abilities. Price TBD in Phase 3 (suggested: steep but reachable mid-game).
- **Ability Patch** — switches a Pokémon to its Hidden Ability. Price TBD in Phase 3 (suggested: significantly more expensive than the Capsule).

These are stocked alongside the evolution items as part of the general "power user" floor of the department store. Exact floor/clerk assignment is a Phase 3 detail.

---

## Day/Night Cycle

The game runs a standard Pokémon day/night cycle driven by the GBA's RTC (real-time clock), consistent with Gold/Silver/HG/SS behaviour.

**Time periods and overworld palette:**

| Period | Hours | Palette tint |
|---|---|---|
| Morning | 06:00–09:59 | Warm amber-gold |
| Day | 10:00–17:59 | Neutral (default) |
| Evening | 18:00–19:59 | Orange-red dusk |
| Night | 20:00–05:59 | Cool dark blue |

**Implementation:**
- Hook into CFRU's existing RTC support (Fire Red has RTC hardware; CFRU exposes it).
- Overworld palette shift applied via `SVC_LoadPalette` or CFRU's dynamic palette hooks — tint blended over the base map palette each period transition.
- Wild encounter tables can optionally vary by time period (e.g. different Pokémon at night) — implement the hook now, populate tables in Phase 3.
- NPC dialogue can reference time of day via a `gTimeOfDay` variable exposed from the RTC hook.
- Indoor maps are exempt from palette shifts (caves, buildings stay neutral regardless of time).

**Files:** `src/day_night.c`, hooked into map load and period-transition callbacks.

---

## Tent (Key Item)

A Key Item given to the player at the start of the game alongside the Exp. Share and Synth. Using it from the bag toggles the game clock between **day** (forces 12:00 noon) and **night** (forces 22:00). The shift is immediate — overworld palette updates on the next map load or within one frame via palette hook.

**Design intent:** Players who want to catch night-exclusive Pokémon or access time-gated content without waiting in real time can do so freely. No cooldown, no restriction on use.

**Implementation:**
- Tent is a Key Item with a custom `ItemUseCallback` in `src/item_use.c`.
- On use: read current `gRTCHour`. If `gRTCHour` is in the Day period (10–19), write 22 to the RTC hour register (set to Night). Otherwise write 12 (set to Day). Update `gTimeOfDay` immediately.
- Force a palette reload after the write so the overworld updates without requiring a map transition.
- The RTC write is to the internal game clock variable, not the hardware clock — the player's system time is unaffected.
- Display a brief message on use: *"You set up the Tent... Time seems to have shifted."* (Day→Night) or *"You pack up the Tent... Morning has come."* (Night→Day).

**Item constants:**
```c
#define ITEM_TENT  (assign unused item ID)
```

**Given by:** Oak's lab delivery script at game start, same event as Exp. Share.

---

## Learn Moves (Party Menu)

New "Learn Moves" sub-option in the party Pokémon action menu (alongside Summary, Switch, etc.).

**Opens a categorized move browser with two tabs:**

1. **Level-up & Evolution moves** — any move the Pokémon could have learned at its current level or via a prior evolution stage, teachable instantly for free.
2. **TMs** — all TMs the player currently owns that are compatible with this Pokémon, teachable instantly.

**Does NOT include:** tutor moves or egg moves. Those require the Pokémon Center Move Tutor NPC.

Applies to ALL Pokémon (not just Synth). Modelled on ROWE / Emerald Rogue.

**File:** `src/learn_moves_menu.c`

---

## Move Tutor NPC (All Pokémon Centers)

NPC present in every Pokémon Center. Teaches **tutor moves** — moves not in the level-up or TM list (e.g. Stealth Rock, Draco Meteor, Knock Off, all Gen 4–9 tutor moves). Does NOT teach level-up, evolution, or TM moves.

- Each move costs PokéDollars — tiered pricing (common tutors cheaper, signature/powerful moves expensive). Exact prices TBD in Phase 3.
- Egg moves: TBD whether included here or excluded entirely.
- Shared script across all Pokémon Centers.

**Script file:** `scripts/move_tutor_pokecenter.pks`

---

## Hidden Cheat Console (Player Room)

When the player interacts with the video game console in their bedroom, a text input prompt appears. Entering a valid code activates its effect. Codes are **case-sensitive**. No in-game hint this system exists.

| Code | Effect | Notes |
|---|---|---|
| `9RARECANDY` | Gives 999 Rare Candies | Adds to bag, caps at 999 |
| `MASTERBALL!` | Gives 999 Master Balls | Adds to bag, caps at 999 |
| `PAYDAY999999` | Adds ₽1,000,000 to wallet | Caps at game's max money value |

**Implementation:**
- Text input reuses or adapts CFRU's existing naming-screen input handler.
- Code validation via string comparison against a static table — easily extensible.
- Each code triggers a direct item/money grant — no persistent flags needed.
- Subtle confirmation on success; neutral "error" screen for invalid codes.

**File:** `src/cheat_console.c`

---

## Hard Level Caps

The game enforces hard experience-based level caps tied to the next gym leader's ace Pokémon level. Pokémon refuse to gain experience beyond the cap until the relevant badge is earned.

**Implementation:**
- Hook into the experience gain function in CFRU's battle/Exp code.
- Before applying Exp: if `pokemon.level >= gNextGymCapLevel[badgesObtained]` → suppress Exp gain (Exp bar fills visually but does not level up).
- `gNextGymCapLevel[]` is a static array indexed by badge bitmask popcount (badges held).
- Display brief notification when cap is hit: *"[Pokémon] is at its limit for now..."*
- Cap lifts automatically when the corresponding badge is earned.
- Badge check uses a **bitmask** (not a simple count) to correctly handle non-sequential badge collection across 18 gyms.
- A cap must be defined for the E4 — set below the Champion's ace level.

**Cap table:** Designed in Phase 3 after all 18 gym leader ace levels are finalized. The 10 optional new leaders form a graduated ladder tuned to world placement.

---

## Battle AI Review & Hard Mode

### CFRU AI Baseline

CFRU improves on vanilla Fire Red: evaluates type effectiveness, stat stage changes, basic priority moves. Does not switch strategically, manage held items properly, play around abilities, or use setup moves intelligently.

### Hard Mode

A difficulty toggle selectable at game start or from the Options menu at any time (confirmation prompt required). Stored as `FLAG_HARD_MODE` in the save block. Affects **trainer AI only** — wild encounters unaffected.

Hard Mode uses a tiered AI decision tree in `src/battle_ai_hard.c`. When `gHardMode == TRUE`, trainers use the enhanced tier:

| Behavior | Normal AI | Hard Mode AI |
|---|---|---|
| Move selection | Best type matchup | Full damage calc including STAB, EVs, ability interactions |
| Status moves | Rarely used | Situational — Will-O-Wisp on physical attackers, Thunder Wave on speed-sensitive matchups |
| Switching | Almost never | Switches on predicted KO, hard type disadvantage, or better available matchup |
| Setup moves | Used once if in move pool | Used when situation is "safe" (no priority KO threat from player) |
| Entry hazards | Used once if in move pool | Leads with Stealth Rock if available |
| Priority moves | Occasional | Used when it secures a KO the normal move would not |
| Ability awareness | None | Avoids triggering beneficial player abilities; respects Levitate, Intimidate, Flash Fire, etc. |

**Gym leaders and E4 in Hard Mode** receive handcrafted strategy sequences — scripted lead choices, predefined switch priorities, held items tuned to teams. Authored in Phase 3.

**Hard Mode does NOT affect:** level caps, catch rates, shop prices, or Synth customization.

**New file:** `src/battle_ai_hard.c` — Hard Mode decision tree, referenced conditionally from `src/battle_ai.c`.
