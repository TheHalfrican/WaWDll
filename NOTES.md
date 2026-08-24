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

## Move Speed

Drives `g_speed`, the engine's base movement speed in units per second, which
walk, sprint and crouch all scale off. Stock is 190, which is also the option's
off position, so there is no separate boolean to fall out of sync with it.

Two things about `g_speed` shaped the implementation, both confirmed live rather
than assumed:

- **It is an int dvar**, so `current.integer` is the union member to write, not
  `current.value`. Probed in a live match: `dvar_s` at `0x021D7804` held
  `int=190`, while `cg_fov` in the same pool held `float=65`, matching the
  existing FOV code. Writing the wrong member lands a denormal and the player
  cannot move at all.
- **It belongs to the server game module**, so it is not registered until a map
  loads and cannot go in the `InsertDvar` startup chain, which would hit the
  `Com_Error` at injection. It is resolved on demand and cached.

`Menu::EnforceMoveSpeed` re-asserts the value from the render thread, because
`g_speed` is cheat protected and the engine resets it on map load. Gated so an
option left at stock costs one integer compare per frame and never writes. This
is the same shape as BO3Z's run speed, where entering a new match rebuilt the
player struct at the default.

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
| `build.cmd` | Builds the solution as Debug x86 |
| `WaWDll.cfg` | Written beside the built DLL at runtime; delete to reset settings |
