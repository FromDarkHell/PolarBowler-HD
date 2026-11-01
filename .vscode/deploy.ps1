param(
    [Parameter(Mandatory)] [string] $DllPath,
    [Parameter(Mandatory)] [string] $GameDirectory,
    [switch] $Launch
)

$ErrorActionPreference = 'Stop'

$gameExecutable = Join-Path $GameDirectory 'Polar Bowler Classic.exe'
$pluginsDirectory = Join-Path $GameDirectory 'Plugins'

# The loaded DLL is locked by the game, close it before overwriting.
$runningGame = Get-Process 'Polar Bowler Classic' -ErrorAction SilentlyContinue
if ($runningGame) {
    Write-Host 'Closing running game...'
    $runningGame | Stop-Process -Force
    $runningGame | Wait-Process
}

Copy-Item -Force -LiteralPath $DllPath -Destination $pluginsDirectory
Write-Host "Deployed $(Split-Path $DllPath -Leaf) to $pluginsDirectory..."

if ($Launch) {
    Start-Process -FilePath $gameExecutable -WorkingDirectory $GameDirectory
    Write-Host 'Started game...'
}
