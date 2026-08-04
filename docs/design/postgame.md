# specs/postgame.md
# Postgame — Sevii Islands Summoning, Cerulean Cave, Fixed Encounters

Read this spec for all Phase 4 work.

---

## Unlock Trigger

All postgame content gates on `FLAG_CHAMPION_DEFEATED`. When set:
- Oak gifts the Silph Resonance Device (Key Item) with a short cutscene.
- Navel Rock and Birth Island unlock as Seagallop Ferry destinations.
- All 12 imprinting sites become active.
- Cerulean Cave four-chamber encounters become active.
- Fixed legendary replacements (Zeraora, Chien-Pao) become active.
- Postgame stat editor activates (`postGameUnlocked = 1` on all Synths).

---

## Silph Resonance Device

- Key Item given by Oak post-Champion.
- Must be in the bag to trigger imprinting site events (checked in script before summon).
- Cannot be used in the main game — sites are inert until the postgame flag is set.
- Script: `silph_resonance_device.pks`

---

## Sevii Islands Legendary Summoning

### Overview

Each Sevii Island and two bonus islands hosts one or more "imprinting sites" — locations with high legendary energy residue. Interacting with a site while holding the Silph Resonance Device summons a random legendary from that generation's pool.

- Each site resets **once per real-time day** — usable again after midnight. The RTC date is checked on interaction; if the stored last-used date matches today's date, the site is dormant.
- Soft-resetting before interaction is intentional and supported. Lore: "the device reads a unique resonance signature each time it activates."
- Minimal scripting per site: environmental description (2–3 lines), one-time Silph field researcher NPC or item, summoning script.
- Lock all buildings except Pokémon Centers by default.

### Island Assignment & Legendary Pools

| Island | Generation | Pool members | Sites |
|---|---|---|---|
| One Island | Gen 1 | Articuno, Zapdos, Moltres, Mewtwo | 1 |
| Two Island | Gen 2 | Ho-Oh, Lugia, Suicune, Raikou, Entei, Celebi | 1 |
| Three Island | Gen 3 | Rayquaza, Kyogre, Groudon, Latios, Latias, Jirachi, Regirock, Regice, Registeel | 1 |
| Four Island | Gen 4 | Dialga, Palkia, Giratina, Darkrai, Shaymin, Cresselia, Uxie, Mesprit, Azelf, Regigigas, Heatran, Manaphy | 1 |
| Five Island | Gen 5 | Reshiram, Zekrom, Kyurem, Victini, Landorus, Tornadus, Thundurus, Keldeo, Meloetta, Cobalion, Terrakion, Virizion, Melmetal | 1 |
| Six Island | Gen 6 | Xerneas, Yveltal, Zygarde (100% form), Diancie, Hoopa, Volcanion | 1 |
| Seven Island | Gen 7 | Site A: Solgaleo, Lunala, Necrozma, Magearna, Marshadow, Tapu Koko, Tapu Lele, Tapu Bulu, Tapu Fini. Site B: All 11 Ultra Beasts | **2 sites** |
| Navel Rock | Gen 8 | Zacian, Zamazenta, Eternatus, Calyrex, Glastrier, Spectrier, Urshifu, Zarude, Regieleki, Regidrago, Enamorus, Galarian Articuno, Galarian Zapdos, Galarian Moltres | **2 sites** |
| Birth Island | Gen 9 | Koraidon, Miraidon, Wo-Chien, Ting-Lu, Chi-Yu, Walking Wake, Raging Bolt, Gouging Fire, Iron Leaves, Okidogi, Munkidori, Fezandipiti, Ogerpon, Pecharunt, Terapagos | **2 sites** |

> **NOT in any island pool:** Mew, Deoxys, Genesect, Arceus (Cerulean Cave only), Zeraora (Power Plant), Chien-Pao (Seafoam Islands).

### Implementation Notes

- Random selection: `(Random() % poolSize)` in C — each island's pool is a static const array in `src/legendary_pools.c`.
- Summon levels: 60–70 for most; box legends at 70; Ultra Beasts at 65; Gen 8–9 legends at 70.
- **Daily reset logic:** Each site stores the last-used RTC date (day/month/year) in the save block. On interaction, compare stored date to today's RTC date. If they match → display dormant message ("The site's energy is quiet for now. Come back tomorrow."). If they differ → allow summon, write today's date to the save block.
- While dormant, the examine prompt changes to the dormant message. The site visually reactivates on the next calendar day.
- **Save data per site:** 3 bytes (day, month, year) × 12 sites = 36 bytes total in save block expansion. No boolean flags needed — the date comparison is the gate.
- **Tent interaction:** If the player uses the Tent item to flip to night/day, the RTC hour changes but the RTC date does not. Sites cannot be farmed by toggling the Tent — only real calendar day progression resets them.
- **Navel Rock:** World map flag `0x8B5`. Map data already in BPRE ROM — rescript only. Remove vanilla Ho-Oh/Lugia sprites (those Pokémon move to Two Island pool).
- **Birth Island:** World map flag `0x8C2`. Map data already in BPRE ROM — rescript only. Remove vanilla Deoxys triangle puzzle (Deoxys moves to Three Island pool).
- Both accessed via existing Seagallop Ferry system — add as destinations unlocked by Champion flag alongside Resonance Device gift.

