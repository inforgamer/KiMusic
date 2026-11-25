// =============================================================================
//  input/Encoder.hpp  -  Encoder rotativo (KY-040)
// -----------------------------------------------------------------------------
//  Leitura por interrupcao nos dois pinos (A e B). Cada mudanca e passada por
//  uma tabela de 4 estados (quadratura): um passo so conta quando o encoder
//  completa o ciclo e volta ao repouso (A e B em HIGH). O repique dos contatos
//  gera ida e volta na tabela, que se cancela, entao nao conta passo dobrado.
//
//  A logica fica em step() (sem Arduino) para ser testada no PC.
// =============================================================================
#ifndef INPUT_ENCODER_HPP
#define INPUT_ENCODER_HPP

#include <stdint.h>

class Encoder
{
    public:
        Encoder(uint8_t pinA, uint8_t pinB) : pinA(pinA), pinB(pinB) {}

        void begin();

        // Soma dos passos desde a ultima chamada (+ horario, - anti-horario, 0 parado).
        int readRotate();

        // Nova leitura dos pinos (true = HIGH). Chamado pela interrupcao.
        void step(bool a, bool b)
        {
            // Indice = estado anterior (2 bits) + estado atual (2 bits).
            static const int8_t TABLE[16] = {
                 0, -1, +1,  0,
                +1,  0,  0, -1,
                -1,  0,  0, +1,
                 0, +1, -1,  0
            };
            uint8_t now = (uint8_t)((a ? 2 : 0) | (b ? 1 : 0));
            acc += TABLE[(last << 2) | now];
            last = now;
            if (now == 3) {               // repouso: fecha o passo
                if (acc >= 2) pending++;
                else if (acc <= -2) pending--;
                acc = 0;
            }
        }

        // Le e zera os passos acumulados (sem a protecao de interrupcao: use readRotate()).
        int takeSteps()
        {
            int s = pending;
            pending = 0;
            return s;
        }

    private:
        uint8_t pinA;
        uint8_t pinB;
        volatile uint8_t last = 3;   // repouso: A e B em HIGH (pull-up)
        volatile int acc = 0;        // quartos de passo desde o ultimo repouso
        volatile int pending = 0;    // passos completos ainda nao lidos
};

#endif
