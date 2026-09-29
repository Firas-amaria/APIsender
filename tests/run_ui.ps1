$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw "Install Visual Studio C++ Build Tools." }
New-Item -ItemType Directory -Force tests/build/ui | Out-Null
$vcvars = Join-Path $installation 'VC\Auxiliary\Build\vcvars64.bat'
$arguments = @('/nologo', '/w', '/EHsc', '/std:c++17', '/DLV_CONF_INCLUDE_SIMPLE', '/DLV_KCONFIG_IGNORE', '/I src', '/I src/config', '/I components/lvgl', 'tests/ui_preview.cpp', 'src/ui/ui.cpp')
$arguments += Get-ChildItem components/lvgl/src -Recurse -Filter '*.c' | ForEach-Object { '"' + $_.FullName + '"' }
$arguments += '/Fo:tests/build/ui/'
$arguments += '/Fe:tests/build/ui_preview.exe'
$arguments | Set-Content tests/build/ui_args.txt
$command = 'call "' + $vcvars + '" >nul && cl @tests/build/ui_args.txt >tests/build/ui_compile.log 2>&1 && tests\build\ui_preview.exe'
& cmd.exe /d /c $command
if ($LASTEXITCODE -ne 0) { Get-Content tests/build/ui_compile.log -Tail 30; throw "UI preview failed." }
