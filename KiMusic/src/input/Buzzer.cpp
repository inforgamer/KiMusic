// =============================================================================
//  input/Buzzer.cpp  -  Implementacao (ver Buzzer.hpp)
// =============================================================================
#include "input/Buzzer.hpp"

Buzzer::Buzzer(uint8_t p) : pin(p) {}

void Buzzer::begin()
{
    pinMode(pin, OUTPUT);
}

void Buzzer::click()
{
    tone(pin, 2000, 15);
}

void Buzzer::longClick()
{
    tone(pin, 1200, 60);
}
