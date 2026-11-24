// =============================================================================
//  input/Button.cpp  -  Implementacao (ver Button.hpp)
// =============================================================================
#include "input/Button.hpp"

Button::Button(uint8_t p, uint32_t longPress) : pin(p), longPressMs(longPress) {}

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

        if (stableState) {                              // acabou de apertar
            pressStart = now;
            longFired = false;
            return longPressMs == 0 ? ButtonEvent::Click : ButtonEvent::None;
        }
        if (longPressMs > 0 && !longFired) {            // soltou antes do tempo
            return ButtonEvent::Click;
        }
        return ButtonEvent::None;
    }

    // Ainda apertado: completou o tempo da pressao longa?
    if (stableState && longPressMs > 0 && !longFired && now - pressStart >= longPressMs) {
        longFired = true;
        return ButtonEvent::LongPress;
    }
    return ButtonEvent::None;
}
