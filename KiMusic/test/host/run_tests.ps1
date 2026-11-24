# =============================================================================
#  Compila e roda todos os testes no PC (precisa do g++ no PATH).
#  Uso (na pasta KiMusic):   powershell -File test/host/run_tests.ps1
# =============================================================================
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..\..')
[Console]::OutputEncoding = [Text.Encoding]::UTF8

$flags = @('-std=c++17', '-Wall', '-Wextra', '-I', 'src', '-I', 'lib/media', '-I', 'lib/tjpgd')
$out = 'test/host'
$failed = 0

function Invoke-Test($name, $sources, $runArgs) {
    Write-Host "`n===== $name =====" -ForegroundColor Cyan
    & g++ @flags @sources -o "$out/$name.exe"
    if ($LASTEXITCODE -ne 0) { Write-Host "Falha ao compilar $name" -ForegroundColor Red; $script:failed++; return }
    & "$out/$name.exe" @runArgs
    if ($LASTEXITCODE -ne 0) { $script:failed++ }
}

# O tjpgd e C puro: compila separado.
& gcc -c -O2 -Wall -I lib/tjpgd lib/tjpgd/tjpgd.c -o "$out/tjpgd.o"

Invoke-Test 'mp3info_test' @('test/host/mp3info_test.cpp', 'lib/media/Mp3Info.cpp') @('SD/music/music.mp3')
Invoke-Test 'cover_test'   @('test/host/cover_test.cpp', 'lib/media/Mp3Info.cpp', 'lib/media/CoverDecoder.cpp', "$out/tjpgd.o") @('SD/cover/music.jpg', "$out/cover_out.bmp")
Invoke-Test 'core_test'    @('test/host/core_test.cpp', 'src/core/Player.cpp', 'src/core/Menu.cpp') @()

if ($failed) { Write-Host "`n$failed conjunto(s) de testes falharam." -ForegroundColor Red; exit 1 }
Write-Host "`nTodos os conjuntos de testes passaram." -ForegroundColor Green
