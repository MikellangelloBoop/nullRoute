param([string]$Executable,[switch]$Visual,[switch]$Network,[string]$OutputRoot)
$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
if(!$OutputRoot){$OutputRoot=Join-Path $Root 'Saved\EvolutionQA'}
New-Item -ItemType Directory -Force $OutputRoot | Out-Null
if(!$Executable){$Executable=& (Join-Path $PSScriptRoot 'GetPackagedGame.ps1')}
$Runs=@()
try {
 if($Network){
  $Port=17935;$Log="$OutputRoot\Server.log"
  $Run=Start-Process -FilePath $Executable -ArgumentList @('/Game/Maps/NR_Switchyard?Training=0?MinPlayers=2?PrepSeconds=120','-server','-nullrhi','-NREvolutionNetwork','-unattended','-nosound','-MULTIHOME=127.0.0.1',"-port=$Port","-abslog=$Log") -WindowStyle Hidden -PassThru;$Runs+=$Run
  $Limit=(Get-Date).AddSeconds(60);$Ready=$false
  while((Get-Date) -lt $Limit){$Run.Refresh();if($Run.HasExited){throw 'Server exited'};if((Test-Path $Log) -and (Select-String -Path $Log -SimpleMatch "listening on port $Port" -Quiet)){$Ready=$true;break};Start-Sleep -Milliseconds 500}
  if(!$Ready){throw 'Server startup timeout'}
  foreach($Name in @('Driver','Observer')){
   $Args=@("127.0.0.1:$Port",'-nullrhi','-NREvolutionNetwork','-unattended','-nosound',"-abslog=$OutputRoot\$Name.log")
   if($Name -eq 'Driver'){$Args+='-NRPoseDriver'}
   $Runs+=Start-Process -FilePath $Executable -ArgumentList $Args -WindowStyle Hidden -PassThru
  }
  foreach($P in $Runs[1..2]){if(!$P.WaitForExit(90000)){throw 'Network check timeout'}}
  if(!(Select-String -LiteralPath "$OutputRoot\Observer.log" -SimpleMatch -Pattern 'NR_EVOLUTION_NET_OBSERVER PASSED' -Quiet)){throw 'Observer replication check failed'}
  if(!(Select-String -LiteralPath "$OutputRoot\Server.log" -SimpleMatch -Pattern 'server has no cosmetic graph: PASS' -Quiet)){throw 'Dedicated server presentation isolation failed'}
  Write-Output 'EVOLUTION DEDICATED SERVER + TWO CLIENTS PASS'
 } else {
  $Flag=if($Visual){'NREvolutionVisual'}else{'NREvolutionTest'}
  $Args=@('/Game/Maps/NR_Switchyard?Training=1?SkipMenu=1',"-$Flag",'-NRVisualTest','-RenderOffscreen','-windowed','-ForceRes','-ResX=1600','-ResY=900','-unattended','-nosound',"-abslog=$OutputRoot\$Flag.log",'-ExecCmds=t.MaxFPS 60')
  $Run=Start-Process -FilePath $Executable -ArgumentList $Args -WindowStyle Hidden -PassThru;$Runs+=$Run
  if(!$Run.WaitForExit(180000)){throw 'Evolution test timeout'}
  if($Run.ExitCode -ne 0){throw "Evolution process failed: $($Run.ExitCode)"}
  if(Select-String -LiteralPath "$OutputRoot\$Flag.log" -Pattern 'NR_EVOLUTION.*FAIL|Fatal error|Assertion failed' -Quiet){throw 'Evolution check failed'}
  $Marker=if($Visual){'NR_EVOLUTION_VISUAL_COMPLETE'}else{'NR_EVOLUTION_COMPLETE PASSED'}
  if(!(Select-String -LiteralPath "$OutputRoot\$Flag.log" -SimpleMatch -Pattern $Marker -Quiet)){throw 'Evolution completion marker missing'}
  Write-Output "$Flag PASS"
 }
} finally {foreach($P in $Runs){$P.Refresh();if(!$P.HasExited){Stop-Process -Id $P.Id}}}
