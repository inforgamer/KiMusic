// =============================================================================
//  KiMusic  -  MP3 player bare-metal (ESP32-S3 + ILI9341 + LVGL)
// -----------------------------------------------------------------------------
//  Ponto de entrada: so cria os modulos, liga uns aos outros e roda o loop.
//
//    input/    Encoder, Button, Buzzer          (hardware de entrada)
//    core/     Player, Menu                     (regras, sem Arduino, testadas no PC)
//    storage/  Library (SD), DemoSource         (de onde vem as musicas)
//    view/     Screen (LVGL), ScreenView        (o que aparece no display)
//    App       le os botoes e chama Player/Menu
//    lib/media Mp3Info, CoverDecoder            (leitura de MP3 e capas, testadas no PC)
//
//  Fluxo: botoes -> App -> Player/Menu -> PlayerView (ScreenView) -> Screen
// =============================================================================
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "Config.hpp"
#include "App.hpp"
#include "core/Player.hpp"
#include "core/Menu.hpp"
#include "storage/Library.hpp"
#include "storage/DemoSource.hpp"
#include "view/Screen.hpp"
#include "view/ScreenView.hpp"

// ---- Modulos (objetos globais: sem alocacao dinamica) ----
Screen screen;
ScreenView view(screen);
Library library(SD, Config::MUSIC_FOLDER, Config::COVER_FOLDER);
DemoSource demo;
Player player(view);
Menu menu(player, view);
App app(player, menu);

// Inicia o cartao SD e procura as musicas.
// Retorna true se houver pelo menos um MP3 na pasta de musicas.
static bool initSD()
{
    // A tela e o SD dividem o barramento SPI. O TFT_eSPI ja chamou SPI.begin()
    // com os pinos do platformio.ini, entao basta manter a tela desselecionada.
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    if (!SD.begin(Config::PIN_SD_CS, SPI)) {
        Serial.println("Erro ao iniciar o SD! Usando musica demo.");
        return false;
    }
    Serial.printf("SD iniciado: %llu bytes\r\n", SD.cardSize());

    size_t found = library.scan();
    Serial.printf("Musicas em %s: %u\r\n", Config::MUSIC_FOLDER, (unsigned)found);
    return found > 0;
}

void setup()
{
    Serial.begin(Config::SERIAL_BAUD);
    delay(1000);
    Serial.println("KiMusic iniciando...");

    screen.init();
    app.begin();

    const TrackSource &source = initSD() ? static_cast<const TrackSource &>(library)
                                         : static_cast<const TrackSource &>(demo);
    player.begin(source, millis());
}

void loop()
{
    screen.update();
    app.poll(millis());
    delay(1);
}
