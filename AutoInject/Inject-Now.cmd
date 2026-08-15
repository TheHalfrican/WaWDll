@echo off
REM Double-click to inject WaWDll.dll into an already-running World at War.
REM Runs unelevated, same as the game does when launched from Steam normally.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Inject-Now.ps1"
