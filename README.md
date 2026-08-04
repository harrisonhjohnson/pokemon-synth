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

## One-time setup

All platforms need: devkitARM at `/opt/devkitpro`, `python3` with `pillow` (`pip install pillow`), libpng, and [mGBA](https://mgba.io/) to play. Then run `./setup.sh` — it clones pret/pokefirered + pret/agbcc (public), builds the compiler and the vanilla base ROM, and seeds `DPE/BPRE0.gba`.

### macOS (primary dev platform — Apple Silicon tested)

1. Install [devkitPro pacman](https://devkitpro.org/wiki/Getting_Started), then `sudo dkp-pacman -S gba-dev`.
2. `brew install libpng` and `pip3 install pillow`.
3. `./setup.sh`

### Windows

Use **WSL2 (Ubuntu)** — the pret base-ROM build and these shell scripts assume a Unix environment, and WSL is far smoother than MSYS2 for the full chain:

1. Install WSL2 + Ubuntu, clone this repo *inside* WSL (not on `/mnt/c` — builds are much slower there and line endings can break scripts).
2. Follow the Linux steps below inside WSL.
3. Run mGBA on the Windows side; the built `synth-base.gba` is reachable from Explorer via `\\wsl$`.

Native Windows/MSYS2 is possible (CFRU's own `CFRU/INSTALL.md` documents it) but you'd still need a separate path for the pret base-ROM build — not recommended.

### Linux (or WSL2)

1. Install [devkitPro pacman](https://devkitpro.org/wiki/devkitPro_pacman), then `sudo dkp-pacman -S gba-dev` (installs to `/opt/devkitpro`).
2. `sudo apt install build-essential libpng-dev python3-pip && pip3 install pillow`
3. `./setup.sh`

### Platform notes

- `bin/mid2agb` and `bin/wav2agb` are **macOS arm64 binaries**. They're only needed for audio conversion (adding new music/cries), not for the standard build. On PC, build them from source: wav2agb from [ipatix/wav2agb](https://github.com/ipatix/wav2agb), mid2agb from the pret pokefirered `tools/` directory (built automatically by its `make`).
- `env.sh` prepends `/opt/devkitpro/devkitARM/bin` and this repo's `bin/` to PATH — correct on macOS and Linux/WSL alike.
- Launching mGBA: macOS `open -a mGBA synth-base.gba`; Linux `mgba-qt synth-base.gba`; Windows just open the file in mGBA.

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
