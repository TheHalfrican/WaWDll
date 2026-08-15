# Auto-injection

Injects `WaWDll.dll` into Call of Duty: World at War automatically when you launch
the game from Steam.

**Nothing is installed and nothing runs at boot.** There is no scheduled task and no
resident watcher. A supervisor starts when Steam launches the game and always ends by
itself: it exits when the game exits, or gives up after five minutes if the game never
appears.

## Setup

One step. In Steam: **Library → Call of Duty: World at War → gear icon → Properties →
Launch Options**, and paste:

```
cmd /c call "<path to this repo>\AutoInject\Trigger.cmd" %command%
```

The `call` keyword and the absolute path both matter. The `%command%` token is what
Steam replaces with the real game command line — without it, `Trigger.cmd` falls back
to launching the game from a hardcoded path and says so in `trigger.log`.

That is it. Launch the game normally and press **INSERT** once you are in.

## Multiplayer is never touched

The supervisor matches `CoDWaW.exe` exactly — the single-player and Nazi Zombies
executable. Multiplayer runs as `CoDWaWmp.exe` and is never matched, so picking
"Play Multiplayer" from Steam's chooser launches a completely untouched game.

## Pieces

| File | What it does |
| --- | --- |
| `Trigger.cmd` | Steam's entry point. Starts the supervisor, then launches the game and blocks until it exits, so Steam still tracks play time correctly. |
| `WaW_AutoInject.ps1` | Waits for the game window, lets it settle, runs the injector, then watches until the game closes. Holds a named mutex so only one runs per session. |
| `Inject-Now.cmd` | Double-click to inject into a game that is already running, or one you launched without the launch option. |
| `..\Debug\WaWInjector.exe` | The actual injector. `CreateRemoteThread` + `LoadLibraryA`. Can be run by hand — pass `-h` for options. |

## Turning it off

Create an empty file called `disabled.flag` in this folder. `Trigger.cmd` will then
launch the game and skip injection entirely, leaving the launch option in place.
Delete the file to re-enable.

## When something goes wrong

Two logs, both in this folder and both gitignored:

- `trigger.log` — did Steam invoke the trigger at all, and what did it pass through
- `autoinject.log` — what the supervisor and injector did, including injector output

Common causes:

- **Nothing in `trigger.log`** — the launch option is wrong or missing.
- **`OpenProcess failed`** — the game is running elevated. Run Steam unelevated, or
  run `Inject-Now.cmd` as administrator.
- **`LoadLibraryA returned NULL`** — the DLL is there but could not load. Rebuild
  with `build.cmd`.
- **Game crashes on injection** — the hardcoded addresses in the DLL do not match
  your `CoDWaW.exe`. That is a DLL problem, not an injector problem.

## Timing

Defaults live at the top of `WaW_AutoInject.ps1`: it ignores a game process younger
than 3 seconds, waits for a real window, then settles 6 seconds before injecting.
Raise `$SettleSecs` if injection lands before the game is ready.
