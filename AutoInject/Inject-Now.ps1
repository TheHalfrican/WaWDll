# Manually inject WaWDll.dll into an already-running World at War.
#
# Use this when the game is already open, or when you launched it without the Steam
# launch option. Starts the same supervisor Trigger.cmd uses, so it also exits by
# itself when the game closes -- nothing is left running.

$here       = Split-Path -Parent $MyInvocation.MyCommand.Path
$supervisor = Join-Path $here 'WaW_AutoInject.ps1'
$log        = Join-Path $here 'trigger.log'

"`n===== MANUAL injection requested $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') =====" |
    Add-Content -Path $log -Encoding utf8

# A supervisor already watching this session holds a named mutex, and a second copy
# would just exit. Clear any existing one so a manual request always takes effect.
$killed = 0
Get-CimInstance Win32_Process -Filter "Name='powershell.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.CommandLine -like '*WaW_AutoInject*' } |
    ForEach-Object {
        try { Stop-Process -Id $_.ProcessId -Force -ErrorAction Stop; $killed++ } catch {}
    }
if ($killed) {
    Start-Sleep -Milliseconds 500
    "  cleared $killed existing supervisor(s)" | Add-Content -Path $log -Encoding utf8
}

Start-Process -FilePath 'powershell.exe' -WindowStyle Hidden -ArgumentList @(
    '-NoProfile','-WindowStyle','Hidden','-ExecutionPolicy','Bypass','-File',$supervisor
)
"  supervisor started" | Add-Content -Path $log -Encoding utf8

Write-Host ''
Write-Host 'Injection requested. Give it about 10 seconds.' -ForegroundColor Green
Write-Host 'Press INSERT in game to open the menu.'
Write-Host ''
Write-Host "Log: $(Join-Path $here 'autoinject.log')" -ForegroundColor DarkGray
Write-Host ''
