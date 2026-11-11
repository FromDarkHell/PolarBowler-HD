param(
    [Parameter(Mandatory)] [string] $DllPath,
    [string] $Launch
)

$ErrorActionPreference = 'Stop'

$games = [ordered]@{
    'Polar Bowler Classic'         = 'C:\Games\Polar Bowler Classic\Polar Bowler Classic.exe'
    'Polar Golfer'                 = 'C:\WildTangent\Apps\GameChannel\Games\169E7C03-35E3-4E8A-855F-225246CE3E5E\golf.exe'
    'Polar Golfer - Pineapple Cup' = 'C:\WildTangent\Apps\GameChannel\Games\PineappleCup\golf.exe'
}

if ($Launch -and -not $games.Contains($Launch)) {
    throw "Unknown game '$Launch'. Known games: $($games.Keys -join ', ')"
}

foreach ($gameName in $games.Keys) {
    $gameExecutable = $games[$gameName]
    $gameDirectory = Split-Path $gameExecutable -Parent
    $pluginsDirectory = Join-Path $gameDirectory 'Plugins'

    if (-not (Test-Path -LiteralPath $pluginsDirectory)) {
        Write-Warning "Skipping $gameName, $pluginsDirectory does not exist."
        continue
    }

    # The loaded DLL is locked by the game, close it before overwriting.
    $runningGame = Get-Process ([IO.Path]::GetFileNameWithoutExtension($gameExecutable)) -ErrorAction SilentlyContinue
    if ($runningGame) {
        Write-Host "Closing $gameName..."
        $runningGame | Stop-Process -Force
        $runningGame | Wait-Process
    }

    Copy-Item -Force -LiteralPath $DllPath -Destination $pluginsDirectory
    Write-Host "Deployed $(Split-Path $DllPath -Leaf) to $pluginsDirectory..."
}

if ($Launch) {
    $gameExecutable = $games[$Launch]
    Start-Process -FilePath $gameExecutable -WorkingDirectory (Split-Path $gameExecutable -Parent)
    Write-Host "Started $Launch..."
}
