// =============================================================================
//  core/DeviceControl.hpp  -  Ajustes do aparelho usados pelo Menu
// -----------------------------------------------------------------------------
//  Implementacao: system/Device (PWM da luz de fundo, tela, deep sleep, NVS).
// =============================================================================
#ifndef CORE_DEVICECONTROL_HPP
#define CORE_DEVICECONTROL_HPP

class DeviceControl
{
    public:
        static constexpr int BRIGHTNESS_MIN = 10;   // nunca apaga a tela de vez
        static constexpr int BRIGHTNESS_MAX = 100;
        static constexpr int BRIGHTNESS_STEP = 10;

        virtual ~DeviceControl() {}

        virtual int brightness() const = 0;            // % (BRIGHTNESS_MIN..MAX)
        virtual void setBrightness(int percent) = 0;   // aplica e salva

        // Apaga so a tela (a musica continua). Qualquer botao acende de novo (ver App).
        virtual void screenOff() = 0;
        virtual void screenOn() = 0;
        virtual bool isScreenOff() const = 0;

        virtual void powerOff() = 0;                   // salva tudo e desliga (nao retorna na placa)
};

#endif
