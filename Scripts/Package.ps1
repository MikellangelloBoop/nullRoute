param([string]$EngineRoot='Z:\games\UE_5.8')
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$PreviousLogFolder=$env:uebp_LogFolder
try {
    $env:uebp_LogFolder=Join-Path $ProjectRoot 'Saved\AutomationLogs'
    New-Item -ItemType Directory -Force -Path $env:uebp_LogFolder | Out-Null
    # Build directly: this installation fails when UAT launches UBT with a Unicode
    # profile path. Separate build and cook also keep their logs in the project.
    foreach ($Target in @('NullRouteEditor','NullRoute')) {
        & (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat') $Target Win64 Development "-Project=$ProjectRoot\NullRoute.uproject" -WaitMutex -NoHotReloadFromIDE "-Log=$ProjectRoot\Saved\Package-$Target.log" *> "$ProjectRoot\Saved\Package-$Target-console.log"
        if ($LASTEXITCODE) { throw "Build failed: Saved/Package-$Target-console.log" }
    }
    & (Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat') BuildCookRun "-project=$ProjectRoot\NullRoute.uproject" -noP4 -platform=Win64 -clientconfig=Development -skipbuild -skipbuildeditor -cook -stage -pak -archive "-archivedirectory=$ProjectRoot\Releases" -utf8output *> "$ProjectRoot\Saved\Package.log"
    $PackageExit=$LASTEXITCODE
} finally { $env:uebp_LogFolder=$PreviousLogFolder }
if ($PackageExit) { Get-Content "$ProjectRoot\Saved\Package.log" -Tail 35; exit $PackageExit }
Set-Content -LiteralPath "$ProjectRoot\Releases\CurrentBuild.txt" -Value 'Windows' -Encoding utf8
Write-Output "Packaged: $ProjectRoot\Releases\Windows\NullRoute.exe"