### Script Files

```
scripts/
  silph_resonance_device.pks
  imprinting_site_1.pks        — One Island Gen 1
  imprinting_site_2.pks        — Two Island Gen 2
  imprinting_site_3.pks        — Three Island Gen 3
  imprinting_site_4.pks        — Four Island Gen 4
  imprinting_site_5.pks        — Five Island Gen 5
  imprinting_site_6.pks        — Six Island Gen 6
  imprinting_site_7a.pks       — Seven Island Gen 7 site A (Tapus + box legends)
  imprinting_site_7b.pks       — Seven Island Gen 7 site B (Ultra Beasts)
  imprinting_site_navel_a.pks  — Navel Rock Gen 8 site A
  imprinting_site_navel_b.pks  — Navel Rock Gen 8 site B
  imprinting_site_birth_a.pks  — Birth Island Gen 9 site A
  imprinting_site_birth_b.pks  — Birth Island Gen 9 site B
```

---

## Cerulean Cave — Four E4 Legends

Mewtwo is removed from Cerulean Cave. Instead, all four E4-associated legendaries are catchable here in the postgame.

| Chamber | Legendary | E4 association | Overworld sprite |
|---|---|---|---|
| Chamber A | Mew | Agatha | Small, Gen 1 — check community repos first |
| Chamber B | Deoxys | Lorelei | Multiple forms; use Normal Forme — check community repos |
| Chamber C | Genesect | Bruno | Mechanical design — may need creation |
| Chamber D | Arceus | Lance | Large — may need scaling |

**Overworld sprite note:** GBA overworld sprites are small (16×32 or 16×16 px). Check community repos for Fire Red-compatible Mew and Deoxys sprites first (popular, may already exist). Genesect and Arceus are less common — may require creation. Claude Code can generate placeholder sprites if needed.

**Implementation:** Each chamber is a scripted encounter. Player examines the tile → brief one-line lore message (referencing the E4 member's connection to this Pokémon) → battle initiates. Flag set on encounter start. All four available once `FLAG_CHAMPION_DEFEATED` is set.

**Narrative note:** The cave's deepest chamber contains residual DNA signatures from the four source organisms Silph used to create Synth. A short Silph field log near the cave entrance explains their presence.

Script files:
```
cerulean_cave_mew.pks
cerulean_cave_deoxys.pks
cerulean_cave_genesect.pks
cerulean_cave_arceus.pks
```

---

## Fixed Kanto Legendary Replacements

The three legendary birds are replaced with fixed scripted encounters. Available postgame only (Champion flag check at encounter trigger). These are NOT random — same location, same Pokémon, every time.

| Location | Replaced | Replacement | Level | Lore framing |
|---|---|---|---|---|
| Power Plant | Zapdos | **Zeraora** | 70 | Silph used the Power Plant as a Zeraora habitat study — drawn by electrical output |
| Seafoam Islands | Articuno | **Chien-Pao** | 70 | Silph researchers detected anomalous cold readings; Chien-Pao migrated from Paldea following cryo-research signals |

> **Zapdos, Articuno, and Mewtwo** re-enter the One Island Gen 1 pool — still catchable via summoning. Moltres's vanilla location (Mt. Ember, One Island) already in Sevii Islands — Moltres re-enters the One Island pool rather than being a fixed encounter.

Script files: `zeraora_encounter.pks`, `chien_pao_encounter.pks`

---

## Postgame Stat Editor

When `FLAG_CHAMPION_DEFEATED` is set, for all Synths in party and box:
- Set `postGameUnlocked = 1`
- Remove BST cap (budget ceiling lifted — player can keep earning RP indefinitely)
- Set `statCaps[6]` to `{255, 255, 255, 255, 255, 255}`
- Set `resonanceCap` to `0xFFFF` (effectively uncapped)
- Per-stat minimum remains 20

`SynthValidateStats()` must still be called — enforces the minimum and per-stat max, no longer enforces a maximum BST sum.

---

## Ultimate Form Reward — Resonance Amplifier

After `FLAG_CHAMPION_DEFEATED` is set, the rival becomes available for a postgame scene gated behind completing all 12 island summoning sites. Once all 12 sites have been activated (all 12 date-comparison entries in the save block are non-zero, indicating at least one use each), the rival contacts the player and meets them at Pallet Town.

**Scene:** The rival presents the Silph Resonance Amplifier — a prototype he retrieved from the Cinnabar Mansion research files, theorising it could push a Synth to its designed biological ceiling. Short dialogue exchange, item delivered, scene ends. No battle in this scene.

**Item delivery script:** `scripts/resonance_amplifier_gift.pks`

**Gate check:** Script checks that all 12 site date entries are non-zero before triggering. This ensures the reward requires meaningful postgame engagement rather than just defeating the Champion.

For full implementation details of the Resonance Amplifier item and ultimate form mechanics, see `specs/synth_system.md` — Ultimate Form section.
