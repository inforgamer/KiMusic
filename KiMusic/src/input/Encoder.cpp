// =============================================================================
//  input/Encoder.cpp  -  Pinos e interrupcao (a logica esta em Encoder.hpp)
// =============================================================================
#include "input/Encoder.hpp"

#include <Arduino.h>

static Encoder *instance = nullptr;
static uint8_t isrPinA, isrPinB;

static void IRAM_ATTR onChange()
{
    instance->step(digitalRead(isrPinA) == HIGH, digitalRead(isrPinB) == HIGH);
}

void Encoder::begin()
{
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);
    instance = this;
    isrPinA = pinA;
    isrPinB = pinB;
    attachInterrupt(pinA, onChange, CHANGE);
    attachInterrupt(pinB, onChange, CHANGE);
}

int Encoder::readRotate()
{
    noInterrupts();
    int steps = takeSteps();
    interrupts();
    return steps;
}
