param([string]$Address='201.51.19.38:7777')
$ErrorActionPreference='Stop'
if($Address -notmatch '^[a-zA-Z0-9.-]+:[0-9]{1,5}$'){throw 'Use host:port'}
$Exe=& (Join-Path $PSScriptRoot 'GetPackagedGame.ps1') -Bootstrap
if(!(Test-Path -LiteralPath $Exe)){throw 'Selected packaged game is missing'}
Start-Process -FilePath $Exe -ArgumentList @($Address,'-windowed','-ResX=1600','-ResY=900')
