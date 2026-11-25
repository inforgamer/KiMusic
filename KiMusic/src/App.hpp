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
//  Menu aberto (o giro do encoder nao faz nada; so o botao dele, o central):
//    Vol / Pos ............. sobe / desce (no brilho em edicao: mais / menos)
//    Prev .................. volta uma pagina
//    Next / toque central .. escolhe
//    toque duplo central ... fecha o menu
//
//  Os botoes sao lidos numa tarefa a cada 5 ms (nao no loop): o loop pode demorar
//  redesenhando a tela sem perder um toque rapido nem estourar a janela do toque duplo.
//
//  Desligar e so pelo menu (Configuracoes > Desligar). Para ligar: segurar o
//  central 2 s (ver system/Power).
//
//  Toda a logica fica em Player, Menu e Device; aqui so ha o roteamento.
// =============================================================================
#ifndef APP_HPP
#define APP_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "core/Player.hpp"
#include "core/Menu.hpp"
#include "core/DeviceControl.hpp"
#include "input/Encoder.hpp"
#include "input/Button.hpp"

class App
{
    public:
        App(Player &player, Menu &menu, DeviceControl &device);

        void begin();                  // configura os pinos e inicia a leitura dos botoes
        void poll(uint32_t nowMs);     // chamar no loop
        void discardInput();           // esquece os toques ainda nao tratados

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

        // Toques desde a ultima volta do loop.
        struct Input
        {
            int rotation = 0;
            ButtonEvent center = ButtonEvent::None;
            bool previous = false;
            bool next = false;
            bool volume = false;
            bool position = false;

            bool any() const
            {
                return rotation != 0 || center != ButtonEvent::None ||
                       previous || next || volume || position;
            }
        };

        // Preenchido pela tarefa de leitura; o loop pega e zera (protegido por inputLock).
        Input pending;
        portMUX_TYPE inputLock = portMUX_INITIALIZER_UNLOCKED;
        static void inputTask(void *self);
        void sampleButtons();

        Input readInput();
        void handleMenu(const Input &in);
        void handlePlayer(const Input &in);
};

#endif
