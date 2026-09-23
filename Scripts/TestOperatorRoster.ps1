param([string]$Executable,[switch]$Visual,[switch]$Network,[switch]$MenuOnly,[int]$Width=2400,[int]$Height=1200)
$ErrorActionPreference='Stop'
$Root=Split-Path $PSScriptRoot -Parent
if(!$Executable){$Executable=& (Join-Path $PSScriptRoot 'GetPackagedGame.ps1')}
$Runs=@()
try {
 if($Network) {
  $Port=17825;$ServerLog="$Root\Saved\RosterNetwork-Server.log";$Started=Get-Date
  $Server=Start-Process -FilePath $Executable -ArgumentList @('/Game/Maps/NR_Arcology?Training=1?SkipMenu=1','-server','-nullrhi','-NRRosterNetwork','-unattended','-nosound','-MULTIHOME=127.0.0.1',"-port=$Port","-abslog=$ServerLog") -WindowStyle Hidden -PassThru
  $Runs+=$Server;$Deadline=(Get-Date).AddSeconds(60);$Ready=$false
  while((Get-Date) -lt $Deadline){$Server.Refresh();if($Server.HasExited){throw 'Roster server exited'};if((Test-Path $ServerLog) -and (Get-Item $ServerLog).LastWriteTime -ge $Started -and (Select-String -Path $ServerLog -SimpleMatch "listening on port $Port" -Quiet)){$Ready=$true;break};Start-Sleep -Milliseconds 400}
  if(!$Ready){throw 'Roster server startup timeout'}
  foreach($Name in @('Client','Observer')){$Run=Start-Process -FilePath $Executable -ArgumentList @("127.0.0.1:$Port",'-nullrhi','-NRRosterNetwork','-unattended','-nosound',"-abslog=$Root\Saved\RosterNetwork-$Name.log") -WindowStyle Hidden -PassThru;$Runs+=$Run}
  foreach($Run in $Runs[1..2]){if(!$Run.WaitForExit(180000)){throw 'Roster network timeout'};$Run.Refresh();if($Run.ExitCode -ne 0){throw 'Roster network client failed'}}
  foreach($Name in @('Client','Observer')){if(!(Select-String -Path "$Root\Saved\RosterNetwork-$Name.log" -SimpleMatch 'NR_ROSTER_NET_CLIENT PASSED' -Quiet)){throw "Missing $Name success marker"}}
  if(!(Select-String -Path $ServerLog -SimpleMatch 'treatment RPC and replicated health PASS' -Quiet)){throw 'Server treatment check missing'}
  if(!(Select-String -Path $ServerLog -SimpleMatch 'cosmetics-free PASS' -Quiet)){throw 'Dedicated server cosmetics check missing'}
  Write-Output 'ROSTER DEDICATED SERVER + TWO CLIENTS PASS'
 } else {
  $Flag=if($Visual){'NRRosterVisual'}else{'NRRosterTest'};$Log="$Root\Saved\Packaged-$Flag.log"
  $Args=@('/Game/Maps/NR_Arcology?Training=1?SkipMenu=1',"-$Flag",'-RenderOffscreen','-windowed','-ForceRes','-unattended','-nosound',"-abslog=$Log")
  if($MenuOnly){$Args+='-NRRosterMenuOnly'}
  $Args+=if($Visual){@("-ResX=$Width","-ResY=$Height")}else{@('-ResX=640','-ResY=480')}
  $Run=Start-Process -FilePath $Executable -ArgumentList $Args -WindowStyle Hidden -PassThru;$Runs+=$Run
  if(!$Run.WaitForExit(240000)){throw 'Roster test timeout'};$Run.Refresh();if($Run.ExitCode -ne 0){throw "Roster test failed: $Log"}
  $Marker=if($Visual){'NR_ROSTER_VISUAL_COMPLETE'}else{'NR_ROSTER_COMPLETE PASSED'}
  if(!(Select-String -Path $Log -SimpleMatch $Marker -Quiet)){throw 'Roster completion marker missing'}
  Write-Output "$Flag PASS"
 }
} finally {foreach($Run in $Runs){$Run.Refresh();if(!$Run.HasExited){Stop-Process -Id $Run.Id}}}
