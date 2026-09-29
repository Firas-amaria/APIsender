$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw "Install Visual Studio C++ Build Tools to run host tests." }
$idf = Join-Path $env:USERPROFILE '.platformio\packages\framework-espidf'
$cjson = Join-Path $idf 'components\json\cJSON'
if (!(Test-Path "$cjson\cJSON.c")) { throw "Run pio run first to install ESP-IDF." }
New-Item -ItemType Directory -Force tests/build | Out-Null
$vcvars = Join-Path $installation 'VC\Auxiliary\Build\vcvars64.bat'
$command = 'call "' + $vcvars + '" >nul && cl /nologo /EHsc /std:c++17 /DCJSON_NESTING_LIMIT=16 /I tests/stubs /I src /I "' + $cjson + '" tests/api_tests.cpp src/network/api_client.cpp src/config/base_url.cpp "' + $cjson + '\cJSON.c" /Fo:tests/build/ /Fe:tests/build/api_tests.exe && tests\build\api_tests.exe'
& cmd.exe /d /c $command
if ($LASTEXITCODE -ne 0) { throw "Host checks failed ($LASTEXITCODE)." }
