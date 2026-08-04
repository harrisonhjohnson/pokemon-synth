#!/bin/sh
# One-time setup: clone public upstreams and build the vanilla base ROM.
# Prereq: devkitARM at /opt/devkitpro (install via devkitPro pacman),
# plus: python3, pillow (pip install pillow), libpng.
set -e
cd "$(dirname "$0")"

[ -d agbcc ] || git clone https://github.com/pret/agbcc.git
[ -d pokefirered ] || git clone https://github.com/pret/pokefirered.git

# Build the compiler and the vanilla Fire Red base ROM from source (no dump needed)
if [ ! -f pokefirered/pokefirered.gba ]; then
  (cd agbcc && ./build.sh && ./install.sh ../pokefirered)
  (cd pokefirered && make -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)")
fi
cp pokefirered/pokefirered.gba DPE/BPRE0.gba

echo "Setup done. Next: see README.md 'Build the SYNTH ROM'."
