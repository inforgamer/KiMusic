// =============================================================================
//  system/Device.hpp  -  Brilho, desligar e ajustes salvos
// -----------------------------------------------------------------------------
//  Implementa DeviceControl (usado pelo Menu e pelo App): brilho, apagar a
//  tela (a musica continua) e desligar. Tambem guarda os ajustes na memoria
//  flash (NVS, biblioteca Preferences), para voltarem iguais ao ligar:
//    - brilho da tela  (salvo a cada mudanca)
//    - volume e modo Repetir (salvos ao mudar)
//  E mostra a bateria (ADC) e se o cabo USB-C esta ligado (VBUS), e liga o modo USB.
// =============================================================================
#ifndef SYSTEM_DEVICE_HPP
#define SYSTEM_DEVICE_HPP

#include <Preferences.h>

#include "core/DeviceControl.hpp"
#include "core/Player.hpp"
#include "core/AudioBackend.hpp"
#include "view/Screen.hpp"

class Device : public DeviceControl
{
    public:
        // usbExitPin: botao que sai do modo USB sem ejetar (Prev: nao tem toque duplo,
        // entao o aperto chega na hora e pode ser descartado, ver App::discardInput).
        Device(Screen &screen, Player &player, AudioBackend &audio, uint8_t powerButtonPin,
               uint8_t usbExitPin, uint8_t batteryPin, uint8_t vbusPin);

        // Le os ajustes salvos e aplica brilho, volume e Repetir.
        // Chamar depois de screen.init() e antes de player.begin().
        void begin();

        // Chamar no loop: atualiza a bateria na tela (a cada 2 s ou quando o cabo muda).
        void update(uint32_t nowMs);

        // true uma vez depois do modo USB: o PC pode ter mudado o cartao inteiro.
        bool takeCardChanged() { bool c = cardChanged; cardChanged = false; return c; }

        // ---- DeviceControl ----
        int brightness() const override { return brightnessPercent; }
        void setBrightness(int percent) override;
        void screenOff() override;
        void screenOn() override;
        bool isScreenOff() const override { return screenIsOff; }
        void saveSettings() override;
        void usbMode() override;
        void powerOff() override;

    private:
        static constexpr int BRIGHTNESS_DEFAULT = 80;
        static constexpr uint32_t BATTERY_PERIOD_MS = 2000;

        Screen &screen;
        Player &player;
        AudioBackend &audio;
        uint8_t powerButtonPin;
        uint8_t usbExitPin;
        uint8_t batteryPin;
        uint8_t vbusPin;
        uint32_t lastBatteryMs = 0;
        int lastCharging = -1;     // -1: ainda nao mostrou
        bool cardChanged = false;
        Preferences prefs;
        int brightnessPercent = BRIGHTNESS_DEFAULT;
        bool screenIsOff = false;
};

#endif
