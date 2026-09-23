param([string]$EngineRoot='Z:\games\UE_5.8',[int]$Port=17777,[switch]$Movement)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$Project=Join-Path $ProjectRoot 'NullRoute.uproject'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Log=Join-Path $ProjectRoot 'Saved\NetworkServer.log'
$ServerProcess=$null
$ExitCode=1
try {
    # Bind only loopback. This test never opens a public game server.
    $Arguments=@(('"'+$Project+'"'),'/Game/Maps/NR_Arcology?Training=1','-server','-nullrhi','-unattended','-nosound','-MULTIHOME=127.0.0.1',"-port=$Port",('-abslog="'+$Log+'"'))
    $StartedAt=Get-Date
    $ServerProcess=Start-Process -FilePath $Editor -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    $Deadline=(Get-Date).AddSeconds(100)
    $Ready=$false
    while ((Get-Date) -lt $Deadline) {
        $ServerProcess.Refresh()
        if ($ServerProcess.HasExited) { throw 'Server exited before accepting connections. See Saved/NetworkServer.log.' }
        if ((Test-Path $Log) -and ((Get-Item -LiteralPath $Log).LastWriteTime -ge $StartedAt) -and (Select-String -LiteralPath $Log -Pattern "listening on port $Port" -Quiet)) { $Ready=$true; break }
        Start-Sleep -Milliseconds 500
    }
    if (!$Ready) { throw 'Server startup timed out. See Saved/NetworkServer.log.' }
    [string[]]$ExtraFlags=@(); if($Movement){$ExtraFlags=@('-NRNetworkMovement')}
    & $Editor $Project "127.0.0.1:$Port" @ExtraFlags -game -nullrhi -RenderOffscreen -nosound -unattended -NRSmokeTest "-abslog=$ProjectRoot\Saved\NetworkClient.log"
    $ExitCode=$LASTEXITCODE
    if($Movement -and (!(Select-String -LiteralPath "$ProjectRoot\Saved\NetworkClient.log" -Pattern 'NR_NETMOVE sprint PASS' -Quiet) -or !(Select-String -LiteralPath "$ProjectRoot\Saved\NetworkClient.log" -Pattern 'NR_NETMOVE crouch PASS' -Quiet))){$ExitCode=1}
} finally {
    if ($ServerProcess) {
        $ServerProcess.Refresh()
        if (!$ServerProcess.HasExited) { Stop-Process -Id $ServerProcess.Id }
    }
}
exit $ExitCode
