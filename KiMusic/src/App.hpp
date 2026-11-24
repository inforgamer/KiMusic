// =============================================================================
//  App.hpp  -  Le os botoes e decide quem recebe cada comando
// -----------------------------------------------------------------------------
//  Botao central (sempre):
//    clique ................ Play/Pause (menu fechado) ou escolher (menu aberto)
//    segurar 3 s ........... bipe de aviso; SOLTAR entre 3 e 6 s abre/fecha o menu
//    segurar 6 s ........... desliga (deep sleep). Para ligar: segurar 2 s
//
//  Menu fechado:
//    encoder ............... volume ou posicao (conforme o modo)
//    Vol (cima) ............ liga/desliga o modo Volume
//    Pos (baixo) ........... liga/desliga o modo Posicao
//    Prev (esquerda) ....... musica anterior / volta ao inicio
//    Next (direita) ........ proxima musica
//
//  Menu aberto:
//    encoder ............... move a selecao (ou ajusta o brilho em edicao)
//    Prev .................. volta uma pagina / fecha
//    Vol, Pos, Next ........ ignorados
//
//  Toda a logica fica em Player, Menu e Device; aqui so ha o roteamento e os bipes.
// =============================================================================
#ifndef APP_HPP
#define APP_HPP

#include "core/Player.hpp"
#include "core/Menu.hpp"
#include "core/DeviceControl.hpp"
#include "input/Encoder.hpp"
#include "input/Button.hpp"
#include "input/Buzzer.hpp"

class App
{
    public:
        App(Player &player, Menu &menu, DeviceControl &device);

        void begin();                  // configura os pinos
        void poll(uint32_t nowMs);     // chamar no loop

    private:
        Player &player;
        Menu &menu;
        DeviceControl &device;

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
        bool handleCenterHold(ButtonEvent center);   // true se tratou (menu/desligar)
        void handleMenu(const Input &in);
        void handlePlayer(const Input &in);
};

#endif
