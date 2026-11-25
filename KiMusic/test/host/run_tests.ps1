# =============================================================================
#  Compila e roda todos os testes no PC (precisa do g++ no PATH).
#  Uso (na pasta KiMusic):   powershell -File test/host/run_tests.ps1
#  As bibliotecas de imagem (JPEGDEC, PNGdec) vem de .pio/libdeps: rode um
#  "pio run" (ou "pio pkg install") pelo menos uma vez antes.
# =============================================================================
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..\..')
[Console]::OutputEncoding = [Text.Encoding]::UTF8

$libs = '.pio/libdeps/esp32-s3-devkitc-1'
$jpegdec = "$libs/JPEGDEC/src"
$pngdec = "$libs/PNGdec/src"
# __LINUX__: as bibliotecas usam C puro (sem Arduino.h); mesmo limite de PNG da placa.
$defines = @('-D__LINUX__', '-DPNG_MAX_BUFFERED_PIXELS=12002')
$flags = @('-std=c++17', '-Wall', '-Wextra', '-I', 'src', '-I', 'lib/media', '-isystem', $jpegdec, '-isystem', $pngdec) + $defines
$out = 'test/host'
$failed = 0

function Invoke-Test($name, $sources, $runArgs) {
    Write-Host "`n===== $name =====" -ForegroundColor Cyan
    & g++ @flags @sources -o "$out/$name.exe"
    if ($LASTEXITCODE -ne 0) { Write-Host "Falha ao compilar $name" -ForegroundColor Red; $script:failed++; return }
    & "$out/$name.exe" @runArgs
    if ($LASTEXITCODE -ne 0) { $script:failed++ }
}

# Bibliotecas de imagem: compiladas uma vez por execucao, sem avisos (codigo de terceiros).
$imageObjs = @()
foreach ($src in @("$jpegdec/JPEGDEC.cpp", "$pngdec/PNGdec.cpp")) {
    $obj = "$out/" + [IO.Path]::GetFileNameWithoutExtension($src) + ".o"
    & g++ -c -O2 -w -std=c++17 @defines -I $jpegdec -I $pngdec $src -o $obj
    $imageObjs += $obj
}
foreach ($src in Get-ChildItem "$pngdec/*.c") {
    $obj = "$out/zlib_" + $src.BaseName + ".o"
    # -Duint=unsigned: o zlib usa o tipo 'uint' (existe no ESP32/Linux, nao no Windows).
    & gcc -c -O2 -w @defines -Duint=unsigned -I $pngdec $src.FullName -o $obj
    $imageObjs += $obj
}

Invoke-Test 'mp3info_test' @('test/host/mp3info_test.cpp', 'lib/media/Mp3Info.cpp') @('SD/music/music.mp3')
Invoke-Test 'cover_test'   (@('test/host/cover_test.cpp', 'lib/media/Mp3Info.cpp', 'lib/media/CoverDecoder.cpp') + $imageObjs) @('SD/cover/music.jpg', "$out/cover_out.bmp")
Invoke-Test 'core_test'    @('test/host/core_test.cpp', 'src/core/Player.cpp', 'src/core/Menu.cpp') @()

if ($failed) { Write-Host "`n$failed conjunto(s) de testes falharam." -ForegroundColor Red; exit 1 }
Write-Host "`nTodos os conjuntos de testes passaram." -ForegroundColor Green
