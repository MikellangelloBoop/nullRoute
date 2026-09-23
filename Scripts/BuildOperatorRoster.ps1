param([string]$EngineRoot='Z:\games\UE_5.8')
$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
foreach($Step in @('NRBreacherAssets','NRMetaHumanArmor')) {
    $Run=Start-Process -FilePath $Editor -ArgumentList @("$Root\NullRoute.uproject","-run=$Step",'-nullrhi','-unattended','-nop4',"-abslog=$Root\Saved\Roster-$Step.log") -WindowStyle Hidden -PassThru
    if(!$Run.WaitForExit(600000)){Stop-Process -Id $Run.Id;throw "Roster generation timeout: $Step"}
    $Run.Refresh();if($Run.ExitCode -ne 0){throw "Roster generation failed: $Step ($($Run.ExitCode))"}
}
Write-Output '20 OPERATOR / FACTION SETS GENERATED'
