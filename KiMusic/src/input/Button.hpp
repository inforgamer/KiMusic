// =============================================================================
//  input/Button.hpp  -  Botao com debounce e pressao longa
// -----------------------------------------------------------------------------
//  Botao ligado ao GND (usa INPUT_PULLUP). Debounce por tempo, sem delay().
//
//  Botao simples (longPressMs = 0):
//      Click     -> no momento em que e apertado
//
//  Botao com pressao longa (botao central: longPressMs = 3 s):
//      Click     -> ao SOLTAR antes de longPressMs           (Play/Pause)
//      LongPress -> uma vez, ao completar longPressMs apertado (menu)
//                   o soltar depois disso nao gera Click
// =============================================================================
#ifndef INPUT_BUTTON_HPP
#define INPUT_BUTTON_HPP

#include <Arduino.h>

enum class ButtonEvent
{
    None,
    Click,
    LongPress
};

class Button
{
    public:
        explicit Button(uint8_t pin, uint32_t longPressMs = 0);

        void begin();

        // Chamar a cada volta do loop (mesmo que o evento seja ignorado,
        // para o debounce acompanhar o estado do botao).
        ButtonEvent poll();

        // Atalho para botoes simples.
        bool clicked() { return poll() == ButtonEvent::Click; }

    private:
        static constexpr uint32_t DEBOUNCE_MS = 30;

        uint8_t pin;
        uint32_t longPressMs;

        bool stableState = false;   // estado confirmado (true = apertado)
        bool lastReading = false;   // ultima leitura crua
        uint32_t lastChange = 0;    // quando a leitura crua mudou
        uint32_t pressStart = 0;    // quando o aperto foi confirmado
        bool longFired = false;     // LongPress ja disparou neste aperto
};

#endif
