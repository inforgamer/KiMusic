// =============================================================================
//  input/Button.cpp  -  Implementacao (ver Button.hpp)
// =============================================================================
#include "input/Button.hpp"

Button::Button(uint8_t p, uint32_t hold, uint32_t veryLong)
    : pin(p), holdMs(hold), veryLongMs(veryLong) {}

void Button::begin()
{
    pinMode(pin, INPUT_PULLUP);
}

ButtonEvent Button::poll()
{
    bool reading = (digitalRead(pin) == LOW);
    uint32_t now = millis();

    if (reading != lastReading) {
        lastReading = reading;
        lastChange = now;
    }

    // Leitura estavel ha DEBOUNCE_MS e diferente do estado confirmado: mudou.
    if (now - lastChange >= DEBOUNCE_MS && reading != stableState) {
        stableState = reading;

        if (stableState) {                         // acabou de apertar
            pressStart = now;
            holdFired = false;
            veryLongFired = false;
            return holdMs == 0 ? ButtonEvent::Click : ButtonEvent::None;
        }

        // Acabou de soltar.
        if (holdMs == 0 || veryLongFired) return ButtonEvent::None;
        return holdFired ? ButtonEvent::LongRelease : ButtonEvent::Click;
    }

    // Ainda apertado: completou algum dos tempos?
    if (stableState && holdMs > 0) {
        uint32_t held = now - pressStart;
        if (veryLongMs > 0 && !veryLongFired && held >= veryLongMs) {
            veryLongFired = true;
            return ButtonEvent::VeryLongHold;
        }
        if (!holdFired && held >= holdMs) {
            holdFired = true;
            return ButtonEvent::HoldReached;
        }
    }
    return ButtonEvent::None;
}
