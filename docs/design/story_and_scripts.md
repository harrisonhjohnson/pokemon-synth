# specs/story_and_scripts.md
# Story & Scripts — Beats, Gift Events, Rival, Elite Four, Narrative Framing

Read this spec for any XSE scripting session.

---

## Premise

You and your rival are selected as Professor Oak's two best students to field-test Silph Co.'s breakthrough product: **Project Synth** — genetically engineered Pokémon built from the DNA of Mew, Deoxys, Arceus, and Genesect. As you travel Kanto the story grows darker. Team Rocket oppose Project Synth publicly but their motives are murkier than they appear. Silph Co. present themselves as scientific pioneers but their ethics are increasingly indefensible. The player is caught between two corrupt institutions and must decide what should happen to the legends Silph captured — and the technology built from them.

---

## Player's Synth Gifts (6 total)

| # | Story beat | Location | Stage | Notes |
|---|---|---|---|---|
| 1 | Game start | Pallet Town (Oak's Lab) | Stage 1 | Starter. Player freely chooses any 1 of 18 types |
| 2 | Bill's House | Route 25 | Stage 1 | Gifted by Bill — expressing difficulty forming a positive bond with it |
| 3 | SS Anne showcase | Vermilion City / SS Anne | Stage 2 | Gifted by Silph as part of the investor event |
| 4 | Rocket Hideout | Celadon City | Stage 2 | Rescued from Team Rocket |
| 5 | Silph Co. rescue | Saffron City (after rescue) | Stage 3 | Silph scientist hands over most advanced prototype in gratitude |
| 6 | Pokémon Mansion | Cinnabar Island | SYNTH_LEGEND (600 BST) | Discovered in containment pod in Silph's old research wing. Standalone pseudo-legendary species — own sprite, stats, cry |

> **Note:** The stat editor is open from the moment the player receives their first Synth. There is no unlock gate tied to the 6th gift or any other story beat.

Script files: `scripts/synth_gift_[1-6].pks`

---

## Key Story Beats (XSE Scripting Targets)

### Beat 1 — Pallet Town: The Setup
Oak introduces the rival competition but frames it as a Silph Co. field test. Player is chosen as one of two test subjects for Project Synth. First Synth received. Oak is enthusiastic and trusting. Player is given a "Project Synth Briefing" document as a Key Item.

### Beat 2 — Cerulean City: First Cracks
Two threads in parallel:
- Player encounters Team Rocket agents near Nugget Bridge — hostile, publicly anti-Synth, dropping a memo with an ambiguous tone. Script: `cerulean_rocket_ambush.pks`
- Player visits Bill's house. Bill is a former Silph contractor who built the Pokémon storage system while Silph was a client. His logs reference early prototype failures and researchers whose objections were buried. He shows the player what he saw without telling them what to think. Second Synth gifted here. Script: `bills_house_lore.pks`

### Beat 3 — SS Anne: The Showcase
The ship is a floating Silph investor event. Trainer battles are Silph security and rival investors' bodyguards. The "Captain" is a Silph executive running the demo. The presentation is focused on profit — market potential, military applications, licensing. Third Synth received as a live demonstration. The event leaves a bad taste: Silph see Synth as a product, not a living creature. Script: `ss_anne_showcase.pks`

### Beat 4 — Rocket Hideout & Pokémon Tower: The Moral Inversion
Player raids the Rocket Hideout at Silph's request, framed as shutting down the opposition. The Rockets are beaten but one pauses before fleeing: *"You think you're the hero here? Go to Lavender Town. Look at what Silph left behind."*

In the Pokémon Tower the player uses the Silph Keycard to open a sealed chamber containing the ghosts of failed early Synth prototypes including eevee, porygon, ditto and Type:Null. Morty is here, tending to the spirits of failed Synth test cases. Defeating him unlocks the Ghost type. The scene reframes Rocket opposition as partly genuine, even if compromised. Fourth Synth rescued from the Hideout. Script: `lavender_tower_chamber.pks`

### Beat 5 — Silph Co. Takeover: The Hypocrite Revealed
The Rockets have occupied Silph Co. Player fights through and rescues the building. Giovanni is found with the CEO, demanding handover of the strongest Synth Pokémon to use as weapons. His public opposition was cover — he's exposed and retreats. Fifth Synth received from a grateful Silph scientist. A Silph executive mentions the four source legendaries have been "placed in safe hands with trusted partners." It sounds reassuring. It isn't. Script: `silph_co_rescue.pks`

### Beat 6 — Pokémon Mansion: The Evidence
Silph's original research facility, hidden in the old Mansion on Cinnabar Island. Experiment logs detail how Mew, Deoxys, Arceus, and Genesect were captured, studied, and genetically harvested. The language is clinical. A sixth Synth (`SYNTH_LEGEND`, 600 BST pseudo-legendary) is found in a containment pod never activated — a standalone species distinct from the three-stage line. The origin of Synth is fully explicit. Scripts: `cinnabar_lab_logs.pks`, `synth_gift_6.pks`

### Beat 7 — Viridian Gym: Giovanni's Last Argument
Giovanni is still here, abandoned by Team Rocket. Diminished. The fight is hollow. After losing he says one thing: *"You think I was wrong to want this. Maybe I was. But if I shouldn't have it — why should they? Silph didn't capture those legends to protect them. Go see for yourself."* He leaves. No arrest. Just a question. Script: `giovanni_confrontation.pks`

### Beat 8 — Pokémon League: The Reckoning
The Elite Four are Silph-sponsored and each holds one of the four source legendaries. Their pre-battle dialogue defends the arrangement. After four battles the player reaches the rival. He's processed everything — the Tower, the Mansion, Giovanni's words. He fights hard. When he loses, the player convinces him that Silph should not hold the legends. They agree to use their position as Silph's star field testers to force a release.

### Beat 9 — Postgame: Oak's Device
Oak calls both players to his lab. The evidence from Cinnabar has reached him. He doesn't defend Silph but isn't ready to condemn everything. He gives the player a device Silph developed to locate and attract legendary Pokémon using resonance signatures from research sites — requisitioned under the field test agreement. *"I trust you'll use it better than they did."* The Sevii Islands and Cerulean Cave open.

---

## Elite Four — Pre-Battle Dialogue & Legendary Assignments

Each member is Silph-sponsored and holds one of the four source legendaries. They believe they are doing the right thing.

| Member | Legendary | Pre-battle dialogue |
|---|---|---|
| Lorelei | Deoxys | "Silph asked us to keep them safe while the legal situation is resolved. I agreed. That's all this is." |
| Bruno | Genesect | "I don't care about the politics. This Pokémon needed someone strong to protect it. Here I am." |
| Agatha | Mew | "I've lived long enough to know that idealism is expensive. Silph pays well and Mew is unharmed. Don't lecture me." |
| Lance | Arceus | "You really think storming in here changes anything? Silph has lawyers. Silph has contracts. You have a Poké Ball." |

> **E4 teams:** TBD — diverse/thematic, not strict type teams. Design individually in Phase 3. E4 does NOT use Synth Pokémon.

---

## Champion — The Rival

Full Synth team. See rival stat budget table below and `specs/synth_system.md`.

---

## Rival's Synth Teams (XSE Script Targets)

All teams designed collaboratively in Phase 3. The stat budgets are fixed per fight — see `specs/synth_system.md` for BST values.

| Fight | Location | Script file |
|---|---|---|
| Fight 1 | Route 1 area / Pallet | `rival_fight_1.pks` |
| Fight 2 | Cerulean City | `rival_fight_2.pks` |
| Fight 3 | SS Anne | (embedded in `ss_anne_showcase.pks`) |
| Fight 4 | Pokémon Tower | `rival_fight_4.pks` |
| Fight 5 | Silph Co. | `rival_fight_5.pks` |
| Fight 6 | Route 22 | `rival_fight_6.pks` |
| Championship | Pokémon League | (Champion battle, no separate script) |

---

## Additional Script Files

```
rocket_hideout_data_terminal.pks  — terminal in Hideout referencing Synth experiments
cinnabar_lab_logs.pks             — Mansion experiment log document items
giovanni_confrontation.pks        — Viridian Gym post-battle scene
ss_anne_showcase.pks              — full SS Anne rescript as Silph investor showcase
cerulean_rocket_ambush.pks        — Nugget Bridge / Cave area first Rocket encounter
bills_house_lore.pks              — Bill contractor logs, DNA source references
lavender_tower_chamber.pks        — failed prototype chamber, Morty pre-battle
silph_resonance_device.pks        — Oak postgame gift event (Key Item)
```

---

## Narrative Framing Notes

- **Neither Silph nor Rocket is unambiguously right.** The moral conflict is dual-institution: Silph's exploitation vs. Rocket's opportunism.
- **The player is never forced to take a side** — the story presents evidence and lets the player process it.
- **Synth are living creatures**, not products. Every script should treat them with that weight.
- **The Cerulean Cave** narrative note: the cave's deepest chamber contains residual DNA signatures from the four source organisms Silph used to create Synth. Their presence is drawn by genetic resonance. A short Silph field log near the cave entrance explains this.
