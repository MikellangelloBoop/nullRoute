param([string]$EngineRoot=$env:UE_ROOT)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
if (!$EngineRoot) { $EngineRoot='Z:\games\UE_5.8' }
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $ProjectRoot 'NullRoute.uproject'
# Back up the arena before replacing this update's generated props and lighting.
$Backup=Join-Path $ProjectRoot ('Backups\operations-assets-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $Backup -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $ProjectRoot 'Content\Maps\NR_Arcology.umap') -Destination $Backup
& (Join-Path $ProjectRoot 'Scripts\Project.ps1') -Action Build -EngineRoot $EngineRoot
if ($LASTEXITCODE) { throw 'C++ build failed' }
foreach ($Step in @('NROperationsAssets','PrepareOperations','NROperationsNav')) {
    $Log=Join-Path $ProjectRoot "Saved\Update$Step.log"
    if ($Step -eq 'PrepareOperations') {
        & $Editor $Project -run=pythonscript "-script=$ProjectRoot\Scripts\PrepareOperations.py" -unattended -nop4 -nullrhi "-abslog=$Log"
    } else {
        & $Editor $Project "-run=$Step" -unattended -nop4 -nullrhi "-abslog=$Log"
    }
    if ($LASTEXITCODE) { throw "$Step failed; inspect $Log" }
    $Markers=switch ($Step) {
        'NROperationsAssets' { @('NR_OPERATIONS_ASSETS_COMPLETE') }
        'PrepareOperations' { @('NR_OPERATIONS_AUDIO_COMPLETE','NR_OPERATIONS_MAP_COMPLETE') }
        'NROperationsNav' { @('NR_OPERATIONS_NAV_COMPLETE') }
    }
    foreach ($Marker in $Markers) {
        if (!(Select-String -LiteralPath $Log -Pattern $Marker -Quiet)) { throw "$Step did not finish; inspect $Log" }
    }
}
Write-Output 'Operations meshes, audio, arena props and navigation bounds updated.'
