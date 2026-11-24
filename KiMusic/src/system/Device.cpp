// =============================================================================
//  system/Device.cpp  -  Implementacao (ver Device.hpp)
// =============================================================================
#include "system/Device.hpp"
#include "system/Power.hpp"

// Chaves na NVS (maximo 15 caracteres).
static constexpr const char *PREFS_NAMESPACE = "kimusic";
static constexpr const char *KEY_BRIGHTNESS  = "brightness";
static constexpr const char *KEY_VOLUME      = "volume";
static constexpr const char *KEY_REPEAT      = "repeat";

Device::Device(Screen &s, Player &p, AudioBackend &a, uint8_t buttonPin)
    : screen(s), player(p), audio(a), powerButtonPin(buttonPin) {}

void Device::begin()
{
    prefs.begin(PREFS_NAMESPACE, false);

    brightnessPercent = prefs.getUChar(KEY_BRIGHTNESS, BRIGHTNESS_DEFAULT);
    screen.setBrightness(brightnessPercent);

    player.setVolume(prefs.getUChar(KEY_VOLUME, Player::VOLUME_DEFAULT));

    uint8_t repeat = prefs.getUChar(KEY_REPEAT, (uint8_t)RepeatMode::Off);
    if (repeat <= (uint8_t)RepeatMode::One) {
        player.setRepeat((RepeatMode)repeat);
    }

    Serial.printf("Ajustes: brilho %d%%, volume %d, repetir %s\r\n",
                  brightnessPercent, player.volume(), repeatModeName(player.repeat()));
}

void Device::setBrightness(int percent)
{
    if (percent < BRIGHTNESS_MIN) percent = BRIGHTNESS_MIN;
    if (percent > BRIGHTNESS_MAX) percent = BRIGHTNESS_MAX;
    if (percent == brightnessPercent) {
        return;
    }
    brightnessPercent = percent;
    screen.setBrightness(percent);
    prefs.putUChar(KEY_BRIGHTNESS, (uint8_t)percent);
}

void Device::screenOff()
{
    if (screenIsOff) {
        return;
    }
    Serial.println("Tela apagada (a musica continua)");
    screenIsOff = true;
    screen.powerDown();
}

void Device::screenOn()
{
    if (!screenIsOff) {
        return;
    }
    Serial.println("Tela acesa");
    screenIsOff = false;
    screen.wakeUp(brightnessPercent);
}

void Device::saveAll()
{
    prefs.putUChar(KEY_BRIGHTNESS, (uint8_t)brightnessPercent);
    prefs.putUChar(KEY_VOLUME, (uint8_t)player.volume());
    prefs.putUChar(KEY_REPEAT, (uint8_t)player.repeat());
}

void Device::powerOff()
{
    Serial.println("Desligando...");

    audio.stop();                       // silencia o DAC
    saveAll();
    prefs.end();

    if (screenIsOff) {
        screen.wakeUp(brightnessPercent);   // para mostrar a mensagem
    }
    screen.showMessage("Desligando...");
    delay(800);
    screen.powerDown();                 // apaga a luz de fundo e poe o display para dormir

    Power::sleep(powerButtonPin);       // nao retorna
}
