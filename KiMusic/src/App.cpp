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
      centerButton(Config::PIN_CENTER, Config::MENU_HOLD_MS, Config::POWER_OFF_HOLD_MS),
      previousButton(Config::PIN_PREVIOUS),
      nextButton(Config::PIN_NEXT),
      volumeButton(Config::PIN_VOLUME),
      positionButton(Config::PIN_POSITION),
      buzzer(Config::PIN_BUZZER) {}

void App::begin()
{
    encoder.begin();
    centerButton.begin();
    previousButton.begin();
    nextButton.begin();
    volumeButton.begin();
    positionButton.begin();
    buzzer.begin();
}

void App::poll(uint32_t nowMs)
{
    Input in = readInput();

    if (!handleCenterHold(in.center)) {
        if (menu.isOpen()) {
            handleMenu(in);
        } else {
            handlePlayer(in);
        }
    }

    player.update(nowMs);   // a musica continua andando com o menu aberto
}

App::Input App::readInput()
{
    Input in;
    in.rotation = encoder.readRotate();
    in.center   = centerButton.poll();
    in.previous = previousButton.clicked();
    in.next     = nextButton.clicked();
    in.volume   = volumeButton.clicked();
    in.position = positionButton.clicked();
    return in;
}

// -----------------------------------------------------------------------------
//  Pressao longa do botao central (vale com o menu aberto ou fechado)
// -----------------------------------------------------------------------------

bool App::handleCenterHold(ButtonEvent center)
{
    switch (center) {
        case ButtonEvent::HoldReached:      // 3 s: avisa que soltar agora abre o menu
            buzzer.click();
            return true;

        case ButtonEvent::LongRelease:      // soltou entre 3 e 6 s
            buzzer.longClick();
            if (menu.isOpen()) menu.close();
            else menu.open();
            return true;

        case ButtonEvent::VeryLongHold:     // 6 s: desliga
            buzzer.longClick();
            menu.close();
            device.powerOff();              // na placa nao retorna
            return true;

        default:
            return false;
    }
}

// -----------------------------------------------------------------------------
//  Menu aberto
// -----------------------------------------------------------------------------

void App::handleMenu(const Input &in)
{
    if (in.rotation != 0) {
        menu.rotate(in.rotation);
    }

    if (in.center == ButtonEvent::Click) {
        buzzer.click();
        menu.select();
    } else if (in.previous) {
        buzzer.click();
        menu.back();
    }
    // Vol, Pos e Next nao fazem nada com o menu aberto.
}

// -----------------------------------------------------------------------------
//  Menu fechado (tela do player)
// -----------------------------------------------------------------------------

void App::handlePlayer(const Input &in)
{
    if (in.rotation != 0) {
        player.rotate(in.rotation);
    }

    if (in.center == ButtonEvent::Click) {
        buzzer.click();
        player.togglePlay();
    }

    // Cada comando devolve false quando e ignorado pelo modo atual: sem bipe.
    if (in.position && player.togglePositionMode()) buzzer.click();
    if (in.volume   && player.toggleVolumeMode())   buzzer.click();
    if (in.previous && player.previous())           buzzer.click();
    if (in.next     && player.next())               buzzer.click();
}
