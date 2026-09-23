param([string]$Executable)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
if (!$Executable) { $Executable=& (Join-Path $PSScriptRoot 'GetPackagedGame.ps1') }
$Executable=(Resolve-Path -LiteralPath $Executable).Path
$GameRoot=Split-Path (Split-Path (Split-Path $Executable -Parent) -Parent) -Parent
$Screens=Join-Path $GameRoot 'Saved\Screenshots'
$Portraits=Join-Path $ProjectRoot 'Saved\MetaHumanPortraits'
New-Item -ItemType Directory -Force -Path $Portraits | Out-Null
foreach ($Format in @('Portrait','Gameplay')) {
    $Width=if($Format -eq 'Portrait'){1100}else{1600}
    $Height=if($Format -eq 'Portrait'){1300}else{900}
    $Log=Join-Path $ProjectRoot "Saved\MetaHumanPackagedVisual-$Format.log"
    $Started=Get-Date
    $Run=Start-Process -FilePath $Executable -ArgumentList @('/Game/Maps/NR_Arcology?Training=1?SkipMenu=1','-RenderOffscreen','-windowed',"-ResX=$Width","-ResY=$Height",'-ForceRes','-nosound','-unattended','-NRBreacherVisual',"-abslog=$Log") -WindowStyle Hidden -PassThru
    try {
        if (!$Run.WaitForExit(120000)) { throw "MetaHuman $Format visual timeout" }
        $Run.Refresh()
        if ($Run.ExitCode -ne 0 -or !(Select-String -LiteralPath $Log -SimpleMatch 'NR_BREACHER_VISUAL_COMPLETE' -Quiet)) { throw "MetaHuman visual failed: $Log" }
        foreach ($Name in @('Chronos','Police','Rebel','FirstPerson','ADS','Reload')) {
            $Screenshot=Get-Item -LiteralPath (Join-Path $Screens "Breacher_$Name.png")
            if ($Screenshot.LastWriteTime -lt $Started) { throw "Stale screenshot: $Name" }
        }
        if ($Format -eq 'Portrait') {
            foreach ($Name in @('Chronos','Police','Rebel')) { Copy-Item -LiteralPath (Join-Path $Screens "Breacher_$Name.png") -Destination $Portraits -Force }
        }
        Write-Output "MetaHuman $Format render PASS"
    } finally { $Run.Refresh(); if (!$Run.HasExited) { Stop-Process -Id $Run.Id } }
}
foreach ($Name in @('Chronos','Police','Rebel')) { Copy-Item -LiteralPath (Join-Path $Portraits "Breacher_$Name.png") -Destination $Screens -Force }
