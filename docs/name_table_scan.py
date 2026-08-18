"""Byte-level inspector for the DPE-repointed gSpeciesNames table and for
FireRed-charmap name strings inside a ROM / .sav.

Used 2026-08-17 to settle the "Mon" nameplate question (see docs/handoff.md).

  python3 docs/name_table_scan.py entry synth-base.gba 165C268 50E 2
  python3 docs/name_table_scan.py find Mon,Synth,Synth2 synth-base.gba synth-base.sav
  python3 docs/name_table_scan.py names synth-base.sav
"""
import sys
import pathlib

# FireRed charmap: 0x00=space, 0xA1-0xAA='0'-'9', 0xBB-0xD4='A'-'Z',
#                  0xD5-0xEE='a'-'z', 0xFF=EOS
NAME_STRIDE = 11  # POKEMON_NAME_LENGTH (10) + 1


def enc(s):
    out = bytearray()
    for c in s:
        if c == ' ':
            out.append(0x00)
        elif c.isdigit():
            out.append(0xA1 + int(c))
        elif 'A' <= c <= 'Z':
            out.append(0xBB + ord(c) - 65)
        elif 'a' <= c <= 'z':
            out.append(0xD5 + ord(c) - 97)
        else:
            raise ValueError('no charmap entry for %r' % c)
    return bytes(out)


def dec(b):
    out = ''
    for x in b:
        if x == 0xFF:
            break
        elif x == 0x00:
            out += ' '
        elif 0xA1 <= x <= 0xAA:
            out += chr(ord('0') + x - 0xA1)
        elif 0xBB <= x <= 0xD4:
            out += chr(65 + x - 0xBB)
        elif 0xD5 <= x <= 0xEE:
            out += chr(97 + x - 0xD5)
        else:
            out += '.'
    return out


def cmd_find(words, files):
    for f in files:
        d = pathlib.Path(f).read_bytes()
        print('%s (%d bytes)' % (f, len(d)))
        for w in words:
            p = enc(w)
            hits = []
            i = d.find(p)
            while i != -1 and len(hits) < 16:
                hits.append(hex(i))
                i = d.find(p, i + 1)
            print('   %-10s pattern=%s count=%d %s' % (w, p.hex(), len(hits), hits))


def cmd_entry(path, base, idx, n):
    d = pathlib.Path(path).read_bytes()
    for k in range(n):
        off = base + (idx + k) * NAME_STRIDE
        raw = d[off:off + NAME_STRIDE]
        print('idx 0x%03X @ 0x%07X  %s  %r' % (idx + k, off, raw.hex(' '), dec(raw)))


def cmd_names(path):
    """List every uppercase-initial, 0xFF-terminated name-like run.

    BoxPokemon.nickname is NOT encrypted, so this surfaces real save nicknames.
    """
    d = pathlib.Path(path).read_bytes()
    seen = {}
    i = 0
    while i < len(d):
        if 0xBB <= d[i] <= 0xD4:
            j = i
            while j < len(d) and (0xBB <= d[j] <= 0xEE or 0xA1 <= d[j] <= 0xAA):
                j += 1
            if 3 <= j - i <= 10 and j < len(d) and d[j] == 0xFF:
                seen.setdefault(dec(d[i:j]), []).append(hex(i))
            i = j
        else:
            i += 1
    for s in sorted(seen):
        print('%-12s x%-3d %s' % (s, len(seen[s]), seen[s][:6]))


if __name__ == '__main__':
    mode = sys.argv[1]
    if mode == 'find':
        cmd_find(sys.argv[2].split(','), sys.argv[3:])
    elif mode == 'entry':
        cmd_entry(sys.argv[2], int(sys.argv[3], 16), int(sys.argv[4], 16),
                  int(sys.argv[5]) if len(sys.argv) > 5 else 1)
    elif mode == 'names':
        cmd_names(sys.argv[2])
    else:
        raise SystemExit(__doc__)
