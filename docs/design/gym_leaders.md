# specs/gym_leaders.md
# Gym Leaders — All 18 Leaders, Locations, Types, Aces

Read this spec for any gym or trainer design session.

---

## System Overview

There are **18 type gyms** total: the 8 original Kanto gyms (canonical types unchanged) plus 10 new type leaders placed in existing overworld locations. No new map geometry required.

**Each gym leader defeat:**
1. Awards a badge (standard).
2. Sets a flag bit in `SynthData.unlockedTypeFlags` unlocking that type in the Synth menu.

Type unlock flag storage: `u8 unlockedTypeFlags[3]` — 3 bytes = 24 bits, 18 used. Bit position matches type ID from `src/types.h`. Script: `type_unlock_[type].pks` (18 scripts).

---

## Original 8 Kanto Gyms (Canonical Types — Unchanged)

| Gym | Leader | Type | Location |
|---|---|---|---|
| Pewter | Brock | Rock | Pewter City |
| Cerulean | Misty | Water | Cerulean City |
| Vermilion | Lt. Surge | Electric | Vermilion City |
| Celadon | Erika | Grass | Celadon City |
| Fuchsia | Koga | Poison | Fuchsia City |
| Saffron | Sabrina | Psychic | Saffron City |
| Cinnabar | Blaine | Fire | Cinnabar Island |
| Viridian | Giovanni | Ground | Viridian City |

---

## 10 New Type Leaders (New NPCs in Existing Locations)

All leaders are visiting Johto gym leaders or similar NPCs placed in existing map locations.

| Type | Leader | Location | Lore / Reason for Being Here |
|---|---|---|---|
| Bug | Bugsy | Viridian Forest entrance | Found collecting a new specimen |
| Fairy | Whitney | Mt. Moon | Found collecting moon stones |
| Steel | Jasmine | Diglett's Cave | Searching for Alolan Dugtrio |
| Dark | Karen | Rock Tunnel | Found training in the dark cave |
| Ghost | Morty | Pokémon Tower (upper floor, Lavender Town) | Tending to a recent spike in ghost activity |
| Fighting | Chuck | Saffron Dojo | Boasts his 100% totally natural, steroid-free fighting types will blow through the Synth Pokémon |
| Flying | Falkner | Cycling Road gatehouse | Flew to Kanto to get a glimpse of the Synth Pokémon |
| Normal | Safari Zone Secret House NPC | Safari Zone (Secret House) | Ace: Kangaskhan |
| Ice | Pryce | Seafoam Islands entrance | Seeking peace, away from society — disturbed by the Synth project and modern scientific progress |
| Dragon | Clair | Victory Road / Route 23 area | Wants a challenging battle |

---

## Team Design Notes

- **8 original leaders:** Team redesigns in Phase 3 — modern Pokémon spread across all levels, type theme maintained, individually designed.
- **10 new leaders:** Full team design in Phase 3. Confirmed ace Pokémon: Normal leader's ace is Kangaskhan.
- **Hard Mode sequences:** All 18 leaders receive handcrafted Hard Mode strategy sequences in Phase 3 — scripted lead choices, predefined switch priorities, held items. Hard Mode AI only applies to full-strength rosters (see Gym Rematch System below); first encounters never use it.
- **Level cap table:** Designed in Phase 3 after all 18 gym ace levels are finalized. The 10 new type leaders are optional-order; their caps form a graduated ladder tuned to placement in the world. E4 also has a cap set below the Champion's ace.
- **Full-strength rosters:** Each leader has a canonical full-strength (mono-type) team — see Gym Rematch System below for the complete list per leader. These are the "true" teams; first-encounter rosters are cut-down previews of these.

---

## Gym Rematch System

Each leader has two roster states: a **first-encounter** roster (a preview) and a **full-strength** roster (their real team, listed under each type below). Which one the player faces depends on a global flag and the leader's position in the badge order.

### Unlock Gate

- `rematchSystemUnlocked` flips to true once the player earns their **9th badge** — the intended midpoint of the 18-gym sequence, landing around Erika/Morty. Exact leader ordering is still open (per the optional-order note above); this flag is defined by badge count, not by specific leader identity, until Phase 3 finalizes the sequence.
- **Badges 1–9 (pre-gate group):** First encounter uses the cut-down preview roster (see below). Once `rematchSystemUnlocked` is true, these 9 leaders become rematchable at full strength, repeatably, at any time.
- **Badges 10–18 (post-gate group):** By the time the player reaches these leaders, the gate is already open, so there is no meaningful difference between a "first" and "full-strength" fight. These leaders skip the preview roster entirely and use their full-strength team on the one and only encounter.

### First-Encounter Roster Rules (pre-gate group, badges 1–9 only)

- For any full-strength team member with a natural pre-evolution, downgrade it for the first encounter (e.g., Monferno instead of Infernape, Fletchinder instead of Talonflame), at the level set by the graduated cap table for that leader's slot.
- For team members with **no earlier evolutionary stage** (e.g., Turtonator, Kangaskhan, Mimikyu, Basculegion, Hisuian Zoroark) — omit that slot entirely from the first encounter. The full-strength rematch is the first time the player sees that mon.
- First encounters never use held items or Hard Mode AI scripting, regardless of leader.

### Full-Strength Roster Rules (rematch for badges 1–9, first fight for badges 10–18)

