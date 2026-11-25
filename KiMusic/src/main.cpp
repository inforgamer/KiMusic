// =============================================================================
//  KiMusic  -  MP3 player bare-metal (ESP32-S3 + ILI9341 + LVGL + PCM5102A)
// -----------------------------------------------------------------------------
//  Ponto de entrada: so cria os modulos, liga uns aos outros e roda o loop.
//
//    input/    Encoder, Button                       (hardware de entrada)
//    core/     Player, Menu + interfaces             (regras, sem Arduino, testadas no PC)
//    storage/  TrackList, SdCatalog, DemoSource      (musicas e playlists do SD)
//    system/   AudioEngine, Device, Power            (som, brilho, ajustes, ligar/desligar)
//    view/     Screen (LVGL), ScreenView             (o que aparece no display)
//    App       le os botoes e chama Player/Menu/Device
//    lib/media Mp3Info, CoverDecoder                 (leitura de MP3 e capas, testadas no PC)
//
//  Fluxo: botoes -> App -> Player/Menu -> PlayerView (ScreenView) -> Screen
//                               Player -> AudioBackend (AudioEngine) -> DAC
// =============================================================================
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <esp_random.h>

#include "Config.hpp"
#include "App.hpp"
#include "core/Player.hpp"
#include "core/Menu.hpp"
#include "storage/SdCatalog.hpp"
#include "storage/DemoSource.hpp"
#include "system/AudioEngine.hpp"
#include "system/Device.hpp"
#include "system/Power.hpp"
#include "view/Screen.hpp"
#include "view/ScreenView.hpp"

// ---- Modulos (objetos globais: sem alocacao dinamica) ----
Screen screen;
ScreenView view(screen);
SdCatalog catalog(SD, Config::MUSIC_FOLDER, Config::COVER_FOLDER,
                  Config::PLAYLIST_FOLDER, Config::FAVORITES_NAME);
DemoSource demo;
AudioEngine audio(SD, Config::PIN_I2S_BCK, Config::PIN_I2S_LRCK,
                  Config::PIN_I2S_DOUT, Config::PIN_DAC_MUTE);
Player player(view);
Device device(screen, player, audio, Config::PIN_CENTER);
Menu menu(player, view, &catalog, &device);
App app(player, menu, device);

// Inicia o cartao SD e procura as musicas e playlists.
// Retorna true se houver pelo menos um MP3 na pasta de musicas.
static bool initSD()
{
    // A tela e o SD dividem o barramento SPI. O TFT_eSPI ja chamou SPI.begin()
    // com os pinos do platformio.ini, entao basta manter a tela desselecionada.
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    if (!SD.begin(Config::PIN_SD_CS, SPI, Config::SD_SPI_HZ)) {
        Serial.println("Erro ao iniciar o SD! Usando musica demo.");
        return false;
    }
    Serial.printf("SD iniciado: %llu bytes\r\n", SD.cardSize());

    size_t found = catalog.scan();
    Serial.printf("Musicas em %s: %u | playlists: %u\r\n", Config::MUSIC_FOLDER,
                  (unsigned)found, (unsigned)catalog.playlistCount());
    return found > 0;
}

void setup()
{
    // Acordou pelo botao? So liga se ele ficar 2 s apertado.
    Power::handleWakeup(Config::PIN_CENTER, Config::POWER_ON_HOLD_MS);

    Serial.begin(Config::SERIAL_BAUD);
    delay(500);
    Serial.println("KiMusic iniciando...");

    screen.init();
    screen.initBacklight(Config::PIN_BACKLIGHT, Config::BACKLIGHT_LEDC_CHANNEL);
    app.begin();
    audio.begin();

    player.setAudio(&audio);
    player.setRandomSeed(esp_random());

    bool hasMusic = initSD();
    device.begin();   // brilho, volume e Repetir salvos

    const TrackSource &source = hasMusic ? static_cast<const TrackSource &>(catalog.allTracks())
                                         : static_cast<const TrackSource &>(demo);
    player.begin(source, millis());
}

void loop()
{
    screen.update();
    app.poll(millis());
    view.poll(millis());
    delay(5);   // botoes tem debounce de 30 ms e o encoder e por interrupcao: 200 Hz basta
}
