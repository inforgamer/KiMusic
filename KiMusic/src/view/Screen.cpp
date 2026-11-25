// =============================================================================
//  view/Screen.cpp  -  Implementacao (ver Screen.hpp)
// =============================================================================
#include "view/Screen.hpp"

// -----------------------------------------------------------------------------
//  Driver do display: o LVGL chama flushDisplay com um retangulo pronto.
// -----------------------------------------------------------------------------

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

    // Largura fixa; artista e album longos terminam em reticencias.
    constexpr lv_coord_t textWidth = 130;
    lv_obj_t *labels[] = { ui_MusicName, ui_Band, ui_Album };
    for (lv_obj_t *label : labels) {
        lv_obj_set_width(label, textWidth);
        lv_obj_set_x(label, 75);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    }
    // Nome da musica longo rola em loop (efeito letreiro) em vez de cortar com "...".
    lv_label_set_long_mode(ui_MusicName, LV_LABEL_LONG_SCROLL_CIRCULAR);
    // Altura de uma linha: a rolagem redesenha o titulo a cada quadro, e com a altura
    // automatica a area ia ate a linha do album (150x41 px em vez de 130x22).
    lv_obj_set_height(ui_MusicName, lv_font_get_line_height(&lv_font_montserrat_18));

    lv_obj_add_flag(ui_volume, LV_OBJ_FLAG_HIDDEN);   // aparece so no modo volume

    createCover();
    createMenu();

    lastTick = millis();
}

void Screen::createCover()
{
    // Icone de nota musical enquanto nao ha capa.
    coverPlaceholder = lv_label_create(ui_AlbumCover);
    lv_label_set_text(coverPlaceholder, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(coverPlaceholder, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_center(coverPlaceholder);

    memset(&coverImage, 0, sizeof(coverImage));
    coverImage.header.cf = LV_IMG_CF_TRUE_COLOR;
    coverImage.header.w = CoverDecoder::COVER_SIZE;
    coverImage.header.h = CoverDecoder::COVER_SIZE;
    coverImage.data_size = sizeof(coverPixels);
    coverImage.data = reinterpret_cast<const uint8_t *>(coverPixels);

    // A imagem fica no mesmo lugar de ui_AlbumCover, por cima dele.
    coverObj = lv_img_create(ui_Center);
    lv_img_set_src(coverObj, &coverImage);
    lv_obj_set_align(coverObj, LV_ALIGN_CENTER);
    lv_obj_set_pos(coverObj, lv_obj_get_x_aligned(ui_AlbumCover), lv_obj_get_y_aligned(ui_AlbumCover));
    lv_obj_add_flag(coverObj, LV_OBJ_FLAG_HIDDEN);
}

// -----------------------------------------------------------------------------
//  Luz de fundo e energia
// -----------------------------------------------------------------------------

void Screen::initBacklight(uint8_t pin, uint8_t ledcChannel)
{
    backlightChannel = ledcChannel;
    ledcSetup(ledcChannel, 5000 /* Hz */, 8 /* bits: 0..255 */);
    ledcAttachPin(pin, ledcChannel);
    ledcWrite(ledcChannel, 0);   // apagado ate o brilho salvo ser aplicado
}

void Screen::setBrightness(int percent)
{
    if (backlightChannel < 0) {
        return;
    }
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    ledcWrite(backlightChannel, (uint32_t)percent * 255 / 100);
}

void Screen::showMessage(const char *text)
{
    hideMessage();
    lv_obj_t *overlay = lv_obj_create(lv_layer_top());
    messageOverlay = overlay;
    lv_obj_set_size(overlay, WIDTH, HEIGHT);
    lv_obj_center(overlay);
    lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), LV_PART_MAIN);

    char ascii[64];
    toDisplayText(text, ascii, sizeof(ascii));
    lv_obj_t *label = lv_label_create(overlay);
    lv_label_set_text(label, ascii);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_center(label);

    lv_refr_now(nullptr);   // desenha agora (o loop pode nao rodar mais)
}

void Screen::hideMessage()
{
    if (messageOverlay) {
        lv_obj_del(messageOverlay);
        messageOverlay = nullptr;
    }
}

void Screen::powerDown()
{
    setBrightness(0);
    tft.writecommand(0x28);   // ILI9341: DISPOFF
    delay(20);
    tft.writecommand(0x10);   // ILI9341: SLPIN (modo de baixo consumo)
    delay(120);
}

void Screen::wakeUp(int brightness)
{
    tft.writecommand(0x11);   // ILI9341: SLPOUT
    delay(120);               // o controlador pede 120 ms antes do proximo comando
    tft.writecommand(0x29);   // ILI9341: DISPON

    // Redesenha a tela inteira antes de acender a luz (sem mostrar lixo).
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(nullptr);
    setBrightness(brightness);
}

// -----------------------------------------------------------------------------
//  Capa
// -----------------------------------------------------------------------------

void Screen::showCover(bool visible)
{
    if (visible) {
        lv_img_cache_invalidate_src(&coverImage);   // o conteudo do buffer mudou
        lv_obj_clear_flag(coverObj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(coverObj);
    } else {
        lv_obj_add_flag(coverObj, LV_OBJ_FLAG_HIDDEN);
    }
}

void Screen::createMenu()
{
    menuPanel = lv_obj_create(lv_layer_top());
    lv_obj_set_size(menuPanel, 240, 196);
    lv_obj_center(menuPanel);
    lv_obj_set_style_pad_all(menuPanel, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_row(menuPanel, 2, LV_PART_MAIN);
    lv_obj_set_flex_flow(menuPanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(menuPanel, LV_OBJ_FLAG_SCROLLABLE);

    menuTitle = lv_label_create(menuPanel);
    lv_obj_set_width(menuTitle, lv_pct(100));
    lv_obj_set_style_text_font(menuTitle, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_align(menuTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    for (int i = 0; i < MENU_ROWS; i++) {
        lv_obj_t *row = lv_label_create(menuPanel);
        lv_obj_set_width(row, lv_pct(100));
        lv_label_set_long_mode(row, LV_LABEL_LONG_DOT);
        lv_obj_set_style_pad_hor(row, 6, LV_PART_MAIN);
        lv_obj_set_style_pad_ver(row, 2, LV_PART_MAIN);
        lv_obj_set_style_radius(row, 4, LV_PART_MAIN);
        lv_obj_set_style_bg_color(row, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
        menuRows[i] = row;
    }

    lv_obj_add_flag(menuPanel, LV_OBJ_FLAG_HIDDEN);
}

void Screen::showMenu(const char *title, const char *const *rows, int count, int selected)
{
    char text[64];

    toDisplayText(title, text, sizeof(text));
    lv_label_set_text(menuTitle, text);

    for (int i = 0; i < MENU_ROWS; i++) {
        lv_obj_t *row = menuRows[i];
        if (i >= count) {
            lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        toDisplayText(rows[i], text, sizeof(text));
        lv_label_set_text(row, text);
        lv_obj_set_style_bg_opa(row, i == selected ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_clear_flag(menuPanel, LV_OBJ_FLAG_HIDDEN);
}

void Screen::hideMenu()
{
    lv_obj_add_flag(menuPanel, LV_OBJ_FLAG_HIDDEN);
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

void Screen::setBattery(int percent, bool charging)
{
    char text[16];
    snprintf(text, sizeof(text), "%s%d%%", charging ? LV_SYMBOL_CHARGE " " : "", percent);
    lv_label_set_text(ui_Time1, text);
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