- Roster is exactly the full-strength list under each type below.
- Levels scale to the player's current Hard Mode level cap at time of battle, not a fixed value — a badge-9 rematch and a postgame rematch should both feel current.
- Held items and full Hard Mode AI sequencing (Phase 3) apply here only.
- Rematches are **repeatable indefinitely** — no one-time claim flag needed. Money is not a meaningful constraint in this game, and TMs are unlimited-use, so there's no economy risk in letting the player refight for practice, EXP, or just to see the team again.

### TM Rewards

Two tiers of TM, gated to match the two-tier badge structure:

- **Badges 1–9 (pre-gate group):** First win (badge + type unlock) also awards a **low-tier TM** of that type. The **signature high-tier TM** is withheld until the player clears the full-strength rematch.
- **Badges 10–18 (post-gate group):** Since there's only one fight, the **signature high-tier TM** is awarded immediately alongside the badge and type unlock — no rematch gate needed.

| Type | Leader | Low-Tier TM (1st win, badges 1–9 only) | Signature TM (full-strength clear) |
|---|---|---|---|
| Rock | Brock | Rock Tomb | Stone Edge |
| Water | Misty | Water Pulse | Scald |
| Electric | Lt. Surge | Charge Beam | Wild Charge |
| Grass | Erika | Magical Leaf | Leaf Storm |
| Poison | Koga | Acid | Gunk Shot |
| Psychic | Sabrina | Psybeam | Psychic |
| Fire | Blaine | Incinerate | Flare Blitz |
| Ground | Giovanni | Mud Shot | Earth Power |
| Bug | Bugsy | Bug Bite | Megahorn |
| Fairy | Whitney | Fairy Wind | Moonblast |
| Steel | Jasmine | Metal Claw | Iron Head |
| Dark | Karen | Snarl | Knock Off |
| Ghost | Morty | Astonish | Shadow Ball |
| Fighting | Chuck | Low Kick | Close Combat |
| Flying | Falkner | Air Cutter | Brave Bird |
| Normal | Safari Zone NPC | Quick Attack | Extreme Speed |
| Ice | Pryce | Powder Snow | Icicle Crash |
| Dragon | Clair | Twister | Outrage |

> **Note:** Which leaders end up in the pre-gate (1–9) vs. post-gate (10–18) group depends on final leader ordering (Phase 3). Both TM tiers listed above apply to every leader regardless of group — the only thing that changes is whether the low-tier TM is awarded at all (pre-gate only) and whether the signature TM requires a rematch (pre-gate) or comes free with the single fight (post-gate).
>
> Before finalizing, cross-check this table against CFRU/DPE's default TM distribution (e.g., Celadon Dept. Store, wild TM pickups) to make sure none of these are already obtainable elsewhere — the reward only works if it's genuinely exclusive.

### Full-Strength Rosters (Mono-Type Teams)

| Type | Leader | Full-Strength Roster |
|---|---|---|
| Fire | Blaine | Turtonator, Monferno→Infernape, Arcanine, Talonflame, Volcarona, Camerupt |
| Ghost | Morty | Gengar, Annihilape, Chandelure, Basculegion, Mimikyu, Hisuian Zoroark |
| Poison | Koga | Venusaur, Crobat, Tentacruel, Alolan Muk, Galarian Weezing, Sneasler |
| Dark | Karen | Meowscarada, Honchkrow, Kingambit, Grimsnarl, Overqwil, Houndoom, Pangoro, Tyranitar |
| Rock | Brock | Aggron, Cursola, Aerodactyl, Barbaracle, Glimmora, Tyrantrum |
| Psychic | Sabrina | Gallade, Hatterene, Slowbro, Hisuian Braviary, Metagross, Wyrdeer, Delphox |
| Ice | Pryce | Lapras, Froslass, Baxcalibur, Crabominable, Mamoswine, Alolan Ninetales |
| Normal | Safari Zone NPC | Staraptor, Ursaluna, Bewear, Heliolisk, Hisuian Zoroark, Kangaskhan, Porygon-Z |
| Grass | Erika | Ludicolo, Ferrothorn, Whimsicott, Cradily, Chesnaught, Tropius, Dhelmise |
| Electric | Lt. Surge | Magnezone, Vikavolt, Kilowattrel, Electivire, Rotom Wash, Hisuian Electrode |
| Bug | Bugsy | Scizor, Ariados, Heracross, Durant, Golisopod, Shedinja |
| Water | Misty | Primarina, Gastrodon, Kingdra, Walrein, Starmie, Pelipper |
| Fighting | Chuck | Annihilape, Hawlucha, Scrafty, Lucario, Poliwrath, Blaziken |
| Flying | Falkner | Gyarados, Gliscor, Togekiss, Salamence, Skarmory, Kilowattrel |
| Ground | Giovanni | Swampert, Gliscor, Excadrill, Nidoking, Palossand, Garchomp |
| Dragon | Clair | Dragalge, Kingdra, Duraludon, Dragapult, Noivern, Flygon |
| Fairy | Whitney | Primarina, Togekiss, Gardevoir, Tinkaton, Mimikyu, Grimsnarl |
| Steel | Jasmine | Excadrill, Corviknight, Tinkaton, Lucario, Empoleon, Scizor |

> Some Pokémon appear on more than one team (e.g., Mimikyu on both Ghost and Fairy, Kilowattrel on both Electric and Flying, Hisuian Zoroark on both Ghost and Normal) — confirm this overlap is intentional or assign alternates during Phase 3 team building.

---

## Script Files

One type unlock trigger script per leader (18 total):
```
type_unlock_rock.pks
type_unlock_water.pks
type_unlock_electric.pks
... (one per type)
```

Each script: sets the relevant bit in `unlockedTypeFlags`, plays a brief unlock notification, then returns to normal badge-award flow.
