"""Finds a dvar in the running game and says how it stores its value.

Why this exists
---------------
GameData::DvarValue is a union. `current.integer` and `current.value` are the
same four bytes, and nothing in the struct says which one the engine reads.
Guessing wrong is not a subtle bug: writing int 39 into a float dvar stores
5.5e-44, and writing float 190.0 into an int dvar stores 1130102784.

The two dvars this repo drives from the menu do not even agree with each other:

    g_speed      int    stock 190
    jump_height  float  stock 39
    g_gravity    float  stock 800
    cg_fov       float  stock 65

So check, do not assume.

How it works
------------
Finds the dvar's name string in the process, then finds the dvar_s whose name
pointer at +0x00 points at it, and decodes current (+0x10) and reset (+0x30)
both ways. The decoding that shows a sane stock value is the union member the
engine uses. Real dvars live in a static pool and have small, plausible flags;
the false positives are misaligned matches inside code, which is what the
filtering below removes.

Usage
-----
    python Tools/dvar_probe.py g_speed jump_height
    python Tools/dvar_probe.py --all g_speed      # keep the rejected candidates
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game_process import Game

DVAR_SIZE = 0x5C
OFF_NAME, OFF_DESC, OFF_FLAGS = 0x00, 0x04, 0x08
OFF_CURRENT, OFF_RESET = 0x10, 0x30

# Do NOT filter on flags. They carry more than the registration bits: setting
# g_speed from the menu flipped its flags from 0x00053000 to 0x01053000, so a
# tight mask rejects exactly the dvars somebody has changed, which are the ones
# worth looking at. The description pointer is the honest discriminator -- a
# real dvar_s points at a readable ASCII sentence, and a misaligned match
# inside code essentially never does.
def plausible(game, blob):
    desc = struct.unpack_from("<I", blob, OFF_DESC)[0]
    if desc == 0:
        return True
    if not 0x00400000 <= desc < 0x7FFF0000:
        return False

    text = game.read(desc, 64)
    if not text:
        return False
    text = text.partition(bytes(1))[0]
    if not text:
        return True
    return all(0x20 <= c < 0x7F for c in text)


def decode(blob, off):
    return (struct.unpack_from("<i", blob, off)[0],
            struct.unpack_from("<f", blob, off)[0])


def looks_like_a_value(i, f):
    """Which decoding reads as something a human would have typed."""
    votes = []
    if -100000 <= i <= 100000:
        votes.append("int")
    if f != 0.0 and abs(f) >= 1e-3 and abs(f) <= 1e6:
        votes.append("float")
    elif f == 0.0:
        votes.append("float")
    return votes


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    show_all = "--all" in sys.argv
    if not args:
        args = ["g_speed", "jump_height", "g_gravity", "cg_fov"]

    game = Game()
    print("pid %d, %d committed regions" % (game.pid, len(game.regions())))

    for name in args:
        print("\n=== %s ===" % name)
        strings = game.scan(name.encode() + b"\x00")
        if not strings:
            print("  name string not found. The dvar is not registered yet --")
            print("  server game dvars only appear once a map has loaded.")
            continue

        seen = set()
        found = False
        for s in strings:
            for addr in game.scan(struct.pack("<I", s)):
                if addr in seen:
                    continue
                seen.add(addr)
                blob = game.read(addr, DVAR_SIZE)
                if not blob or len(blob) < DVAR_SIZE:
                    continue
                if not plausible(game, blob) and not show_all:
                    continue

                flags = struct.unpack_from("<I", blob, OFF_FLAGS)[0]
                ci, cf = decode(blob, OFF_CURRENT)
                ri, rf = decode(blob, OFF_RESET)
                print("  dvar_s @ 0x%08X  flags=0x%08X" % (addr, flags))
                print("    current   int=%-12d  float=%g" % (ci, cf))
                print("    reset     int=%-12d  float=%g" % (ri, rf))
                votes = looks_like_a_value(ri, rf)
                if len(votes) == 1:
                    print("    -> reset reads as %s; write current.%s"
                          % (votes[0],
                             "integer" if votes[0] == "int" else "value"))
                else:
                    print("    -> ambiguous; compare reset against the dvar's"
                          " known stock value by hand")
                found = True

        if not found:
            print("  no plausible dvar_s referenced the name string.")
            print("  re-run with --all to see the rejected candidates.")


if __name__ == "__main__":
    main()
