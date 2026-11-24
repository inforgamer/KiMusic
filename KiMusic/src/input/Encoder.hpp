// =============================================================================
//  input/Encoder.hpp  -  Encoder rotativo (KY-040)
// -----------------------------------------------------------------------------
//  Leitura por polling: chamar readRotate() a cada volta do loop.
//  Conta um passo na borda de descida do pino A; o pino B diz o sentido.
// =============================================================================
#ifndef INPUT_ENCODER_HPP
#define INPUT_ENCODER_HPP

#include <Arduino.h>

class Encoder
{
    public:
        Encoder(uint8_t pinA, uint8_t pinB);

        void begin();

        // +1 (horario), -1 (anti-horario) ou 0 (parado).
        int readRotate();

    private:
        uint8_t pinA;
        uint8_t pinB;
        int lastState = HIGH;
};

#endif
