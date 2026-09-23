param([string]$Executable)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
if (!$Executable) { $Executable=& (Join-Path $PSScriptRoot 'GetPackagedGame.ps1') }
$Runs=@()
try {
    foreach ($Case in @('Breacher','Module','Stance')) {
        $Log=Join-Path $ProjectRoot "Saved\PackagedBreacher-$Case.log"
        # Cooked mesh render data is deliberately discarded by NullRHI. The armor
        # geometry check needs a real RHI; gameplay-only cases stay headless.
        $RenderArgs=if($Case -eq 'Breacher'){@('-RenderOffscreen','-windowed','-ResX=640','-ResY=480','-ForceRes')}else{@('-nullrhi')}
        $Run=Start-Process -FilePath $Executable -ArgumentList (@('/Game/Maps/NR_Arcology?Training=1?SkipMenu=1','-nosound','-unattended',"-NR${Case}Test","-abslog=$Log")+$RenderArgs) -WindowStyle Hidden -PassThru
        $Runs+=@{Process=$Run;Case=$Case;Log=$Log}
    }
    foreach ($Item in $Runs) {
        if (!$Item.Process.WaitForExit(120000)) { throw "$($Item.Case) packaged test timeout" }
        $Item.Process.Refresh()
        $Marker="NR_$($Item.Case.ToUpper())_COMPLETE PASSED"
        if ($Item.Process.ExitCode -ne 0 -or !(Select-String -LiteralPath $Item.Log -SimpleMatch $Marker -Quiet)) { throw "$($Item.Case) packaged test failed: $($Item.Log)" }
        Write-Output "$($Item.Case) packaged PASS"
    }
} finally {
    foreach ($Item in $Runs) { $Item.Process.Refresh();if(!$Item.Process.HasExited){Stop-Process -Id $Item.Process.Id} }
}
