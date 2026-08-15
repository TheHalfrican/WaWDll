@echo off
REM Invoked by the Steam launch option for Call of Duty: World at War:
REM
REM   cmd /c call "<this file>" %command%
REM
REM Starts the injection supervisor, then hands off to the game, passing through
REM whatever arguments Steam supplied. Because the game runs from inside this script,
REM Steam still sees a process to track, so play time and "Stop" behave normally.
REM
REM NOTHING IS INSTALLED AND NOTHING RUNS AT BOOT. There is no scheduled task and no
REM resident watcher. The supervisor starts here, and exits when the game exits (or
REM after five minutes if the game never appears).
REM
REM NO ELEVATION. CoDWaW.exe runs as the same user at the same integrity level, so
REM OpenProcess/WriteProcessMemory work fine unelevated. If you ever run Steam as
REM administrator, run the injector as administrator too or it cannot open the game.
REM
REM The "%command%" passthrough form is used rather than "trigger & game" on purpose:
REM it keeps the game as a child of this script. (The cmd AutoRun on this machine used
REM to split launch options on "&"; that has since been fixed in
REM %USERPROFILE%\.config\cmd-autorun.cmd, but the passthrough form is still cleaner.)

set "LOG=%~dp0trigger.log"
set "SUPERVISOR=%~dp0WaW_AutoInject.ps1"

REM Only reached when the launch option is missing its %%command%% token and the
REM game has to be started directly. Steam libraries live on whichever drive the
REM user put them on, so check the usual layouts rather than hardcoding a path.
set "FALLBACK="
set "REL=steamapps\common\Call of Duty World at War\CoDWaW.exe"
for %%D in (C D E F G H S T U V W X Y Z) do (
    if not defined FALLBACK if exist "%%D:\" (
        if not defined FALLBACK if exist "%%D:\SteamLibrary\%REL%" set "FALLBACK=%%D:\SteamLibrary\%REL%"
        if not defined FALLBACK if exist "%%D:\Steam\%REL%" set "FALLBACK=%%D:\Steam\%REL%"
        if not defined FALLBACK if exist "%%D:\Program Files (x86)\Steam\%REL%" set "FALLBACK=%%D:\Program Files (x86)\Steam\%REL%"
    )
)

>>"%LOG%" echo(
>>"%LOG%" echo ===== trigger invoked %DATE% %TIME% =====
>>"%LOG%" echo   cwd      = %CD%
>>"%LOG%" echo   passthru = %*

if exist "%~dp0disabled.flag" (
    >>"%LOG%" echo   BYPASSED - disabled.flag present, DLL NOT injected.
) else (
    >>"%LOG%" echo   starting supervisor: %SUPERVISOR%
    start "" /b powershell -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "%SUPERVISOR%"
)

if "%~1"=="" (
    >>"%LOG%" echo   WARNING: no game arguments received.
    >>"%LOG%" echo   The Steam launch option is probably missing the %%command%% token.
    >>"%LOG%" echo   Expected: cmd /c call "%~f0" %%command%%
    if exist "%FALLBACK%" (
        >>"%LOG%" echo   Falling back to launching the game directly.
        "%FALLBACK%"
        >>"%LOG%" echo ===== game exited %DATE% %TIME% =====
    ) else (
        >>"%LOG%" echo   ERROR: fallback not found at %FALLBACK%
    )
    exit /b 0
)

>>"%LOG%" echo   launching game...
%*
>>"%LOG%" echo ===== game exited %DATE% %TIME% =====
exit /b 0
