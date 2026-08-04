# Pokémon SYNTH

A Fire Red ROM hack built on [CFRU](https://github.com/Skeli789/Complete-Fire-Red-Upgrade) (Complete Fire Red Upgrade) and [DPE](https://github.com/Skeli789/Dynamic-Pokemon-Expansion) (Dynamic Pokémon Expansion). This repo is the workspace root: docs, art, tooling, and setup. The actual engine changes live in two companion repos, both on their `synth` branch:

- [pokemon-synth-cfru](https://github.com/harrisonhjohnson/pokemon-synth-cfru) — SYNTH core engine, synth menu, config/table changes
- [pokemon-synth-dpe](https://github.com/harrisonhjohnson/pokemon-synth-dpe) — SYNTH species line (sprites, stats, dex data)

No ROMs are committed anywhere. The vanilla Fire Red base is **built from source** via [pret/pokefirered](https://github.com/pret/pokefirered) — no dump needed.

## Repo map

| Path | What it is |
|---|---|
| `docs/handoff.md` | Build protocol, current state, and gotchas — **read this first** |
| `docs/design/` | Design specs (synth system, menu, gym leaders, story, postgame, QoL) |
| `art/` | Concept art and sprite sources |
| `setup.sh` | Clones sub-repos and builds the vanilla base ROM |
| `env.sh` | PATH setup for devkitARM + local tools (`source env.sh` before building) |
| `bin/` | Audio conversion tools (mid2agb; wav2agb built from [ipatix/wav2agb](https://github.com/ipatix/wav2agb)) |

`FRLG-Plus/` may exist locally as an exploratory alternate base; it is not part of the build.

## One-time setup

1. Install devkitARM to `/opt/devkitpro` via [devkitPro pacman](https://devkitpro.org/wiki/Getting_Started), plus `python3`, `pip install pillow`, and libpng.
2. `./setup.sh` — clones CFRU/DPE (synth branches) + pret/pokefirered + pret/agbcc, builds the compiler and the vanilla base ROM, and seeds `DPE/BPRE0.gba`.

## Build the SYNTH ROM

Order matters: DPE first, then CFRU.

```sh
cd "$(git rev-parse --show-toplevel)" && source env.sh

cd DPE && rm -f generatedrepoints && python3 scripts/make.py
cd ../CFRU && rm -f generatedrepoints && cp ../DPE/test.gba BPRE0.gba && python3 scripts/make.py
cp test.gba ../synth-base.gba
```

Gotchas (full list in `docs/handoff.md`):

- If a header or `config.h` changed since the last build, `rm -rf build` in that repo first — incremental builds silently reuse stale constants.
- **Always** `rm -f generatedrepoints` before each insertion; a stale repoint cache corrupts the ROM.
- Never run insertion against a dummy/fake ROM.

## Verify, then play

1. `xxd -s 0xA0 -l 12 CFRU/test.gba` must read `POKEMON FIRE`.
2. Spot-check any bytes patched this session (see handoff doc).
3. Open `synth-base.gba` in [mGBA](https://mgba.io/).

Moves/abilities are stamped at Pokémon creation — learnset/stat changes need a New Game to observe.

## QA notes

Log findings in `docs/handoff.md` (state table + Gotchas section) so the next person inherits them.
