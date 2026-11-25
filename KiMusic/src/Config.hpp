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
    constexpr uint8_t PIN_CENTER    = 5;    // SW: Play/Pause (toque) e Menu (toque duplo)

    // ---- Botoes (cruz direcional) ----
    constexpr uint8_t PIN_VOLUME    = 16;   // cima     (vermelho)
    constexpr uint8_t PIN_POSITION  = 17;   // baixo    (azul)
    constexpr uint8_t PIN_PREVIOUS  = 18;   // esquerda (verde)
    constexpr uint8_t PIN_NEXT      = 8;    // direita  (amarelo)

    // ---- Outros ----
    constexpr uint8_t PIN_SD_CS     = 15;   // SD divide o SPI com o display
    constexpr uint32_t SD_SPI_HZ    = 20000000;   // padrao da biblioteca e 4 MHz; SD em SPI aceita ate 25 MHz
                                                  // (se der erro de leitura com fios longos, baixe para 10 MHz)
    constexpr uint8_t PIN_BACKLIGHT = 21;   // LED do display (PWM = brilho)

    // ---- Bateria e USB-C ----
    constexpr uint8_t PIN_BATTERY   = 1;    // ADC: bateria por divisor 100k/100k (metade da tensao)
    constexpr uint8_t PIN_USB_VBUS  = 2;    // 5 V do USB-C por divisor 100k/100k: HIGH = cabo ligado
                                            // (dados USB: D- = GPIO 19, D+ = GPIO 20, fixos no S3)

    // ---- DAC PCM5102A (I2S) ----
    constexpr uint8_t PIN_I2S_BCK   = 40;   // BCK
    constexpr uint8_t PIN_I2S_LRCK  = 41;   // LCK / LRCK
    constexpr uint8_t PIN_I2S_DOUT  = 42;   // DIN
    constexpr uint8_t PIN_DAC_MUTE  = 39;   // XSMT: HIGH = som, LOW = mudo

    // ---- Botao central: tempos de pressao ----
    constexpr uint32_t DOUBLE_TAP_MS    = 400;    // toque duplo (abre/fecha o menu); o toque simples espera isso
    constexpr uint32_t POWER_ON_HOLD_MS = 2000;   // segurar 2 s desligado: liga
                                                  // (desligar: Menu > Configuracoes > Desligar)

    // ---- Diversos ----
    constexpr uint32_t SERIAL_BAUD  = 115200;
    constexpr uint8_t BACKLIGHT_LEDC_CHANNEL = 7;  // canal PWM (LEDC) da luz de fundo

    // ---- Cartao SD ----
    constexpr const char *MUSIC_FOLDER    = "/music";      // MP3 tocados (nao recursivo)
    constexpr const char *COVER_FOLDER    = "/cover";      // capas: /cover/<nome da musica>.jpg|.jpeg|.png
    constexpr const char *PLAYLIST_FOLDER = "/playlists";  // playlists: /playlists/<nome>.m3u
    constexpr const char *FAVORITES_NAME  = "Favoritas";   // playlist do "Favoritar musica"
}

#endif
