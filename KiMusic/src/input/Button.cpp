// =============================================================================
//  input/Button.cpp  -  Leitura do pino (a logica esta em Button.hpp)
// =============================================================================
#include "input/Button.hpp"

#include <Arduino.h>

void Button::begin()
{
    pinMode(pin, INPUT_PULLUP);
}

ButtonEvent Button::poll()
{
    return update(digitalRead(pin) == LOW, millis());
}
