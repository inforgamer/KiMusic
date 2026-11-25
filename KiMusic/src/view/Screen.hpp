// =============================================================================
//  view/Screen.hpp  -  Display ILI9341 + LVGL
// -----------------------------------------------------------------------------
//  Unica classe que mexe no display e nos objetos ui_* gerados pelo
//  SquareLine Studio (pasta src/UI, que nao deve ser editada a mao).
//
//  O que ela desenha:
//    - barra do topo:  Play/Pause + modo (VOL/POS)          -> setStatus()
//    - titulo, artista e album                              -> setMusicInfo()
//    - tempo decorrido, restante e barra de progresso       -> setProgress()
//    - barra de volume (so aparece no modo Volume)          -> setVolume() / setStatus()
//    - capa do album (140x140) ou icone de nota             -> coverBuffer() + showCover()
//    - menu sobreposto                                      -> showMenu() / hideMenu()
//    - brilho da luz de fundo (PWM no pino LED)             -> setBrightness()
//
//  Textos chegam em UTF-8 e sao convertidos para ASCII, porque as fontes
//  Montserrat embutidas no LVGL nao tem letras acentuadas ("ç" vira "c").
// =============================================================================
#ifndef VIEW_SCREEN_HPP
#define VIEW_SCREEN_HPP

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

extern "C" {
    #include "UI/ui.h"
}

#include "CoverDecoder.hpp"
#include "core/Types.hpp"

class Screen
{
    public:
        static constexpr int MENU_ROWS = 7;   // linhas visiveis do menu

        // ---- Ciclo de vida ----
        void init();      // display, LVGL, tela do SquareLine, capa e menu
        void update();    // chamar no loop: avanca o relogio do LVGL e redesenha

        // ---- Energia e luz de fundo ----
        // Liga o PWM do pino LED do display (comeca apagado ate setBrightness).
        void initBacklight(uint8_t pin, uint8_t ledcChannel);
        void setBrightness(int percent);     // 0..100
        void showMessage(const char *text);  // tela cheia, desenhada na hora (ex.: "Desligando...")
        void hideMessage();
        void powerDown();                    // apaga a luz e poe o ILI9341 para dormir
        void wakeUp(int brightness);         // acorda o ILI9341, redesenha e reacende a luz

        // ---- Tela principal ----
        void setMusicInfo(const char *title, const char *artist, const char *album);
        void setVolume(int volume);
        void setProgress(uint32_t elapsed, uint32_t duration);
        void setStatus(bool playing, PlayerMode mode);
        void setBattery(int percent, bool charging);   // canto superior direito

        // ---- Capa ----
        // Quem chama decodifica a imagem em coverBuffer() e depois chama showCover(true).
        uint16_t *coverBuffer() { return reinterpret_cast<uint16_t *>(coverPixels); }
        void showCover(bool visible);

        // ---- Menu ----
        // rows[0..count) sao as linhas visiveis; selected e a linha destacada (-1 = nenhuma).
        void showMenu(const char *title, const char *const *rows, int count, int selected);
        void hideMenu();

    private:
        static constexpr int WIDTH = 320;
        static constexpr int HEIGHT = 240;
        static constexpr int BUFFER_LINES = 20;   // linhas do buffer de desenho do LVGL

        TFT_eSPI tft = TFT_eSPI();

        lv_disp_draw_buf_t drawBuffer;
        lv_disp_drv_t displayDriver;
        lv_color_t buffer[WIDTH * BUFFER_LINES];
        uint32_t lastTick = 0;

        int backlightChannel = -1;   // canal LEDC da luz de fundo (-1 = sem controle)
        lv_obj_t *messageOverlay = nullptr;

        // Capa: imagem desenhada por cima de ui_AlbumCover.
        lv_color_t coverPixels[CoverDecoder::BUFFER_PIXELS];
        lv_img_dsc_t coverImage;
        lv_obj_t *coverObj = nullptr;
        lv_obj_t *coverPlaceholder = nullptr;

        // Menu: painel com MENU_ROWS linhas fixas (a lista rola por cima delas).
        lv_obj_t *menuPanel = nullptr;
        lv_obj_t *menuTitle = nullptr;
        lv_obj_t *menuRows[MENU_ROWS] = {};

        void createCover();
        void createMenu();

        static void flushDisplay(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color);
        static void formatTime(char *out, size_t size, uint32_t seconds);
        static void toDisplayText(const char *utf8, char *out, size_t size);
        static void setHighlight(lv_obj_t *obj, bool on);
};

#endif
