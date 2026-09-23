param([int]$Port=7777,[int]$MaxPlayers=10,[int]$PreparationSeconds=45,[string]$Name='NullRoute',[switch]$Editor,[string]$EngineRoot='Z:\games\UE_5.8',[string]$PackageFolder='')
$ErrorActionPreference='Stop'
if($Port -lt 1 -or $Port -gt 65534){throw 'Port must be 1..65534 (query uses Port+1)'}
if($MaxPlayers -lt 2 -or $MaxPlayers -gt 10){throw 'MaxPlayers must be 2..10'}
if($Name -notmatch '^[A-Za-z0-9_-]{1,48}$'){throw 'Use 1..48 letters, numbers, underscores or hyphens for Name'}
$Root=Split-Path $PSScriptRoot -Parent
$Exe=if($PackageFolder){Join-Path $Root "Releases\$PackageFolder\NullRoute\Binaries\Win64\NullRoute.exe"}else{& (Join-Path $PSScriptRoot "GetPackagedGame.ps1")}
$Prefix=@()
if($Editor){$Exe=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe';$Prefix=@(('"'+(Join-Path $Root 'NullRoute.uproject')+'"'))}
if(!(Test-Path -LiteralPath $Exe)){throw 'Build/package the project first, or pass -Editor'}
$Log=Join-Path $Root "Saved\Dedicated-$Port.log";New-Item -ItemType Directory -Force (Split-Path $Log) | Out-Null
$Map="/Game/Maps/NR_Switchyard?Training=0?MinPlayers=2?MaxPlayers=$MaxPlayers`?PrepSeconds=$PreparationSeconds`?ServerName=$Name"
$Run=Start-Process -FilePath $Exe -ArgumentList ($Prefix+@($Map,'-server','-nullrhi','-nosound','-unattended',"-port=$Port",('-abslog="'+$Log+'"'))) -WindowStyle Hidden -PassThru
Write-Output "Null Route PID $($Run.Id), game UDP $Port, query UDP $($Port+1). Log: $Log"
Write-Output 'This process continues after closing this terminal. Stop this specific PID when finished.'
