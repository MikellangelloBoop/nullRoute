param(
    [ValidateSet('Build','MetaHumanAssets','MetaHumanCloud','BreacherAssets','BreacherTest','BreacherVisual','GenerateArena','Editor','Play','Server','Join','Tests','Smoke','MatchTest','VisualTest','CollisionTest','DetailTest','ExpansionTest','ModuleTest','Package')]
    [string]$Action='Play',
    [string]$EngineRoot=$env:UE_ROOT,
    [string]$Address='127.0.0.1',
    [int]$Port=7777,
    [switch]$Competitive
)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$Project=Join-Path $ProjectRoot 'NullRoute.uproject'
$Packaged=& (Join-Path $PSScriptRoot 'GetPackagedGame.ps1') -Bootstrap
if ($Action -eq 'Play' -and (Test-Path -LiteralPath $Packaged)) {
    $RunMode=if ($Competitive) { '0' } else { '1' }
    & $Packaged "/Game/Maps/NR_Switchyard?Training=$RunMode" -windowed -ResX=1600 -ResY=900
    exit $LASTEXITCODE
}
if (!$EngineRoot) { $EngineRoot='Z:\games\UE_5.8' }
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$Commandlet=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (!(Test-Path -LiteralPath $Editor)) { throw 'Set UE_ROOT to the Unreal Engine 5.8.1 installation directory.' }
$Map=if($Action -in @('Play','Server')){'/Game/Maps/NR_Switchyard'}else{'/Game/Maps/NR_Arcology'}
$Training=if ($Competitive) { '0' } else { '1' }
switch ($Action) {
    MetaHumanCloud {
        & $Commandlet $Project -run=pythonscript "-script=$ProjectRoot\Scripts\BuildMetaHuman.py" -RenderOffscreen -unattended -nop4 "-abslog=$ProjectRoot\Saved\MetaHumanBuild.log"
    }
    MetaHumanAssets {
        & $Commandlet $Project -run=NRMetaHumanAssets -RenderOffscreen -unattended -nop4 "-abslog=$ProjectRoot\Saved\MetaHumanBody.log"
        if ($LASTEXITCODE) { exit $LASTEXITCODE }
        & $Commandlet $Project -run=pythonscript "-script=$ProjectRoot\Scripts\RetargetMetaHuman.py" -RenderOffscreen -unattended -nop4 "-abslog=$ProjectRoot\Saved\MetaHumanRetarget.log"
        if ($LASTEXITCODE) { exit $LASTEXITCODE }
        & $Commandlet $Project -run=NRMetaHumanArmor -nullrhi -unattended -nop4 "-abslog=$ProjectRoot\Saved\MetaHumanArmor.log"
    }
    BreacherAssets {
        & $Commandlet $Project -run=pythonscript "-script=$ProjectRoot\Scripts\ImportBreacher.py" -unattended -nop4 -nullrhi "-abslog=$ProjectRoot\Saved\ImportBreacher.log"
        if ($LASTEXITCODE) { exit $LASTEXITCODE }
        & $Commandlet $Project -run=NRBreacherAssets -unattended -nop4 -nullrhi "-abslog=$ProjectRoot\Saved\BreacherAssets.log"
        if ($LASTEXITCODE) { exit $LASTEXITCODE }
        & python "$ProjectRoot\ArtSource\Breacher\ExportGLB.py"
    }
    BreacherTest { & $Commandlet $Project "$Map`?Training=1?SkipMenu=1" -game -nullrhi -unattended -NRBreacherTest "-abslog=$ProjectRoot\Saved\BreacherTest.log" }
    BreacherVisual { & $Commandlet $Project "$Map`?Training=1?SkipMenu=1" -game -RenderOffscreen -windowed -ResX=1100 -ResY=1300 -ForceRes -unattended -NRBreacherVisual "-abslog=$ProjectRoot\Saved\BreacherVisual.log" }
    Build { & (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat') NullRouteEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE }
    GenerateArena {
        foreach ($Step in @('NRGenerateArt','NRPolishAssets','NRPrepareFirstPerson','NRDetailAssets','NRExpansionAssets','NRModuleAssets')) {
            & $Commandlet $Project "-run=$Step" -unattended -nop4 -nullrhi "-abslog=$ProjectRoot\Saved\$Step.log"
            if ($LASTEXITCODE) { exit $LASTEXITCODE }
        }
        & $Commandlet $Project -run=pythonscript "-script=$ProjectRoot\Scripts\PolishLighting.py" -unattended -nop4 -nullrhi "-abslog=$ProjectRoot\Saved\PolishLighting.log"
        if ($LASTEXITCODE) { exit $LASTEXITCODE }
        & $Commandlet $Project -run=pythonscript "-script=$ProjectRoot\Scripts\DetailArena.py" -unattended -nop4 -nullrhi "-abslog=$ProjectRoot\Saved\DetailArena.log"
        if ($LASTEXITCODE) { exit $LASTEXITCODE }
        & $Commandlet $Project -run=pythonscript "-script=$ProjectRoot\Scripts\ExpansionArena.py" -unattended -nop4 -nullrhi "-abslog=$ProjectRoot\Saved\ExpansionArena.log"
    }
    Editor { & $Editor $Project }
    Play { & $Editor $Project "$Map`?Training=$Training" -game -windowed -ResX=1600 -ResY=900 }
    Server { & $Commandlet $Project "$Map`?Training=$Training" -server -nullrhi -unattended "-port=$Port" "-abslog=$ProjectRoot\Saved\DedicatedServer.log" }
    Join { & $Editor $Project "$Address`:$Port" -game -windowed -ResX=1280 -ResY=720 }
    Tests { & $Commandlet $Project -unattended -nop4 -nullrhi '-ExecCmds=Automation RunTests NullRoute' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$ProjectRoot\Saved\Tests" "-abslog=$ProjectRoot\Saved\Automation.log" }
    Smoke { & $Commandlet $Project "$Map`?Training=1" -game -nullrhi -unattended -NRSmokeTest "-abslog=$ProjectRoot\Saved\Smoke.log" }
    CollisionTest { & $Commandlet $Project "$Map`?Training=1" -game -nullrhi -unattended -NRCollisionTest "-abslog=$ProjectRoot\Saved\CollisionQA.log" }
    ModuleTest { & $Commandlet $Project "$Map`?Training=1" -game -nullrhi -unattended -NRModuleTest "-abslog=$ProjectRoot\Saved\ModuleTest.log" }
    ExpansionTest { & $Commandlet $Project "$Map`?Training=1" -game -nullrhi -unattended -NRExpansionTest "-abslog=$ProjectRoot\Saved\ExpansionTest.log" }
    DetailTest { & $Commandlet $Project "$Map`?Training=1" -game -nullrhi -unattended -NRDetailTest "-abslog=$ProjectRoot\Saved\DetailTest.log" }
    MatchTest { & $Commandlet $Project "$Map`?Training=1" -game -nullrhi -unattended -NRMatchTest "-abslog=$ProjectRoot\Saved\MatchTest.log" }
    VisualTest { & $Commandlet $Project "$Map`?Training=1" -game -RenderOffscreen -windowed -ResX=1600 -ResY=900 -ForceRes -unattended -NRVisualTest "-abslog=$ProjectRoot\Saved\Visual.log" }
    Package { & (Join-Path $PSScriptRoot 'Package.ps1') -EngineRoot $EngineRoot }
}
if ($LASTEXITCODE) { exit $LASTEXITCODE }
