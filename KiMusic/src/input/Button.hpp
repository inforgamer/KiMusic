// =============================================================================
//  input/Button.hpp  -  Botao com debounce e toque duplo
// -----------------------------------------------------------------------------
//  Botao ligado ao GND (usa INPUT_PULLUP). Debounce por tempo, sem delay().
//
//  Botao simples (doubleTapMs = 0):
//      Click       -> no momento em que e apertado
//
//  Botao com toque duplo (botao central: doubleTapMs = 400 ms):
//      DoubleClick -> no 2o aperto, se ele vier ate doubleTapMs depois de soltar o 1o
//      Click       -> doubleTapMs depois de soltar, se nao veio o 2o aperto
//  O toque simples e adiado de proposito: assim o 1o toque de um toque duplo
//  nunca chega como Click (nao da Play/Pause ao abrir o menu).
//
//  A logica fica em update() (sem Arduino) para ser testada no PC.
// =============================================================================
#ifndef INPUT_BUTTON_HPP
#define INPUT_BUTTON_HPP

#include <stdint.h>

enum class ButtonEvent
{
    None,
    Click,
    DoubleClick
};

class Button
{
    public:
        explicit Button(uint8_t pin, uint32_t doubleTapMs = 0) : pin(pin), doubleTapMs(doubleTapMs) {}

        void begin();

        // Le o pino e chama update(). Chamar a cada volta do loop (mesmo que o
        // evento seja ignorado, para o debounce acompanhar o estado do botao).
        ButtonEvent poll();

        // Atalho para botoes simples.
        bool clicked() { return poll() == ButtonEvent::Click; }

        // Maquina de estados: pressed = leitura crua do pino, now = millis().
        ButtonEvent update(bool pressed, uint32_t now)
        {
            if (pressed != lastReading) {
                lastReading = pressed;
                lastChange = now;
            }

            // Leitura estavel ha DEBOUNCE_MS e diferente do estado confirmado: mudou.
            if (now - lastChange >= DEBOUNCE_MS && pressed != stableState) {
                stableState = pressed;
                if (doubleTapMs == 0) {
                    return stableState ? ButtonEvent::Click : ButtonEvent::None;
                }
                if (stableState && waitingSecond) {   // 2o aperto dentro da janela
                    waitingSecond = false;
                    secondPress = true;
                    return ButtonEvent::DoubleClick;
                }
                if (!stableState) {                   // soltou
                    if (secondPress) {
                        secondPress = false;          // fim do toque duplo
                    } else {
                        waitingSecond = true;
                        releasedAt = now;
                    }
                }
                return ButtonEvent::None;
            }

            // Passou a janela sem 2o aperto: era um toque simples.
            if (waitingSecond && !stableState && now - releasedAt >= doubleTapMs) {
                waitingSecond = false;
                return ButtonEvent::Click;
            }
            return ButtonEvent::None;
        }

    private:
        static constexpr uint32_t DEBOUNCE_MS = 30;

        uint8_t pin;
        uint32_t doubleTapMs;

        bool stableState = false;   // estado confirmado (true = apertado)
        bool lastReading = false;   // ultima leitura crua
        uint32_t lastChange = 0;    // quando a leitura crua mudou
        bool waitingSecond = false; // soltou o 1o toque e espera um possivel 2o
        uint32_t releasedAt = 0;
        bool secondPress = false;   // o aperto atual e o 2o de um toque duplo
};

#endif
