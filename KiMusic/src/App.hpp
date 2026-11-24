// =============================================================================
//  App.hpp  -  Le os botoes e decide quem recebe cada comando
// -----------------------------------------------------------------------------
//  Menu fechado:
//    encoder ............... volume ou posicao (conforme o modo)
//    clique central ........ Play/Pause
//    segurar central (3 s) . abre o menu
//    Vol (cima) ............ liga/desliga o modo Volume
//    Pos (baixo) ........... liga/desliga o modo Posicao
//    Prev (esquerda) ....... musica anterior / volta ao inicio
//    Next (direita) ........ proxima musica
//
//  Menu aberto:
//    encoder ............... move a selecao
//    clique central ........ escolhe
//    Prev .................. volta uma pagina / fecha
//    segurar central (3 s) . fecha o menu
//    Vol, Pos, Next ........ ignorados
//
//  Toda a logica fica em Player e Menu; aqui so ha o roteamento e os bipes.
// =============================================================================
#ifndef APP_HPP
#define APP_HPP

#include "core/Player.hpp"
#include "core/Menu.hpp"
#include "input/Encoder.hpp"
#include "input/Button.hpp"
#include "input/Buzzer.hpp"

class App
{
    public:
        App(Player &player, Menu &menu);

        void begin();                  // configura os pinos
        void poll(uint32_t nowMs);     // chamar no loop

    private:
        Player &player;
        Menu &menu;

        Encoder encoder;
        Button centerButton;
        Button previousButton;
        Button nextButton;
        Button volumeButton;
        Button positionButton;
        Buzzer buzzer;

        // Leituras de uma volta do loop (todos os botoes sao lidos sempre,
        // para o debounce de cada um continuar correto).
        struct Input
        {
            int rotation;
            ButtonEvent center;
            bool previous;
            bool next;
            bool volume;
            bool position;
        };

        Input readInput();
        void handleMenu(const Input &in);
        void handlePlayer(const Input &in);
};

#endif
