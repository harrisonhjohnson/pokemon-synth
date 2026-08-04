# Pokémon SYNTH

A Fire Red ROM hack with customizable "Synth" species. Everything you need is in this one repo — the modified [CFRU](https://github.com/Skeli789/Complete-Fire-Red-Upgrade) engine, the modified [DPE](https://github.com/Skeli789/Dynamic-Pokemon-Expansion) species expansion, design docs, art, and build tooling. No ROM dump is needed: the vanilla base is built from source via [pret/pokefirered](https://github.com/pret/pokefirered).

**Using Claude Code?** Clone this repo, open Claude Code in it, and say *"set up and build the SYNTH ROM"* — `CLAUDE.md` has the full protocol and gotchas.

## Repo map

| Path | What it is |
|---|---|
| `CFRU/` | Complete Fire Red Upgrade + SYNTH engine (synth core, synth menu, patches) |
| `DPE/` | Dynamic Pokémon Expansion + SYNTH species (sprites, stats, dex data) |
| `docs/handoff.md` | Build protocol, current state, and gotchas — **read this first** |
| `docs/design/` | Design specs (synth system, menu, gym leaders, story, postgame, QoL) |
| `art/` | Concept art and sprite sources |
| `setup.sh` | Clones public upstreams (pret) and builds the vanilla base ROM |
| `env.sh` | PATH setup for devkitARM + local tools (`source env.sh` before building) |
| `bin/` | Audio conversion tools (mid2agb; wav2agb built from [ipatix/wav2agb](https://github.com/ipatix/wav2agb)) |

## One-time setup (macOS)

1. Install devkitARM to `/opt/devkitpro` via [devkitPro pacman](https://devkitpro.org/wiki/Getting_Started), plus `python3`, `pip install pillow`, and libpng (`brew install libpng`).
2. `./setup.sh` — clones pret/pokefirered + pret/agbcc (public), builds the compiler and the vanilla base ROM, and seeds `DPE/BPRE0.gba`.

## Build the SYNTH ROM

Order matters: DPE first (species data), then CFRU (engine) on top of DPE's output.

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

## Upstream provenance

`CFRU/` and `DPE/` are vendored snapshots with SYNTH changes on top. Upstream bases: CFRU master @ `b637a27` (Skeli789), DPE Unbound @ `fe058e0` (Skeli789). Full-history copies (upstream + synth branches) are archived at [pokemon-synth-cfru](https://github.com/harrisonhjohnson/pokemon-synth-cfru) and [pokemon-synth-dpe](https://github.com/harrisonhjohnson/pokemon-synth-dpe) if an upstream merge is ever needed.
