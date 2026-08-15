# Supervises a single World at War session: waits for the game, injects WaWDll.dll,
# then exits when the game exits.
#
# NOTHING RUNS AT BOOT AND NOTHING SITS RESIDENT. This script is started on demand --
# by Trigger.cmd from the Steam launch option, or by Inject-Now.cmd -- and it always
# terminates on its own:
#   * if the game never appears, it gives up after $FirstWaitSecs (5 minutes) and exits
#   * once the game is running, it exits the moment the game does
#
# Why a supervisor at all, rather than letting Trigger.cmd call the injector directly:
# the injector has to wait until the game is actually up before touching it, and doing
# that inline would block Steam's launch chain. This runs alongside instead, and gives
# a log to look at when something goes wrong.

param(
    # Overrides for rehearsing the chain without the real game. Leave both unset
    # for normal use; -Process takes a name with no .exe suffix.
    [string]$Process,
    [string]$Dll
)

# The single-player / Nazi Zombies executable. Multiplayer is CoDWaWmp.exe and is
# deliberately never matched here, so nothing is ever injected into an online game.
$ProcessName    = if ($Process) { $Process } else { 'CoDWaW' }

$InjectorPath   = Join-Path $PSScriptRoot '..\Debug\WaWInjector.exe'
$LogPath        = Join-Path $PSScriptRoot 'autoinject.log'
$FirstWaitSecs  = 300   # how long to wait for the game to appear at all, then give up
$MinGameAgeSecs = 3     # ignore a game process younger than this
$SettleSecs     = 6     # breathing room after the window shows up
$PollSecs       = 2

function Write-Log {
    param([string]$Message)
    "$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')  $Message" | Add-Content -Path $LogPath -Encoding utf8
}

function Get-Game {
    Get-Process -Name $ProcessName -ErrorAction SilentlyContinue |
        Sort-Object StartTime -Descending | Select-Object -First 1
}

# Runs the injector against the already-running game and folds its output into our log.
function Invoke-Injector {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName               = $InjectorPath
    $psi.Arguments              = "-p `"$ProcessName.exe`" -w 0 -t 30000"
    if ($Dll) { $psi.Arguments += " -d `"$Dll`"" }
    $psi.UseShellExecute        = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError  = $true
    $psi.CreateNoWindow         = $true

    $proc = [System.Diagnostics.Process]::Start($psi)
    $output = $proc.StandardOutput.ReadToEnd() + $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()

    foreach ($line in ($output -split "`r?`n")) {
        if ($line.Trim()) { Write-Log "    $line" }
    }

    return $proc.ExitCode
}

if ((Test-Path $LogPath) -and ((Get-Item $LogPath).Length -gt 256KB)) {
    Remove-Item $LogPath -Force -ErrorAction SilentlyContinue
}

# One supervisor per session. Back-to-back launches would otherwise stack up.
$createdNew = $false
$mutex = New-Object System.Threading.Mutex($true, 'Global\WaWDll_AutoInject', [ref]$createdNew)
if (-not $createdNew) {
    Write-Log 'Another supervisor already owns this session; exiting.'
    exit 0
}

try {
    Write-Log '--- Supervisor started. ---'

    if (-not (Test-Path $InjectorPath)) {
        Write-Log "ABORT: injector not found at $InjectorPath -- run build.cmd first."
        exit 1
    }
    $InjectorPath = (Resolve-Path $InjectorPath).Path

    # Phase 1: wait for the game to show up at all, but not forever.
    $deadline = (Get-Date).AddSeconds($FirstWaitSecs)
    while ((Get-Date) -lt $deadline -and -not (Get-Game)) {
        Start-Sleep -Seconds $PollSecs
    }
    if (-not (Get-Game)) {
        Write-Log "Timed out after $FirstWaitSecs s -- $ProcessName.exe never appeared. Exiting."
        exit 1
    }

    # Phase 2: inject once the game is settled, then just watch until it closes.
    $injectedPid = 0

    while ($true) {
        $game = Get-Game
        if (-not $game) {
            Write-Log 'Game closed. Supervisor exiting.'
            break
        }

        if ($game.Id -ne $injectedPid) {
            $age = ((Get-Date) - $game.StartTime).TotalSeconds

            if ($game.MainWindowHandle -ne 0 -and $age -ge $MinGameAgeSecs) {
                Start-Sleep -Seconds $SettleSecs

                # Re-check: the game can die or restart during the settle window.
                $game = Get-Game
                if (-not $game -or $game.MainWindowHandle -eq 0) { continue }

                Write-Log "Injecting into PID $($game.Id) ..."
                $exitCode = Invoke-Injector

                if ($exitCode -eq 0) {
                    $injectedPid = $game.Id
                    Write-Log "Injected successfully into PID $injectedPid."
                }
                else {
                    Write-Log "Injector exited with code $exitCode; retrying shortly."
                    Start-Sleep -Seconds 5
                }
            }
        }

        Start-Sleep -Seconds $PollSecs
    }
}
finally {
    $mutex.ReleaseMutex()
    $mutex.Dispose()
}

exit 0
