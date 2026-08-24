# Status and open items

Working notes. Last updated 2026-08-24. See [CLAUDE.md](CLAUDE.md) for build and
tooling guidance.

## Confirmed working in game

- Injection, and auto-injection through the Steam launch option
- Insert opens and closes the menu, with no ReShade collision
- God Mode, Infinite Ammo, Enable Cheats
- Aimbot
- Clicking and dragging across options no longer fires the ones passed over
- No more Panzerschreck rockets on every shot
- On/Off indicators on God Mode, No Clip and No Target
- Aim Key 2, restricting the aimbot to aiming down sights
- Settings persistence across a full game restart, via `WaWDll.cfg`
- Console starting minimised rather than stealing focus, after two earlier
  approaches failed
- **Move Speed**, including sprint scaling with it and the value surviving the
  start of a new match
- **Jump Height** and **Gravity**

## Confirmed by reading the live process

`playerState_s::fWeaponPosFrac` at offset `0x0110` **is** the ADS transition
fraction, 0 at the hip and 1 fully sighted. This was the original author's
label rather than something confirmed, and this codebase has had several
mislabelled things, so it was sampled directly:

```
0x351E060   =   cgameGlob 0x34732B8  +  predictedPlayerState 0xAAC98  +  fWeaponPosFrac 0x110
```

Polled at 20Hz for 20 seconds while alternating hip and sights, it swung
cleanly between 0.0000 and 1.0000 in step with the sights, and it was the only
field in the whole `ps+0x0F0` to `ps+0x140` window that moved at all, so there
is no rival candidate. Toggle ADS support rests on this, and it holds.

Note that Aim Key 2 working in game did not settle this on its own: hold to ADS
is satisfied by the key check alone and would behave identically with the label
wrong.

## Known incomplete, by design

- **Mouse only responds while the game is paused.** `MonitorMouse` reads
  `uiDC->cursorPos`, which the engine only updates while its own UI layer is
  active. Noah has said he is fine with this. Fixing it properly is upstream's
  listed future item, rendering the cursor while the menu is open.
- **No Spread** flips its own checkbox and nothing reads it. A fully written
  `Aimbot::RemoveSpread` exists in `aimbot.cpp` and is never called from
  anywhere. Noah asked to leave it untouched and wire it up another time.
- **Friendly ESP** is unimplemented; nothing reads it. Little use in solo
  Zombies.
- **Host menu is unfinished.** `VM_NotifyDetour` is kept installed as its entry
  point but is deliberately side effect free. It fires on every script notify in
  the game, so anything added there must be gated behind a menu option. This is
  where the Panzerschreck bug lived.
- **`ocmd->serverTime++`** is now inside the Auto Shoot branch. The reading that
  it stops the server discarding the retro-edited command as stale is inferred
  from the code's shape, not traced through the netcode.

## The enforced dvar options

Move Speed, Jump Height and Gravity all run through the `enforcedDvars` table in
`nonhost_menu.cpp`, which owns their ranges and stock values.

| Option | dvar | stock | range | step | written as |
| --- | --- | --- | --- | --- | --- |
| Move Speed | `g_speed` | 190 | 190-600 | 10 | **int** |
| Jump Height | `jump_height` | 39 | 39-839 | 50 | **float** |
| Gravity | `g_gravity` | 800 | 50-800 | 50 | **float** |

Each stock doubles as the option's off position, so none of them has a separate
boolean that can fall out of sync with the number. Gravity's stock is the top of
its range, since lower is floatier.

Three things shaped this, all confirmed by reading the live process rather than
assumed. Use `Tools/dvar_probe.py`.

- **Which union member to write is not guessable, and is not consistent.**
  `g_speed` is an int dvar while `jump_height` and `g_gravity` are floats. That
  is what the `isFloat` column exists for. Writing the wrong member stores a
  denormal: an int `39` into `jump_height` is 5.5e-44 and the player cannot
  jump at all.
- **They belong to the server game module**, so they are not registered until a
  map loads and cannot go in the `InsertDvar` startup chain, which would hit
  the `Com_Error` at injection. They are resolved on demand and cached.
- **They are cheat protected**, so the engine resets them on map load.
  `Menu::EnforceDvars` re-asserts them from the render thread, gated so an
  option left at stock costs one integer compare per frame and never writes.
  Same shape as BO3Z's run speed, where entering a new match rebuilt the player
  struct at the default.

**Gravity multiplies Jump Height rather than being independent of it.** Observed
in game: the lower the gravity, the higher the jump as well as the longer the
hang time, and at stock gravity raising Jump Height alone does much less than
expected. They are meant to be tuned together. The mechanism has not been traced
through the movement code, so treat the interaction as observed behaviour and
nothing more.

An earlier pass gave Jump Height and Gravity fine steps over narrow ranges. The
extremes were then 20 to 30 clicks away, nobody reached them, and both options
felt like they did nothing.

## Possible next steps

- Wire up No Spread to the existing `RemoveSpread`
- Render the game cursor while the menu is open, so it works unpaused
- Audit the remaining options that have never been exercised, on the assumption
  that unconditional side effects are still lurking

## Repo layout

| Path | What it is |
| --- | --- |
| `WaWDll/` | The mod menu DLL |
| `Injector/` | Standalone 32-bit injector, `CreateRemoteThread` + `LoadLibraryA` |
| `AutoInject/` | Steam launch option auto-injection, see its own README |
| `Tools/` | Live-process probes, see its own README. Diagnostics, not built |
| `build.cmd` | Builds the solution as Debug x86 |
| `WaWDll.cfg` | Written beside the built DLL at runtime; delete to reset settings |
