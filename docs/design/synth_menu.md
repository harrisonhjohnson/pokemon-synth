# Synth Menu

The Synth Menu is a dedicated screen for viewing and customising the player's Synth Pokémon. It is accessible from the main menu / party screen and is **not accessible during battle**.

---

## Layout

```
┌─────────────────────────────────────┐
│ ◀           AXIOM-01 · Lv.28       ▶│  ← Banner
├─────────────────────────────────────┤
│  FORM   Striker                      │  ← Form bar
├──────────────┬──────────────────────┤
│  [sprite]    │  Budget  266 / 300   │
│              │  ▓▓▓▓▓▓▓▓▓▓░░        │
│  [TYPE 1]    │  HP   ▓▓▓▓▓▓░  55 ↕ │
│  [TYPE 2]    │  Atk  ▓▓▓▓▓░░  50 ↕ │
│              │  Def  ▓▓▓▓░░░  40 ↕ │
│              │  SpA  ▓▓▓▓▓▓░  60 ↕ │
│              │  SpD  ▓▓▓░░░░  35 ↕ │
│              │  Spe  ▓▓▓░░░░  26 ↕ │
├──────────────┴──────────────────────┤
│  ABILITY   Levitate    (from Gengar) │  ← Ability bar
└─────────────────────────────────────┘
```

### Sections
- **Banner** — Synth name and level. Left/right to switch between party Synths.
- **Form bar** — Current Form (archetype). Full-width strip beneath the banner. Opens a dropdown sub-menu on A.
- **Type slots** — Primary and secondary type. Left column, beneath sprite.
- **Stat editor** — BST budget bar and six stat rows. Right column.
- **Ability bar** — Current ability and source Pokémon. Full-width bottom strip.

---

## Navigation Model

### Cursor / focus rules
- On open, cursor is always on the **Banner**.
- **D-pad up/down** moves the cursor between sections in order: Banner → Form → Type → Ability (down), and reverse (up). Right from Type moves cursor to Stats; left from Stats moves cursor back to Type.
- Sections are **highlighted** when the cursor is on them (yellow outline in final UI).
- **A** enters the highlighted section (activates sub-menu).
- **B behaviour is two-step:**
  - If inside a sub-menu (actively editing), B **exits the sub-menu** but leaves the cursor on that section.
  - If the cursor is on a section but not inside a sub-menu, B **returns cursor to Banner**.

### D-pad layout summary
```
Banner:     ◀/▶ = switch Synth
            ↓   = move cursor to Form

Form:       ↑   = move cursor to Banner
            ↓   = move cursor to Type
            A   = enter Form sub-menu

Type:       ↑   = move cursor to Form
            ↓   = move cursor to Ability
            →   = move cursor to Stats
            A   = enter Type sub-menu

Stats:      ←   = move cursor to Type
            ↓ (on last stat) = move cursor to Ability
            A   = enter Stats sub-menu

Ability:    ↑   = move cursor to Stats (or Type if coming from left col)
            A   = enter Ability sub-menu
```

### Inside sub-menus

**Form sub-menu**
- Opens a dropdown list of all available Forms for this Synth species.
- ▲/▼ scrolls through the list.
- Selecting a Form (A) does two things immediately:
  1. Updates the front sprite to the Form's sprite.
  2. Loads the Form's preset stat spread into the stat editor as a new starting point.
- B exits without confirming; cursor remains on Form section.

**Type sub-menu**
- ◀/▶ switches between primary and secondary type slot.
- ▲/▼ cycles through the type dropdown list for the selected slot.
- Dropdown shows only **gym-unlocked types** — locked types are hidden entirely.
- Secondary type always has **"None"** at the top of its dropdown; selecting it clears type 2.
- B exits the sub-menu; cursor remains on Type section.

**Stat sub-menu**
- ▲/▼ moves between stat rows (HP → Atk → Def → SpA → SpD → Spe).
- ◀/▶ raises or lowers the selected stat by 1.
- Constraints enforced in real time:
  - Per-stat minimum: **20**
  - Per-stat cap: **80** (stage 1), **100** (stage 2), **120** (stage 3), **150** (legend), **255** (postgame / ultimate)
  - Total cannot exceed **BST budget** (base stage budget + current RP). Raising a stat that would exceed budget does nothing.
- Changes apply **immediately** on adjustment — no confirm step.
- Budget bar updates live as stats are changed.
- After each stat adjustment, `GetClosestForm()` runs and may update the displayed Form and sprite (see Form Auto-Assignment below).
- B exits the sub-menu; cursor remains on Stats section.

**Ability sub-menu**
- ◀/▶ cycles through the ability pool list.
- Pool contents:
  - **Trace** — always present as a baseline option.
  - Abilities of all **party Pokémon** (deduplicated).
  - The Synth's **remembered ability** (see below), if one exists — shown with source label.
- B exits the sub-menu without confirming; cursor remains on Ability section.
- **Confirming** (A on selected ability, or backing out after selection — TBD at implementation): populates the remembered slot and updates the displayed ability.

---

## Form Auto-Assignment

Whenever the player adjusts a stat, `GetClosestForm()` runs and updates the active Form (and sprite) to whichever preset best matches the current spread.

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
2. **Current form sticky** — if the Synth's currently assigned Form is among the tied forms, it stays. No sprite change.
3. **Closest Spe stat** — if the current form is not in the tie, whichever tied form has the smaller `|current->spe - form->spe|` wins.
4. **Lower preset index** — deterministic fallback; should effectively never fire in practice.

### Behaviour notes
- Form auto-assignment only fires during stat editing. Manually selecting a Form from the dropdown always overrides it.
- The sprite updates immediately on each auto-assign to reflect the new Form.
- There is no "Custom" state — the Form always resolves to the closest named preset.

---

## Budget Display

- Shows **base BST + accumulated RP** as a single combined number: e.g. `266 / 300`.
- Denominator is the current stage budget (does not display RP cap separately).
- RP continues accumulating via battle (4 EVs = 1 RP) up to the stage cap. Budget ceiling grows accordingly and stops once the RP cap is reached.
- No EV counter is shown to the player.
- Postgame (after Resonance Amplifier): RP cap and per-stat cap are lifted. Budget bar denominator becomes open-ended; bar fills to a soft reference line and overflows past it in a distinct colour.

---

## Ability — Remembered Slot

- Each Synth tracks **one remembered ability** independently in its save data.
- Populated only after the player confirms an ability selection and exits the sub-menu.
- If no ability has ever been assigned, the remembered slot is **hidden** from the pool.
- Ability is **copied at assignment time** — no persistent link to the source Pokémon. Moving that Pokémon out of the party does not affect the Synth's current or remembered ability.
- All Synths start with **Trace** as their default ability.

---

## Save Block

- Each Synth slot stores one remembered ability ID and one active Form ID.
- 6 Synth slots × 2 bytes = **12 bytes** additional in the Synth save block (1 byte ability + 1 byte form per slot).
- See `synth_system.md` for full save block layout and total size.

---

## Accessibility Note

The Synth Menu is **not accessible during battle**. No in-battle hook or check is needed beyond blocking the menu entry point.

---

## Open Questions (for implementation)

- Exact confirm gesture inside Ability sub-menu: A on the highlighted entry, or auto-confirm on B-exit? Recommend A to confirm to avoid accidental changes.
- Visual treatment for the budget bar in postgame overflow state.
- Sound cue when a stat adjustment is blocked (at cap or budget limit).
- Form preset spreads need to be defined per species (SYNTH/SYNTH2/SYNTH3/SYNTH_LEGEND/SYNTH3_ULTIMATE) — these belong in `synth_system.md`.
