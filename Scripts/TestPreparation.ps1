param([switch]$Packaged)
$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
$Editor='Z:\games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project="$Root\NullRoute.uproject";$Port=17786
$Prefix=@($Project)
if($Packaged){$Editor="$Root\Releases\Windows\NullRoute\Binaries\Win64\NullRoute.exe";$Prefix=@()}
$Server=$null;$Clients=@()
try {
 $Started=Get-Date
 $Server=Start-Process -FilePath $Editor -ArgumentList ($Prefix+@('/Game/Maps/NR_Arcology?Training=1','-server','-NRPreparationNetwork','-nullrhi','-unattended','-nosound','-MULTIHOME=127.0.0.1',"-port=$Port","-abslog=$Root\Saved\PreparationServer.log")) -WindowStyle Hidden -PassThru
 $Ready=$false;$Deadline=(Get-Date).AddSeconds(90)
 while((Get-Date) -lt $Deadline){
  $Server.Refresh();if($Server.HasExited){throw 'Preparation server exited'}
  if((Test-Path -LiteralPath "$Root\Saved\PreparationServer.log") -and (Get-Item -LiteralPath "$Root\Saved\PreparationServer.log").LastWriteTime -ge $Started -and (Select-String -LiteralPath "$Root\Saved\PreparationServer.log" -Pattern "listening on port $Port" -Quiet)){$Ready=$true;break}
  Start-Sleep -Milliseconds 400
 }
 if(!$Ready){throw 'Preparation server startup timeout'}
 foreach($Index in 0..1){
  $Clients+=Start-Process -FilePath $Editor -ArgumentList ($Prefix+@("127.0.0.1:$Port",'-game','-NRPreparationNetwork','-nullrhi','-nosound','-unattended',"-abslog=$Root\Saved\PreparationClient$Index.log")) -WindowStyle Hidden -PassThru
 }
 foreach($Run in $Clients){if(!$Run.WaitForExit(120000)){throw 'Preparation client timeout'};$Run.Refresh();if($Run.ExitCode -ne 0){throw "Preparation client failed: $($Run.ExitCode)"}}
 foreach($Index in 0..1){if(!(Select-String -LiteralPath "$Root\Saved\PreparationClient$Index.log" -Pattern 'NR_PREPNET_CLIENT_COMPLETE PASSED' -Quiet)){throw "Client $Index checks failed"}}
 if(!(Select-String -LiteralPath "$Root\Saved\PreparationServer.log" -Pattern 'NR_PREPNET_SERVER_COMPLETE PASSED' -Quiet)){throw 'Server checks failed'}
 'PREPARATION DEDICATED SERVER + TWO CLIENTS PASS'
} finally {
 foreach($Run in ($Clients+@($Server))){if($Run){$Run.Refresh();if(!$Run.HasExited){Stop-Process -Id $Run.Id}}}
}
