param([string]$Python='python',[int]$Port=8000,[switch]$Install)
$ErrorActionPreference='Stop'
Set-Location $PSScriptRoot
if ($Install) {
    & $Python -m venv .venv
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
    & .\.venv\Scripts\python.exe -m pip install -r requirements.txt
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
}
if (Test-Path .\.venv\Scripts\python.exe) { $Python='.\.venv\Scripts\python.exe' }
# This launcher binds localhost only. HTTPS deployment keeps secure cookies enabled.
$env:NR_PUBLIC_ORIGIN="http://localhost:$Port"
$env:NR_SECURE_COOKIES='0'
& $Python -m uvicorn app.main:app --host 127.0.0.1 --port $Port
exit $LASTEXITCODE
