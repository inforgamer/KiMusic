// =============================================================================
//  Config.hpp  -  Pinagem e constantes do KiMusic
// -----------------------------------------------------------------------------
//  Pinagem completa e ligacoes: Documentação/Pinagem.md
//  Os pinos do display (TFT_*) ficam no platformio.ini (build_flags), porque
//  a biblioteca TFT_eSPI le de la.
// =============================================================================
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <stdint.h>

namespace Config
{
    // ---- Encoder rotativo (KY-040) ----
    constexpr uint8_t PIN_ENCODER_A = 6;    // CLK
    constexpr uint8_t PIN_ENCODER_B = 7;    // DT
    constexpr uint8_t PIN_CENTER    = 5;    // SW: Play/Pause (clique) e Menu (segurar)

    // ---- Botoes (cruz direcional) ----
    constexpr uint8_t PIN_VOLUME    = 16;   // cima     (vermelho)
    constexpr uint8_t PIN_POSITION  = 17;   // baixo    (azul)
    constexpr uint8_t PIN_PREVIOUS  = 18;   // esquerda (verde)
    constexpr uint8_t PIN_NEXT      = 8;    // direita  (amarelo)

    // ---- Outros ----
    constexpr uint8_t PIN_BUZZER    = 4;
    constexpr uint8_t PIN_SD_CS     = 15;   // SD divide o SPI com o display
    constexpr uint8_t PIN_BACKLIGHT = 21;   // LED do display (PWM = brilho)

    // ---- DAC PCM5102A (I2S) ----
    constexpr uint8_t PIN_I2S_BCK   = 40;   // BCK
    constexpr uint8_t PIN_I2S_LRCK  = 41;   // LCK / LRCK
    constexpr uint8_t PIN_I2S_DOUT  = 42;   // DIN
    constexpr uint8_t PIN_DAC_MUTE  = 39;   // XSMT: HIGH = som, LOW = mudo

    // ---- Botao central: tempos de pressao ----
    constexpr uint32_t MENU_HOLD_MS     = 3000;   // segurar 3 s: abre/fecha o menu
    constexpr uint32_t POWER_ON_HOLD_MS = 2000;   // segurar 2 s desligado: liga
                                                  // (desligar: Menu > Configuracoes > Desligar)

    // ---- Diversos ----
    constexpr uint32_t SERIAL_BAUD  = 115200;
    constexpr uint8_t BACKLIGHT_LEDC_CHANNEL = 7;  // canal 0 e usado pelo tone() do buzzer

    // ---- Cartao SD ----
    constexpr const char *MUSIC_FOLDER    = "/music";      // MP3 tocados (nao recursivo)
    constexpr const char *COVER_FOLDER    = "/cover";      // capas: /cover/<nome da musica>.jpg
    constexpr const char *PLAYLIST_FOLDER = "/playlists";  // playlists: /playlists/<nome>.m3u
    constexpr const char *FAVORITES_NAME  = "Favoritas";   // playlist do "Favoritar musica"
}

#endif
