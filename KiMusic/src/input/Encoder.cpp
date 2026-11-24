// =============================================================================
//  input/Encoder.cpp  -  Implementacao (ver Encoder.hpp)
// =============================================================================
#include "input/Encoder.hpp"

Encoder::Encoder(uint8_t a, uint8_t b) : pinA(a), pinB(b) {}

void Encoder::begin()
{
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);
    lastState = digitalRead(pinA);
}

int Encoder::readRotate()
{
    int stateA = digitalRead(pinA);
    if (stateA == lastState) {
        return 0;
    }
    lastState = stateA;

    if (stateA != LOW) {
        return 0;   // so a borda de descida conta
    }
    return (digitalRead(pinB) != stateA) ? 1 : -1;
}
