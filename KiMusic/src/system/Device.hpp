// =============================================================================
//  system/Device.hpp  -  Brilho, desligar e ajustes salvos
// -----------------------------------------------------------------------------
//  Implementa DeviceControl (usado pelo Menu e pelo App): brilho, apagar a
//  tela (a musica continua) e desligar. Tambem guarda os ajustes na memoria
//  flash (NVS, biblioteca Preferences), para voltarem iguais ao ligar:
//    - brilho da tela  (salvo a cada mudanca)
//    - volume e modo Repetir (salvos ao desligar)
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
        Device(Screen &screen, Player &player, AudioBackend &audio, uint8_t powerButtonPin);

        // Le os ajustes salvos e aplica brilho, volume e Repetir.
        // Chamar depois de screen.init() e antes de player.begin().
        void begin();

        // ---- DeviceControl ----
        int brightness() const override { return brightnessPercent; }
        void setBrightness(int percent) override;
        void screenOff() override;
        void screenOn() override;
        bool isScreenOff() const override { return screenIsOff; }
        void powerOff() override;

    private:
        static constexpr int BRIGHTNESS_DEFAULT = 80;

        Screen &screen;
        Player &player;
        AudioBackend &audio;
        uint8_t powerButtonPin;
        Preferences prefs;
        int brightnessPercent = BRIGHTNESS_DEFAULT;
        bool screenIsOff = false;

        void saveAll();
};

#endif
