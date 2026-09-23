param([switch]$Bootstrap)
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$ReleaseRoot=[IO.Path]::GetFullPath((Join-Path $ProjectRoot 'Releases'))
$Pointer=Join-Path $ReleaseRoot 'CurrentBuild.txt'
$Directory=if(Test-Path -LiteralPath $Pointer){(Get-Content -LiteralPath $Pointer -Raw).Trim()}else{'Windows'}
$Selected=[IO.Path]::GetFullPath((Join-Path $ReleaseRoot $Directory))
if (!$Selected.StartsWith($ReleaseRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Current build must be inside Releases' }
Join-Path $Selected $(if($Bootstrap){'NullRoute.exe'}else{'NullRoute\Binaries\Win64\NullRoute.exe'})
