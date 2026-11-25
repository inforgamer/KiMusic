// =============================================================================
//  system/Device.cpp  -  Implementacao (ver Device.hpp)
// =============================================================================
#include "system/Device.hpp"
#include "system/Power.hpp"
#include "system/UsbDisk.hpp"

// Chaves na NVS (maximo 15 caracteres).
static constexpr const char *PREFS_NAMESPACE = "kimusic";
static constexpr const char *KEY_BRIGHTNESS  = "brightness";
static constexpr const char *KEY_VOLUME      = "volume";
static constexpr const char *KEY_REPEAT      = "repeat";

Device::Device(Screen &s, Player &p, AudioBackend &a, uint8_t buttonPin, uint8_t usbExit,
               uint8_t battery, uint8_t vbus)
    : screen(s), player(p), audio(a), powerButtonPin(buttonPin), usbExitPin(usbExit),
      batteryPin(battery), vbusPin(vbus) {}

void Device::begin()
{
    prefs.begin(PREFS_NAMESPACE, false);
    pinMode(vbusPin, INPUT_PULLDOWN);   // sem o divisor ligado: "sem cabo"

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

void Device::update(uint32_t nowMs)
{
    int charging = digitalRead(vbusPin) == HIGH;
    if (charging == lastCharging && nowMs - lastBatteryMs < BATTERY_PERIOD_MS) {
        return;
    }
    lastCharging = charging;
    lastBatteryMs = nowMs;

    uint32_t mv = 0;
    for (int i = 0; i < 8; i++) {
        mv += analogReadMilliVolts(batteryPin);
    }
    mv = mv * 2 / 8;   // divisor 100k/100k
    // shortcut: LiPo linear de 3,3 V (0%) a 4,2 V (100%); a curva real e plana no meio,
    // trocar por uma tabela se a porcentagem andar estranho.
    int percent = ((int)mv - 3300) * 100 / (4200 - 3300);
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    screen.setBattery(percent, charging);
}

void Device::usbMode()
{
    Serial.println("Modo USB (o cartao vira pendrive no PC)");
    audio.stop();
    saveSettings();
    if (screenIsOff) {
        screen.wakeUp(brightnessPercent);
    }
    screen.showMessage("Modo USB\nEjete no PC para sair\n(ou aperte Prev)");
    UsbDisk::run(vbusPin, usbExitPin);
    screen.showMessage("Lendo o cartao...");
    cardChanged = true;   // o loop remonta o SD e recarrega as musicas (ver main.cpp)
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

void Device::saveSettings()
{
    prefs.putUChar(KEY_BRIGHTNESS, (uint8_t)brightnessPercent);
    prefs.putUChar(KEY_VOLUME, (uint8_t)player.volume());
    prefs.putUChar(KEY_REPEAT, (uint8_t)player.repeat());
}

void Device::powerOff()
{
    Serial.println("Desligando...");

    audio.stop();                       // silencia o DAC
    saveSettings();
    prefs.end();

    if (screenIsOff) {
        screen.wakeUp(brightnessPercent);   // para mostrar a mensagem
    }
    screen.showMessage("Desligando...");
    delay(800);
    screen.powerDown();                 // apaga a luz de fundo e poe o display para dormir

    Power::sleep(powerButtonPin);       // nao retorna
}
