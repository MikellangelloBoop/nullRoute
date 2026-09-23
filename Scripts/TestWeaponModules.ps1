$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
$Editor='Z:\games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe';$Project="$Root\NullRoute.uproject";$Port=17779
$Server=$null;$Client=$null;$Observer=$null
try {
 $Started=Get-Date
 $Server=Start-Process -FilePath $Editor -ArgumentList @($Project,'/Game/Maps/NR_Arcology?Training=1','-server','-NRModuleNetwork','-nullrhi','-unattended','-nosound','-MULTIHOME=127.0.0.1',"-port=$Port","-abslog=$Root\Saved\ModuleServer.log") -WindowStyle Hidden -PassThru
 $Deadline=(Get-Date).AddSeconds(100);$Ready=$false
 while((Get-Date) -lt $Deadline){
  $Server.Refresh();if($Server.HasExited){throw 'Module server exited'}
  if((Test-Path -LiteralPath "$Root\Saved\ModuleServer.log") -and (Get-Item -LiteralPath "$Root\Saved\ModuleServer.log").LastWriteTime -ge $Started -and (Select-String -LiteralPath "$Root\Saved\ModuleServer.log" -Pattern "listening on port $Port" -Quiet)){$Ready=$true;break}
  Start-Sleep -Milliseconds 400
 }
 if(!$Ready){throw 'Module server startup timeout'}
 $Client=Start-Process -FilePath $Editor -ArgumentList @($Project,"127.0.0.1:$Port",'-game','-nullrhi','-nosound','-unattended','-NRModuleNetwork',"-abslog=$Root\Saved\ModuleClient.log") -WindowStyle Hidden -PassThru
 $Observer=Start-Process -FilePath $Editor -ArgumentList @($Project,"127.0.0.1:$Port",'-game','-nullrhi','-nosound','-unattended','-NRModuleNetwork','-NRModuleObserver',"-abslog=$Root\Saved\ModuleObserver.log") -WindowStyle Hidden -PassThru
 foreach($Run in @($Client,$Observer)) {if(!$Run.WaitForExit(120000)){throw 'Module client timeout'};$Run.Refresh();if($Run.ExitCode -ne 0){throw "Module client failed $($Run.ExitCode)"}}
 if(!(Select-String -LiteralPath "$Root\Saved\ModuleClient.log" -Pattern 'NR_NETMODULE_COMPLETE PASSED' -Quiet)){throw 'Module client marker missing'}
 if(!(Select-String -LiteralPath "$Root\Saved\ModuleObserver.log" -Pattern 'NR_NETMODULE_OBSERVER PASSED' -Quiet)){throw 'Observer marker missing'}
 if(!(Select-String -LiteralPath "$Root\Saved\ModuleServer.log" -Pattern 'NR_NETMODULE_SERVER cosmetics absent : PASS' -Quiet)){throw 'DS cosmetic check missing'}
 Write-Output 'MODULE DEDICATED SERVER + TWO CLIENTS PASS'
} finally {
 foreach($Run in @($Client,$Observer,$Server)){if($Run){$Run.Refresh();if(!$Run.HasExited){Stop-Process -Id $Run.Id}}}
}
