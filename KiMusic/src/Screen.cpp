#include "Screen.hpp"

void Screen::flushDisplay(
    lv_disp_drv_t *disp,
    const lv_area_t *area,
    lv_color_t *color
)
{
    Screen *screen = static_cast<Screen *>(disp->user_data);

    uint32_t width = area->x2 - area->x1 + 1;
    uint32_t height = area->y2 - area->y1 + 1;

    screen->tft.startWrite();
    screen->tft.setAddrWindow(area->x1, area->y1, width, height);
    // LV_COLOR_16_SWAP = 1: o LVGL ja entrega as cores com bytes trocados.
    screen->tft.pushColors((uint16_t *)&color->full, width * height, false);
    screen->tft.endWrite();

    lv_disp_flush_ready(disp);
}

void Screen::formatTime(char *out, size_t size, uint32_t seconds)
{
    snprintf(
        out,
        size,
        "%02lu:%02lu",
        (unsigned long)(seconds / 60),
        (unsigned long)(seconds % 60)
    );
}

// As fontes Montserrat embutidas no LVGL so tem ASCII: troca letras acentuadas
// (U+00C0..U+00FF) pela letra base e o resto por '?'.
void Screen::toDisplayText(const char *utf8, char *out, size_t size)
{
    static const char LATIN1[] =
        "AAAAAAACEEEEIIII"    // C0-CF
        "DNOOOOOxOUUUUYPs"    // D0-DF
        "aaaaaaaceeeeiiii"    // E0-EF
        "dnooooo/ouuuuypy";   // F0-FF

    const uint8_t *in = (const uint8_t *)utf8;
    size_t n = 0;

    while (*in && n + 1 < size) {
        uint32_t cp;
        int extra;
        if (*in < 0x80)                { cp = *in; extra = 0; }
        else if ((*in & 0xE0) == 0xC0) { cp = *in & 0x1F; extra = 1; }
        else if ((*in & 0xF0) == 0xE0) { cp = *in & 0x0F; extra = 2; }
        else if ((*in & 0xF8) == 0xF0) { cp = *in & 0x07; extra = 3; }
        else                           { in++; continue; }
        in++;

        for (int i = 0; i < extra; i++) {
            if ((*in & 0xC0) != 0x80) { cp = '?'; break; }
            cp = (cp << 6) | (*in & 0x3F);
            in++;
        }

        if (cp < 0x80)                       out[n++] = (char)cp;
        else if (cp >= 0xC0 && cp <= 0xFF)   out[n++] = LATIN1[cp - 0xC0];
        else                                 out[n++] = '?';
    }
    out[n] = '\0';
}

void Screen::setHighlight(lv_obj_t *obj, bool on)
{
    lv_obj_set_style_outline_width(obj, on ? 2 : 0, LV_PART_MAIN);
    lv_obj_set_style_outline_pad(obj, 2, LV_PART_MAIN);
    lv_obj_set_style_outline_color(obj, lv_color_white(), LV_PART_MAIN);
}

void Screen::init()
{
    tft.init();
    tft.setRotation(1);
    lv_init();

    lv_disp_draw_buf_init(&drawBuffer, buffer, nullptr, WIDTH * BUFFER_LINES);

    lv_disp_drv_init(&displayDriver);
    displayDriver.hor_res = WIDTH;
    displayDriver.ver_res = HEIGHT;
    displayDriver.flush_cb = flushDisplay;
    displayDriver.draw_buf = &drawBuffer;
    displayDriver.user_data = this;
    lv_disp_drv_register(&displayDriver);

    ui_init();

    // Largura fixa e reticencias para nomes longos.
    constexpr lv_coord_t textWidth = 130;
    lv_obj_t *labels[] = { ui_MusicName, ui_Band, ui_Album };
    for (lv_obj_t *label : labels) {
        lv_obj_set_width(label, textWidth);
        lv_obj_set_x(label, 75);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    }

    lv_obj_add_flag(ui_volume, LV_OBJ_FLAG_HIDDEN);   // aparece so no modo volume

    lastTick = millis();
}

void Screen::update()
{
    uint32_t now = millis();
    lv_tick_inc(now - lastTick);
    lastTick = now;
    lv_timer_handler();
}

void Screen::setMusicInfo(const char *title, const char *artist, const char *album)
{
    char text[128];

    toDisplayText(title, text, sizeof(text));
    lv_label_set_text(ui_MusicName, text);

    toDisplayText(artist, text, sizeof(text));
    lv_label_set_text(ui_Band, text);

    toDisplayText(album, text, sizeof(text));
    lv_label_set_text(ui_Album, text);
}

void Screen::setStatus(bool playing, PlayerMode mode)
{
    const char *modeText = "";
    if (mode == PlayerMode::Volume) {
        modeText = "  VOL";
    } else if (mode == PlayerMode::Position) {
        modeText = "  POS";
    }

    char text[16];
    snprintf(text, sizeof(text), "%s%s", playing ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE, modeText);
    lv_label_set_text(ui_Time, text);

    // A barra de volume so aparece enquanto o modo volume esta ativo.
    if (mode == PlayerMode::Volume) {
        lv_obj_clear_flag(ui_volume, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui_volume, LV_OBJ_FLAG_HIDDEN);
    }

    setHighlight(ui_Prog, mode == PlayerMode::Position);
}

void Screen::setVolume(int volume)
{
    lv_bar_set_value(ui_volume, volume, LV_ANIM_ON);
}

void Screen::setProgress(uint32_t elapsed, uint32_t duration)
{
    if (duration == 0) {
        duration = 1;
    }
    if (elapsed > duration) {
        elapsed = duration;
    }

    char text[12];

    formatTime(text, sizeof(text), elapsed);
    lv_label_set_text(ui_TimeMusic, text);

    text[0] = '-';
    formatTime(text + 1, sizeof(text) - 1, duration - elapsed);
    lv_label_set_text(ui_TimeMusicLeft, text);

    lv_bar_set_range(ui_Prog, 0, (int32_t)duration);
    lv_bar_set_value(ui_Prog, (int32_t)elapsed, LV_ANIM_OFF);
}
