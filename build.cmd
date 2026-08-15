@echo off
setlocal
rem ---------------------------------------------------------------------------
rem Builds WaWDll as a 32-bit DLL (the game is x86).
rem
rem Requires Microsoft Detours, built for x86. By default it is expected at
rem   ..\Detours   (i.e. a sibling folder of this repo) with
rem   ..\Detours\include\detours\detours.h  and  ..\Detours\lib.X86\detours.lib
rem Override with:  set DetoursDir=C:\path\to\Detours\
rem ---------------------------------------------------------------------------

set "MSBUILD=%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD%" (
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"
)
if not exist "%MSBUILD%" (
    echo ERROR: could not locate MSBuild.exe. Install Visual Studio Build Tools
    echo        with the "Desktop development with C++" workload.
    exit /b 1
)

"%MSBUILD%" "%~dp0WaWDll.sln" /p:Configuration=Debug /p:Platform=x86 /v:minimal /nologo %*
