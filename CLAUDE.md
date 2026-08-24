# CLAUDE.md

Guidance for working in this repo. Read [NOTES.md](NOTES.md) too, which carries
the current status and open items.

## What this is

A fork of [e7ite/WaWDll](https://github.com/e7ite/WaWDll), an injected mod menu
for Call of Duty: World at War. Used for **solo offline Nazi Zombies only**.

The injector matches `CoDWaW.exe` exactly, which is the single player and
Zombies executable. Multiplayer is `CoDWaWmp.exe` and is deliberately never
matched. Preserve that property if targeting is ever changed.

## Building

```
build.cmd
```

Wraps MSBuild for `Debug|x86`. Requirements:

- **Visual Studio Build Tools 2026**, toolset **v145**, Windows SDK
  **10.0.26100**. The upstream project asked for v141/v142 and SDK 17763, which
  are not installed; the vcxproj is retargeted.
- **Microsoft Detours** built for x86, expected at `..\Detours\` beside this
  repo. Override with the `DetoursDir` MSBuild property. The source includes
  `<detours/detours.h>`, but a Detours source build outputs `include\detours.h`,
  so the headers must also be copied into `include\detours\`.

**Debug is the only configuration that links.** `TopLevelExceptionFilter` is
defined under `#ifdef _DEBUG` in `nonhost_menu.cpp` but referenced
unconditionally in `dllmain.cpp`, so Release fails at link. This is upstream's
bug, not a retargeting artefact. Debug uses the static CRT (`/MTd`), so the DLL
imports only KERNEL32, USER32 and SHELL32 and needs no redistributables.

**The game must be closed to relink.** An injected DLL cannot be unloaded, so
the linker fails with `LNK1168: cannot open ... WaWDll.dll for writing` while
the game is running. Check for `CoDWaW` before building and ask the user to
close it.

## Tooling on this machine

Invoke `cmd.exe`, `vcvarsall.bat`, `nmake` and `MSBuild` through the
**PowerShell** tool. Through the Bash tool, MSYS path conversion mangles their
arguments: `/c` gets eaten, cmd starts interactive and hangs. For the same
reason, prefer Edit/Write over `sed` when a replacement contains backslashes,
which otherwise silently substitutes nothing. Bash is fine for grep, ls and cat.

## Reverse engineering

`CoDWaW.exe` on disk is **Steam DRM packed** (note the `.bind` section) and its
`.text` is encrypted, so reading bytes from the file returns garbage no matter
how correct the PE section maths is. That is why the original author dumped the
game from memory.

**Verify against the running process instead**, via
`OpenProcess(PROCESS_VM_READ)` and `ReadProcessMemory`. The module has no ASLR
and loads at `0x00400000`, which is what makes the hardcoded absolute addresses
work. Those addresses have been confirmed to match this build: all detour sites
hold `E9` jumps into the injected DLL's range, and both `WriteBytes` patch sites
hold their expected original bytes.

`Tools/` exists for this, and both probes have already caught something a guess
would have got wrong. Read [Tools/README.md](Tools/README.md) before writing a
new one.

- **`Tools/dvar_probe.py`** reports which member of the `DvarValue` union a dvar
  actually uses. This is not guessable and **not consistent between dvars**:
  `g_speed` is an int while `jump_height` and `g_gravity` are floats. Writing
  the wrong member stores a denormal, so an int `39` into `jump_height` is
  5.5e-44 and the player cannot jump. **Probe before adding any dvar backed
  option**, and note that server game dvars are not registered until a map has
  loaded, so probe from inside a match.
- **`Tools/field_watch.py`** samples a window of struct fields and prints which
  ones moved. This is how `fWeaponPosFrac` was finally confirmed. Watch a
  window, not the single labelled offset: if the label is wrong, this finds the
  field that is right instead of only telling you the label is wrong.

Much of the struct layout here is the original author's **labels, not facts**,
and several have turned out wrong. When code is about to depend on one, checking
costs a minute and guessing has repeatedly cost far more.

## The failure mode this codebase keeps having

Four separate defects shared one shape: **an unconditional side effect paired
with a menu toggle that nothing reads.** Weapons firing Panzerschreck rockets on
every shot, ESP always drawn, clicks re-firing while held, and an ungated write
to the client command ring buffer.

So for any "it does X and nobody asked it to" report, **grep for code that runs
unconditionally first** — in `DllMain`, in the detours, and in
`Menu_PaintAllDetour` — before theorising about what the user clicked. Several
options still only flip their own checkbox and are read by nothing; see
NOTES.md. Treat untested options as guilty until proven otherwise.

When suggesting a diagnostic, check it applies to Zombies specifically. A
suggestion to "look for a pile of weapons from `give all`" was useless because
Zombies caps the player at two weapon slots.

## Conventions

- 4 spaces, no tabs. Allman braces. Match the surrounding style.
- Comments explain **why**, especially for hand written assembly, hardcoded
  addresses, and anything that looks arbitrary. Much of the debugging pain here
  came from undocumented intent.
- Menu callbacks are click handlers that **toggle**, not apply-state functions.
  Restoring a saved value therefore needs `Menu::ApplySettings` to push it into
  the game; the value alone shows a checkbox that lies.
- Anything added to a detour runs for every event in the game. Gate it behind a
  menu option.
