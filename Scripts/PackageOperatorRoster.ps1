param([string]$EngineRoot='Z:\games\UE_5.8')
$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
$Before=$env:uebp_LogFolder
try {
 $env:uebp_LogFolder="$Root\Saved\RosterAutomationLogs"
 New-Item -ItemType Directory -Force -Path $env:uebp_LogFolder | Out-Null
 & (Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat') BuildCookRun "-project=$Root\NullRoute.uproject" -noP4 -platform=Win64 -clientconfig=Development -skipbuild -skipbuildeditor -cook -stage -pak -archive "-stagingdirectory=$Root\Saved\RosterStaging" "-archivedirectory=$Root\Releases\OperatorsCandidate" -utf8output *> "$Root\Saved\RosterPackage.log"
 if($LASTEXITCODE){throw 'Roster packaging failed: Saved/RosterPackage.log'}
} finally { $env:uebp_LogFolder=$Before }
Write-Output 'ROSTER CANDIDATE PACKAGED'
