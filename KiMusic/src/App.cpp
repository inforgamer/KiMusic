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
//  Menu aberto
// -----------------------------------------------------------------------------

void App::handleMenu(const Input &in)
{
    if (in.rotation != 0) {
        menu.rotate(in.rotation);
    }

    if (in.center == ButtonEvent::DoubleClick) {
        menu.close();
    } else if (in.center == ButtonEvent::Click) {
        menu.select();
    }

    // Joystick: Vol sobe, Pos desce, Prev volta. Next nao faz nada no menu.
    if (in.volume)   menu.up();
    if (in.position) menu.down();
    if (in.previous) menu.back();
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
    if (in.volume)   player.toggleVolumeMode();
    if (in.previous) player.previous();
    if (in.next)     player.next();
}
