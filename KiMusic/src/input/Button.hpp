// =============================================================================
//  input/Button.hpp  -  Botao com debounce e pressao longa em dois niveis
// -----------------------------------------------------------------------------
//  Botao ligado ao GND (usa INPUT_PULLUP). Debounce por tempo, sem delay().
//
//  Botao simples (holdMs = 0):
//      Click        -> no momento em que e apertado
//
//  Botao com pressao longa (botao central: holdMs = 3 s, veryLongMs = 6 s):
//      Click        -> ao SOLTAR antes de holdMs                (Play/Pause)
//      HoldReached  -> uma vez, ao completar holdMs apertado     (bipe de aviso)
//      LongRelease  -> ao SOLTAR entre holdMs e veryLongMs       (menu)
//      VeryLongHold -> uma vez, ao completar veryLongMs apertado (desligar)
//                      o soltar depois disso nao gera evento
// =============================================================================
#ifndef INPUT_BUTTON_HPP
#define INPUT_BUTTON_HPP

#include <Arduino.h>

enum class ButtonEvent
{
    None,
    Click,
    HoldReached,
    LongRelease,
    VeryLongHold
};

class Button
{
    public:
        explicit Button(uint8_t pin, uint32_t holdMs = 0, uint32_t veryLongMs = 0);

        void begin();

        // Chamar a cada volta do loop (mesmo que o evento seja ignorado,
        // para o debounce acompanhar o estado do botao).
        ButtonEvent poll();

        // Atalho para botoes simples.
        bool clicked() { return poll() == ButtonEvent::Click; }

    private:
        static constexpr uint32_t DEBOUNCE_MS = 30;

        uint8_t pin;
        uint32_t holdMs;
        uint32_t veryLongMs;

        bool stableState = false;   // estado confirmado (true = apertado)
        bool lastReading = false;   // ultima leitura crua
        uint32_t lastChange = 0;    // quando a leitura crua mudou
        uint32_t pressStart = 0;    // quando o aperto foi confirmado
        bool holdFired = false;     // HoldReached ja disparou neste aperto
        bool veryLongFired = false; // VeryLongHold ja disparou neste aperto
};

#endif
