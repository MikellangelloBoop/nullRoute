$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
$Editor='Z:\games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe';$Project="$Root\NullRoute.uproject";$Port=17784
$Server=$null;$Client=$null;$Observer=$null
try {
 $Started=Get-Date
 $Server=Start-Process -FilePath $Editor -ArgumentList @($Project,'/Game/Maps/NR_Arcology?Training=1','-server','-NRBreacherNetwork','-nullrhi','-unattended','-nosound','-MULTIHOME=127.0.0.1',"-port=$Port","-abslog=$Root\Saved\BreacherServer.log") -WindowStyle Hidden -PassThru
 $Deadline=(Get-Date).AddSeconds(100);$Ready=$false
 while((Get-Date) -lt $Deadline){
  $Server.Refresh();if($Server.HasExited){throw 'Breacher server exited'}
  if((Test-Path -LiteralPath "$Root\Saved\BreacherServer.log") -and (Get-Item -LiteralPath "$Root\Saved\BreacherServer.log").LastWriteTime -ge $Started -and (Select-String -LiteralPath "$Root\Saved\BreacherServer.log" -Pattern "listening on port $Port" -Quiet)){$Ready=$true;break}
  Start-Sleep -Milliseconds 400
 }
 if(!$Ready){throw 'Breacher server startup timeout'}
 $Client=Start-Process -FilePath $Editor -ArgumentList @($Project,"127.0.0.1:$Port",'-game','-nullrhi','-nosound','-unattended','-NRBreacherNetwork',"-abslog=$Root\Saved\BreacherClient.log") -WindowStyle Hidden -PassThru
 $Observer=Start-Process -FilePath $Editor -ArgumentList @($Project,"127.0.0.1:$Port",'-game','-nullrhi','-nosound','-unattended','-NRBreacherNetwork','-NRBreacherObserver',"-abslog=$Root\Saved\BreacherObserver.log") -WindowStyle Hidden -PassThru
 foreach($Run in @($Client,$Observer)){if(!$Run.WaitForExit(120000)){throw 'Breacher client timeout'};$Run.Refresh();if($Run.ExitCode -ne 0){throw "Breacher client failed $($Run.ExitCode)"}}
 if(!(Select-String -LiteralPath "$Root\Saved\BreacherClient.log" -Pattern 'NR_BREACHER_NET_CLIENT PASSED' -Quiet)){throw 'Breacher client marker missing'}
 if(!(Select-String -LiteralPath "$Root\Saved\BreacherObserver.log" -Pattern 'NR_BREACHER_NET_CLIENT PASSED' -Quiet)){throw 'Breacher observer marker missing'}
 if(@(Select-String -LiteralPath "$Root\Saved\BreacherServer.log" -Pattern 'NR_BREACHER_NET_SERVER .* : PASS').Count -ne 1){throw 'Server stance checks missing'}
 Write-Output 'BREACHER DEDICATED SERVER + TWO CLIENTS PASS'
} finally {
 foreach($Run in @($Client,$Observer,$Server)){if($Run){$Run.Refresh();if(!$Run.HasExited){Stop-Process -Id $Run.Id}}}
}
