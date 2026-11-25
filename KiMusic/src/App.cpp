// =============================================================================
//  App.cpp  -  Implementacao (ver App.hpp)
// =============================================================================
#include "App.hpp"
#include "Config.hpp"

App::App(Player &p, Menu &m, DeviceControl &d)
    : player(p),
      menu(m),
      device(d),
      encoder(Config::PIN_ENCODER_A, Config::PIN_ENCODER_B),
      centerButton(Config::PIN_CENTER, Config::DOUBLE_TAP_MS),
      previousButton(Config::PIN_PREVIOUS),
      nextButton(Config::PIN_NEXT),
      volumeButton(Config::PIN_VOLUME),
      positionButton(Config::PIN_POSITION) {}

void App::begin()
{
    encoder.begin();
    centerButton.begin();
    previousButton.begin();
    nextButton.begin();
    volumeButton.begin();
    positionButton.begin();

    // Prioridade acima do loop (1): le os botoes no horario mesmo com a tela ocupada.
    xTaskCreatePinnedToCore(inputTask, "input", 3072, this, 2, nullptr, 1);
}

void App::inputTask(void *self)
{
    for (;;) {
        static_cast<App *>(self)->sampleButtons();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void App::sampleButtons()
{
    ButtonEvent center = centerButton.poll();
    bool previous = previousButton.clicked();
    bool next     = nextButton.clicked();
    bool volume   = volumeButton.clicked();
    bool position = positionButton.clicked();

    portENTER_CRITICAL(&inputLock);
    if (center != ButtonEvent::None) pending.center = center;
    pending.previous |= previous;
    pending.next     |= next;
    pending.volume   |= volume;
    pending.position |= position;
    portEXIT_CRITICAL(&inputLock);
}

void App::discardInput()
{
    portENTER_CRITICAL(&inputLock);
    pending = Input();
    portEXIT_CRITICAL(&inputLock);
    encoder.readRotate();
}

void App::poll(uint32_t nowMs)
{
    Input in = readInput();

    if (device.isScreenOff()) {
        // Tela apagada: o primeiro toque so acende a tela.
        if (in.any()) {
            device.screenOn();
        }
    } else if (menu.isOpen()) {
        handleMenu(in);
    } else {
        handlePlayer(in);
    }

    player.update(nowMs);   // a musica continua andando com o menu aberto ou a tela apagada
}

App::Input App::readInput()
{
    portENTER_CRITICAL(&inputLock);
    Input in = pending;
    pending = Input();
    portEXIT_CRITICAL(&inputLock);
    in.rotation = encoder.readRotate();
    return in;
}

// -----------------------------------------------------------------------------
//  Menu aberto
// -----------------------------------------------------------------------------

void App::handleMenu(const Input &in)
{
    // O giro do encoder nao mexe no menu: so os botoes (Vol sobe, Pos desce,
    // Prev volta, Next/central escolhe).
    if (in.center == ButtonEvent::DoubleClick) {
        menu.close();
        return;
    }
    if (in.volume)   menu.up();
    if (in.position) menu.down();
    if (in.previous) menu.back();
    if (in.center == ButtonEvent::Click || in.next) menu.select();
}

// -----------------------------------------------------------------------------
//  Menu fechado (tela do player)
// -----------------------------------------------------------------------------

void App::handlePlayer(const Input &in)
{
    if (in.rotation != 0) {
        player.rotate(in.rotation);
    }

    if (in.center == ButtonEvent::DoubleClick) {
        menu.open();   // nao mexe no Play/Pause: o toque simples so chega depois da janela
        return;   // o resto desta volta e ignorado
    }

    if (in.center == ButtonEvent::Click) {
        player.togglePlay();
    }

    // Os comandos que nao valem no modo atual sao ignorados pelo proprio Player.
    if (in.position) player.togglePositionMode();
    if (in.volume && player.toggleVolumeMode() && player.mode() == PlayerMode::Normal) {
        device.saveSettings();   // saiu do modo volume: grava uma vez, nao a cada passo
    }
    if (in.previous) player.previous();
    if (in.next)     player.next();
}
