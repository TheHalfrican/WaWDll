# Tools

Small probes for answering questions about the game by reading it while it
runs. They are diagnostics, not part of the build, and nothing in `WaWDll/`
depends on them.

## Why these exist

`CoDWaW.exe` on disk is Steam DRM packed. The `.bind` section decrypts `.text`
at load, so reading bytes from the file returns garbage no matter how correct
the PE section maths is. The only honest source for anything about this game is
the running process.

Beyond that, much of the struct layout in this repo came from the original
author as *labels*, and several have turned out wrong. When code is about to
depend on one, it is cheaper to check than to debug the consequences.

Both tools have already earned their place:

- `dvar_probe.py` established that `g_speed` is an **int** dvar while
  `jump_height` and `g_gravity` are **floats**. `DvarValue` is a union, so
  writing the wrong member stores a denormal: int `39` into a float dvar is
  5.5e-44, and the player simply cannot jump. Copy-pasting the Move Speed code
  for jump height would have shipped exactly that bug.
- `field_watch.py` settled `playerState_s::fWeaponPosFrac` at `0x0110`, which
  the aimbot's toggle-ADS path depends on and which had never been verified.

## Requirements

Python 3, and the game running. No third-party packages; everything goes
through `ctypes` and `kernel32`. If `OpenProcess` fails, run from an elevated
shell.

## `dvar_probe.py`

Finds a dvar and reports how it stores its value.

```
python Tools/dvar_probe.py g_speed jump_height
python Tools/dvar_probe.py --all g_speed      # keep rejected candidates
```

It finds the dvar's name string, then the `dvar_s` whose name pointer at `+0x00`
points at it, and decodes `current` (`+0x10`) and `reset` (`+0x30`) both ways.
The decoding that reads as a sane stock value is the union member the engine
uses.

Server game dvars such as `g_speed` are not registered until a map has loaded,
so probe from inside a match, not the main menu.

Do not filter candidates on `flags`. Setting `g_speed` from the menu flipped its
flags from `0x00053000` to `0x01053000`, so a tight mask rejects exactly the
dvars somebody has changed. The tool discriminates on the description pointer
instead, which a real dvar has and a misaligned match inside code does not.

Known values, all confirmed live:

| dvar | address | stored as | stock |
| --- | --- | --- | --- |
| `g_speed` | `0x021D7804` | int | 190 |
| `jump_height` | `0x021CD3C8` | float | 39 |
| `g_gravity` | `0x021D7860` | float | 800 |
| `cg_fov` | `0x021C4F88` | float | 65 |

## `field_watch.py`

Samples a window of struct fields over time and prints which ones moved, with a
glyph trace so a 0-to-1 swing is obvious at a glance.

```
# Default window, around fWeaponPosFrac. Aim while it runs.
python Tools/field_watch.py

python Tools/field_watch.py --base 0x351DF50 --lo 0x0F0 --hi 0x140 --secs 20
python Tools/field_watch.py --base 0x351DF50 --lo 0 --hi 0x40 --as int
```

Watch a window rather than the single labelled offset. If the label is wrong,
this finds the field that is right, instead of only telling you the label is
wrong.

Stand still while sampling. Running around moves position and velocity fields
and buries the one you are looking for in noise.
