# Status and open items

Working notes. Last updated 2026-08-14. See [CLAUDE.md](CLAUDE.md) for build and
tooling guidance.

## Confirmed working in game

- Injection, and auto-injection through the Steam launch option
- Insert opens and closes the menu, with no ReShade collision
- God Mode, Infinite Ammo, Enable Cheats
- Aimbot
- Clicking and dragging across options no longer fires the ones passed over
- No more Panzerschreck rockets on every shot

## Built but not yet tested

Everything here compiles and is in the pushed build, but Noah had not exercised
it before the session ended. **Ask how these went before building on them.**

1. **On/Off indicators** on God Mode, No Clip and No Target
2. **Aim Key 2**, restricting the aimbot to aiming down sights, with the fix
   that checks both aim bind names
3. **Settings persistence** across a full game restart, via `WaWDll.cfg`
4. **Console starting minimised** rather than stealing focus

Item 4 has already failed twice with different approaches. If minimising also
fails, the fallback is not creating the console at all in this build.

## Unverified assumption

The ADS check depends on `playerState_s::fWeaponPosFrac` at offset `0x0110`
being the ADS transition fraction, 0 at the hip and 1 fully sighted. **That is
the original author's label on the struct field, not something confirmed**, and
this codebase has had several mislabelled things.

To verify, with the game running and the user aiming, read:

```
0x351E060   =   cgameGlob 0x34732B8  +  predictedPlayerState 0xAAC98  +  fWeaponPosFrac 0x110
```

It should swing 0 to 1 as the sights come up. If the label is wrong, hold-to-ADS
still works through the key check and only toggle-ADS support is lost.

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

## Possible next steps

- Test the four items above and report back
- Confirm `fWeaponPosFrac`
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
