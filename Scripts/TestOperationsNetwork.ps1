$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent;$Editor='Z:\games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe';$Project="$Root\NullRoute.uproject";$Port=17783
$Runs=@();$Server=$null
try {
 $Started=Get-Date
 $Server=Start-Process -FilePath $Editor -ArgumentList @($Project,'/Game/Maps/NR_Arcology?Training=1','-server','-NROperationsNetwork','-nullrhi','-unattended','-nosound','-MULTIHOME=127.0.0.1',"-port=$Port","-abslog=$Root\Saved\OperationsServer.log") -WindowStyle Hidden -PassThru
 $Deadline=(Get-Date).AddSeconds(100);$Ready=$false
 while((Get-Date) -lt $Deadline){
  $Server.Refresh();if($Server.HasExited){throw 'Operations server exited'}
  if((Test-Path -LiteralPath "$Root\Saved\OperationsServer.log") -and (Get-Item -LiteralPath "$Root\Saved\OperationsServer.log").LastWriteTime -ge $Started -and (Select-String -LiteralPath "$Root\Saved\OperationsServer.log" -Pattern "listening on port $Port" -Quiet)){$Ready=$true;break}
  Start-Sleep -Milliseconds 400
 }
 if(!$Ready){throw 'Operations server startup timeout'}
 foreach($Role in @('Enemy','Observer','Owner')){
  $Args=@($Project,"127.0.0.1:$Port`?Name=Ops$Role",'-game','-nullrhi','-unattended','-nosound','-NROperationsNetwork',"-abslog=$Root\Saved\Operations$Role.log")
  if($Role -ne 'Owner'){$Args+="-NROps$Role"}
  $Runs+=Start-Process -FilePath $Editor -ArgumentList $Args -WindowStyle Hidden -PassThru
 }
 foreach($Run in $Runs){if(!$Run.WaitForExit(150000)){throw 'Operations network client timeout'};$Run.Refresh();if($Run.ExitCode -ne 0){throw 'Operations network client failed'}}
 foreach($Role in @('Enemy','Observer','Owner')){if(!(Select-String -LiteralPath "$Root\Saved\Operations$Role.log" -Pattern 'NR_NETOPS_COMPLETE PASSED' -Quiet)){throw "Operations $Role marker missing"}}
 if(@(Select-String -LiteralPath "$Root\Saved\OperationsServer.log" -Pattern 'NR_NETOPS_SERVER .* : PASS').Count -ne 2){throw 'Operations server checks missing'}
 Write-Output 'OPERATIONS SERVER + ALLY + OWNER + ENEMY PASS'
} finally {
 foreach($Run in @($Runs)+@($Server)){if($Run){$Run.Refresh();if(!$Run.HasExited){Stop-Process -Id $Run.Id}}}
}
