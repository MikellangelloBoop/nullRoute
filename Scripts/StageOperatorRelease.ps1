$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
$Candidate=Join-Path $Root 'Releases\OperatorsCandidate\Windows'
$Destination=[IO.Path]::GetFullPath((Join-Path $Root 'Releases\Operators\Windows'))
$ReleaseRoot=[IO.Path]::GetFullPath((Join-Path $Root 'Releases'))+[IO.Path]::DirectorySeparatorChar
if(!$Destination.StartsWith($ReleaseRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Unexpected release target'}
if(!(Select-String -Path "$Root\Saved\RosterPackage.log" -SimpleMatch 'BUILD SUCCESSFUL' -Quiet)){throw 'Package has not completed'}
if(!(Select-String -Path "$Root\Saved\RosterHUDGameBuild.log" -SimpleMatch 'Result: Succeeded' -Quiet)){throw 'Final native build has not completed'}
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
& robocopy $Candidate $Destination /E /XD Saved /R:1 /W:1 /NFL /NDL /NJH /NJS
if($LASTEXITCODE -ge 8){throw 'Release copy failed'}
# Only native HUD/test code changed after cook; the cooked assets remain identical.
Copy-Item -LiteralPath "$Root\Binaries\Win64\NullRoute.exe" -Destination "$Destination\NullRoute\Binaries\Win64\NullRoute.exe"
if(Test-Path "$Root\Binaries\Win64\NullRoute.pdb"){Copy-Item -LiteralPath "$Root\Binaries\Win64\NullRoute.pdb" -Destination "$Destination\NullRoute\Binaries\Win64\NullRoute.pdb"}
$Previous=Split-Path (& (Join-Path $PSScriptRoot 'GetPackagedGame.ps1') -Bootstrap) -Parent
foreach($Folder in @('Config','SaveGames')) {
 $Source=Join-Path $Previous "NullRoute\Saved\$Folder"
 if((Test-Path $Source) -and $Previous -ne $Destination){& robocopy $Source "$Destination\NullRoute\Saved\$Folder" /E /R:1 /W:1 /NFL /NDL /NJH /NJS;if($LASTEXITCODE -ge 8){throw "Could not preserve $Folder"}}
}
foreach($Name in @('global.ucas','global.utoc','NullRoute-Windows.pak','NullRoute-Windows.ucas','NullRoute-Windows.utoc')) {
 $A=(Get-FileHash -LiteralPath "$Candidate\NullRoute\Content\Paks\$Name").Hash
 $B=(Get-FileHash -LiteralPath "$Destination\NullRoute\Content\Paks\$Name").Hash
 if($A -ne $B){throw "Release archive differs: $Name"}
}
if((Get-FileHash "$Root\Binaries\Win64\NullRoute.exe").Hash -ne (Get-FileHash "$Destination\NullRoute\Binaries\Win64\NullRoute.exe").Hash){throw 'Native executable differs'}
Write-Output 'OPERATOR RELEASE STAGED; CURRENT BUILD POINTER UNCHANGED'
exit 0
