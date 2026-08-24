"""Watches a window of struct fields in the running game and shows what moves.

Why this exists
---------------
Most of the struct layouts in this repo came from the original author and are
labels, not facts. Several have turned out wrong. When a label matters -- when
code is about to depend on it -- the cheap way to settle it is to watch a range
of offsets while doing the thing the field claims to describe, and see which
one actually responds.

Watching a *window* rather than the single labelled offset matters: if the
label is wrong, this finds the field that is right instead of only telling you
the label is wrong.

This settled playerState_s::fWeaponPosFrac. Sampled while alternating hip and
sights, ps+0x110 swung cleanly 0.0000 to 1.0000 and was the only field in
ps+0x0F0..0x140 that moved at all, so there was no rival candidate.

Usage
-----
    # The default window, around fWeaponPosFrac. Aim while it runs.
    python Tools/field_watch.py

    # Any window, as ints or floats.
    python Tools/field_watch.py --base 0x351DF50 --lo 0x0F0 --hi 0x140 --secs 20
    python Tools/field_watch.py --base 0x351DF50 --lo 0 --hi 0x40 --as int
"""
import argparse
import os
import struct
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from game_process import Game

# cgameGlob 0x34732B8 + predictedPlayerState 0xAAC98. fWeaponPosFrac is +0x110
# from here, i.e. 0x351E060 absolute.
PREDICTED_PLAYER_STATE = 0x34732B8 + 0xAAC98

# Low to high. A field pinned at one value prints as a flat run.
GLYPHS = " .-+#"


def trace(values, lo, span, width=60):
    step = max(1, len(values) // width)
    return "".join(
        GLYPHS[min(len(GLYPHS) - 1, int((v - lo) / span * (len(GLYPHS) - 0.001)))]
        for v in values[::step])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", type=lambda s: int(s, 0),
                    default=PREDICTED_PLAYER_STATE,
                    help="struct base address (default: predictedPlayerState)")
    ap.add_argument("--lo", type=lambda s: int(s, 0), default=0x0F0,
                    help="first offset in the window")
    ap.add_argument("--hi", type=lambda s: int(s, 0), default=0x140,
                    help="last offset in the window")
    ap.add_argument("--secs", type=float, default=20.0)
    ap.add_argument("--hz", type=float, default=20.0)
    ap.add_argument("--as", dest="kind", choices=("float", "int"),
                    default="float")
    args = ap.parse_args()

    size = args.hi - args.lo + 4
    count = size // 4
    fmt = "<%d%s" % (count, "f" if args.kind == "float" else "i")

    game = Game()
    if game.read(args.base + args.lo, size) is None:
        sys.exit("Cannot read 0x%08X. Is a match actually loaded?"
                 % (args.base + args.lo))

    print("pid %d   window 0x%08X..0x%08X   (base+0x%03X..0x%03X, as %s)"
          % (game.pid, args.base + args.lo, args.base + args.hi,
             args.lo, args.hi, args.kind))
    print("sampling %.0fs at %.0fHz -- exercise the thing you are testing now\n"
          % (args.secs, args.hz))

    series = []
    interval = 1.0 / args.hz
    end = time.time() + args.secs
    while time.time() < end:
        raw = game.read(args.base + args.lo, size)
        if raw and len(raw) == size:
            series.append(struct.unpack(fmt, raw))
        time.sleep(interval)

    if not series:
        sys.exit("No samples read.")
    print("%d samples\n" % len(series))
    print("  offset       min          max        range   trace (%s)"
          % "".join(GLYPHS).replace(" ", "_"))

    moved = 0
    for i in range(count):
        col = [s[i] for s in series]
        lo, hi = min(col), max(col)
        span = hi - lo
        if span <= 1e-6:
            continue
        moved += 1
        print("  +0x%03X %11.4f %12.4f %12.4f   %s"
              % (args.lo + i * 4, lo, hi, span, trace(col, lo, span)))

    if not moved:
        print("  nothing in this window moved.")
    else:
        print("\n%d of %d fields moved. A field that tracks what you were"
              % (moved, count))
        print("doing is the candidate; one that moved alone is a strong one.")


if __name__ == "__main__":
    main()
