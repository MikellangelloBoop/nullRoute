$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
$Editor='Z:\games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe';$Project="$Root\NullRoute.uproject";$Port=17781
$Server=$null;$Client=$null;$Observer=$null
try {
 $Started=Get-Date
 $Server=Start-Process -FilePath $Editor -ArgumentList @($Project,'/Game/Maps/NR_Arcology?Training=1','-server','-NRStanceNetwork','-nullrhi','-unattended','-nosound','-MULTIHOME=127.0.0.1',"-port=$Port","-abslog=$Root\Saved\StanceServer.log") -WindowStyle Hidden -PassThru
 $Deadline=(Get-Date).AddSeconds(100);$Ready=$false
 while((Get-Date) -lt $Deadline){
  $Server.Refresh();if($Server.HasExited){throw 'Stance server exited'}
  if((Test-Path -LiteralPath "$Root\Saved\StanceServer.log") -and (Get-Item -LiteralPath "$Root\Saved\StanceServer.log").LastWriteTime -ge $Started -and (Select-String -LiteralPath "$Root\Saved\StanceServer.log" -Pattern "listening on port $Port" -Quiet)){$Ready=$true;break}
  Start-Sleep -Milliseconds 400
 }
 if(!$Ready){throw 'Stance server startup timeout'}
 $Client=Start-Process -FilePath $Editor -ArgumentList @($Project,"127.0.0.1:$Port",'-game','-nullrhi','-nosound','-unattended','-NRStanceNetwork',"-abslog=$Root\Saved\StanceClient.log") -WindowStyle Hidden -PassThru
 $Observer=Start-Process -FilePath $Editor -ArgumentList @($Project,"127.0.0.1:$Port",'-game','-nullrhi','-nosound','-unattended','-NRStanceNetwork','-NRStanceObserver',"-abslog=$Root\Saved\StanceObserver.log") -WindowStyle Hidden -PassThru
 foreach($Run in @($Client,$Observer)){if(!$Run.WaitForExit(120000)){throw 'Stance client timeout'};$Run.Refresh();if($Run.ExitCode -ne 0){throw "Stance client failed $($Run.ExitCode)"}}
 if(!(Select-String -LiteralPath "$Root\Saved\StanceClient.log" -Pattern 'NR_NETSTANCE_COMPLETE PASSED' -Quiet)){throw 'Stance client marker missing'}
 if(!(Select-String -LiteralPath "$Root\Saved\StanceObserver.log" -Pattern 'NR_NETSTANCE_OBSERVER PASSED' -Quiet)){throw 'Stance observer marker missing'}
 if(@(Select-String -LiteralPath "$Root\Saved\StanceServer.log" -Pattern 'NR_NETSTANCE_SERVER .* : PASS').Count -ne 3){throw 'Server stance checks missing'}
 Write-Output 'STANCE DEDICATED SERVER + TWO CLIENTS PASS'
} finally {
 foreach($Run in @($Client,$Observer,$Server)){if($Run){$Run.Refresh();if(!$Run.HasExited){Stop-Process -Id $Run.Id}}}
}
