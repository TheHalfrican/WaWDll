[![Build status](https://ci.appveyor.com/api/projects/status/mekgh42kq15fjkv6?svg=true)](https://ci.appveyor.com/project/e7ite/wawdll)

# WaWDll
Simple mod menu for Call of Duty World at War on Steam for Windows. Reverse engineering this game has taught me so much about the x86 instruction set, the layout of Windows PE executables, etc. Using this knowledge I was able to find many game objects. I learned a ton of how the Call of Duty engine works, as well by doing so. This game has no ASLR so we are allowed to get static pointers to game objects in the .data segment, and game subroutines in the .text segment in the executable. I used Explorer Suite to dump the game, and I used IDA to reverse engineer the game and find all the information that I have in the project.

If you have any questions about this program or want any explanations to anything, feel free to message me on Discord @E7ite#1156

## Features
- Non-Host Mod Menu
- Support for All Resolutions
- Regular Aimbot for Zombies
- ESP (Extra Sensory Perception) for Zombies.
- No Recoil
- FOV
- Super Steady Aim
- God Mode
- No Clip
- No Target
- Infinite Ammo
- No Flinch

## Future Updates
- Host All-Client Mod Menu
- Rendering the game mouse cursor always when menu is open

## Controls
- Insert button opens and closes the menu
- Backspace exits submenus and menus
- Use mouse cursor to highlight options, and left-click to select.

## Changes in this fork
Solo offline Nazi Zombies only. Everything below is on top of e7ite's original.

### Fixes
- **Weapons fired Panzerschreck rockets.** The `VM_Notify` detour, installed
  unconditionally from `DllMain`, called `GScr_MagicBullet("panzerschrek")` on
  every `weapon_fired` script notify. Leftover experimentation from the
  unfinished host menu, gated by nothing, so it applied to every shot from every
  weapon the moment the DLL was injected. Detour is now side effect free.
- **ESP was always on.** `RenderESP()` ran every frame regardless of the Enemy
  ESP option, which only ever flipped its own checkbox. Now gated on it.
- **Aim Key 2 never worked.** It tested `Key_IsDown("+speed_throw")`, but World
  at War rebinds the aim key between `+speed_throw` and `+toggleads_throw`
  depending on the Aim Down Sight setting, so only one is ever bound. Both are
  checked now, with a `fWeaponPosFrac` fallback so it also holds under the
  toggle setting, where the key is down for a single moment.
- **Clicks were level triggered**, re-firing every 200ms while held, so a click
  that lingered or drifted ran every option it passed over. Now one action per
  press, except numeric options which keep hold to repeat.
- **`ocmd->serverTime++` ran every frame** in the `CL_CreateNewCommands` detour,
  ungated. Moved inside the Auto Shoot branch it belongs to.
- **The `dvars` map was keyed by `const char *`**, so lookups compared pointers
  and only worked because the compiler happened to pool identical literals.

### Changes
- Menu key moved from Home to **Insert**, which no longer collides with ReShade,
  and it now toggles rather than only opening.
- Menu settings persist to `WaWDll.cfg` beside the DLL.
- God Mode, No Clip and No Target show On/Off. The game owns the real state, so
  this reflects clicks made here, and is deliberately not persisted.
- Aim Key defaults to 2, restricting the aimbot to aiming down sights.
- Debug console starts minimised instead of yanking a fullscreen game out of
  focus.
- Removed Aim Type and Auto Aim, which were never read by anything. No Spread is
  also unread, but left in place.
- Added a 32 bit injector and Steam launch option auto injection. See
  [AutoInject/README.md](AutoInject/README.md).

## Build Instructions
1. Clone and build [Microsoft Detours](https://github.com/microsoft/Detours) for
   x86, then copy its headers into `include\detours\` so `<detours/detours.h>`
   resolves. Expected beside this repo as `..\Detours\` by default; override
   with the `DetoursDir` MSBuild property.
2. Run `build.cmd`, or build the solution as **Debug|x86** (the game is 32-bit).
   Debug is the only working configuration: `TopLevelExceptionFilter` is defined
   under `#ifdef _DEBUG` but referenced unconditionally, so Release will not link.
3. Launch the game and inject `WaWDll.dll`, either with `AutoInject` or your own
   injector.

Builds with Visual Studio Build Tools 2026 (toolset v145, Windows SDK 10.0.26100).

## Preview
![](/screenshot.png)
