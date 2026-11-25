// =============================================================================
//  App.hpp  -  Le os botoes e decide quem recebe cada comando
// -----------------------------------------------------------------------------
//  Tela apagada (Menu > Configuracoes > Desligar tela):
//    qualquer botao ou o encoder ... acende a tela (esse toque nao faz mais nada)
//
//  Menu fechado:
//    encoder ............... volume ou posicao (conforme o modo)
//    toque central ......... Play/Pause (confirmado 400 ms depois, ver Button)
//    toque duplo central ... abre o menu (sem mexer no Play/Pause)
//    Vol (cima) ............ liga/desliga o modo Volume
//    Pos (baixo) ........... liga/desliga o modo Posicao
//    Prev (esquerda) ....... musica anterior / volta ao inicio
//    Next (direita) ........ proxima musica
//
//  Menu aberto:
//    encoder ............... move a selecao (ou ajusta o brilho em edicao)
//    Vol / Pos ............. sobe / desce (no brilho em edicao: mais / menos)
//    Prev .................. volta uma pagina
//    toque central ......... escolhe
//    toque duplo central ... fecha o menu
//    Next .................. ignorado
//
//  Desligar e so pelo menu (Configuracoes > Desligar). Para ligar: segurar o
//  central 2 s (ver system/Power).
//
//  Toda a logica fica em Player, Menu e Device; aqui so ha o roteamento.
// =============================================================================
#ifndef APP_HPP
#define APP_HPP

#include "core/Player.hpp"
#include "core/Menu.hpp"
#include "core/DeviceControl.hpp"
#include "input/Encoder.hpp"
#include "input/Button.hpp"

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

            bool any() const
            {
                return rotation != 0 || center != ButtonEvent::None ||
                       previous || next || volume || position;
            }
        };

        Input readInput();
        void handleMenu(const Input &in);
        void handlePlayer(const Input &in);
};

#endif
