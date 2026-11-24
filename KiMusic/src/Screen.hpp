#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

extern "C" {
    #include "UI/ui.h"
}

// O que o encoder controla no momento.
enum class PlayerMode
{
    Normal,
    Volume,
    Position
};

// Camada entre o firmware e a interface gerada pelo SquareLine Studio.
// Todo acesso aos objetos ui_* deve passar por esta classe.
class Screen
{
    private:
        static constexpr int WIDTH = 320;
        static constexpr int HEIGHT = 240;
        static constexpr int BUFFER_LINES = 20;

        TFT_eSPI tft = TFT_eSPI();

        lv_disp_draw_buf_t drawBuffer;
        lv_disp_drv_t displayDriver;
        lv_color_t buffer[WIDTH * BUFFER_LINES];

        uint32_t lastTick = 0;

        static void flushDisplay(
            lv_disp_drv_t *disp,
            const lv_area_t *area,
            lv_color_t *color
        );

        static void formatTime(char *out, size_t size, uint32_t seconds);
        static void toDisplayText(const char *utf8, char *out, size_t size);
        static void setHighlight(lv_obj_t *obj, bool on);

    public:
        void init();
        void update();

        void setMusicInfo(const char *title, const char *artist, const char *album);
        void setVolume(int volume);
        void setProgress(uint32_t elapsed, uint32_t duration);
        void setStatus(bool playing, PlayerMode mode);
};

#endif
