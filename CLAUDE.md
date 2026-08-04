# Pokémon SYNTH — agent guide

Fire Red ROM hack: vendored CFRU (engine) + DPE (species) with SYNTH changes. Design bible in `docs/design/` (`claude.md` there is the project guide); living build/state doc in `docs/handoff.md` — read its Gotchas section before debugging any build failure.

## Setup (first run on a new machine)

Run `./setup.sh`. It needs devkitARM at `/opt/devkitpro` (devkitPro pacman), `python3` with `pillow`, and libpng. It clones pret/pokefirered + pret/agbcc and builds the vanilla base ROM from source — never ask the user for a ROM dump. On Windows, work inside WSL2 (repo cloned in the WSL filesystem, not /mnt/c); `bin/` binaries are macOS arm64 — on other platforms build wav2agb/mid2agb from source if audio conversion is needed (see README platform notes).

## Build chain (order matters: DPE first, then CFRU)

```sh
source env.sh   # from repo root
cd DPE && rm -f generatedrepoints && python3 scripts/make.py
cd ../CFRU && rm -f generatedrepoints && cp ../DPE/test.gba BPRE0.gba && python3 scripts/make.py
cp test.gba ../synth-base.gba
```

## Hard rules

- `rm -f generatedrepoints` before EVERY insertion in both repos — a stale repoint cache silently corrupts the ROM.
- If any `.h` or `config.h` changed since a repo's last build: `rm -rf build` there first (incremental build keeps stale NUM_SPECIES/flags).
- Never run insertion against a dummy/fake ROM (it poisons the repoint cache).
- DPE table files can hold MULTIPLE arrays — when editing, verify which array an entry lands in; never append blindly.
- Verify before declaring a build good: `xxd -s 0xA0 -l 12 CFRU/test.gba` must read `POKEMON FIRE`, then spot-check any bytes patched this session.

## QA loop

Launch: `open -a mGBA synth-base.gba` (back up `synth-base.sav` first if a fresh save is needed). Moves/abilities are stamped at mon creation — learnset/stat changes require a New Game to observe. Log findings and new gotchas in `docs/handoff.md`.
