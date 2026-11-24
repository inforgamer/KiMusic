// =============================================================================
//  input/Buzzer.hpp  -  Bipes de confirmacao dos botoes
// -----------------------------------------------------------------------------
//  Usa tone() com duracao, que nao bloqueia o loop.
// =============================================================================
#ifndef INPUT_BUZZER_HPP
#define INPUT_BUZZER_HPP

#include <Arduino.h>

class Buzzer
{
    public:
        explicit Buzzer(uint8_t pin);

        void begin();
        void click();       // bipe curto e agudo: clique normal
        void longClick();   // bipe mais longo e grave: pressao longa (menu)

    private:
        uint8_t pin;
};

#endif
