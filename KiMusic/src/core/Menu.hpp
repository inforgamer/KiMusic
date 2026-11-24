// =============================================================================
//  core/Menu.hpp  -  Navegacao do menu
// -----------------------------------------------------------------------------
//  Aberto segurando o botao central por 3 s (ver App). Paginas:
//
//    Menu
//     |- Musicas ............ lista de todas as musicas; escolher uma toca ela
//     |- Repetir: <modo> .... cada clique alterna Desligado -> Todas -> Uma
//     '- Voltar ............. fecha o menu
//
//  Controles (o App traduz os botoes nestes comandos):
//    encoder         -> rotate()   move a selecao (da volta no fim da lista)
//    clique central  -> select()   escolhe o item
//    botao Anterior  -> back()     volta uma pagina / fecha
//    segurar central -> close()    fecha de qualquer pagina
//
//  So VISIBLE_ROWS linhas sao enviadas a PlayerView por vez: a lista rola
//  em volta do item selecionado. Assim a memoria usada pela tela nao cresce
//  com o tamanho da playlist.
// =============================================================================
#ifndef CORE_MENU_HPP
#define CORE_MENU_HPP

#include <string>
#include <vector>

#include "core/Player.hpp"
#include "core/PlayerView.hpp"

class Menu
{
    public:
        static constexpr int VISIBLE_ROWS = 7;

        enum class Page
        {
            Closed,
            Main,
            Tracks
        };

        // Itens da pagina principal (na ordem da tela).
        enum MainItem
        {
            ITEM_TRACKS = 0,
            ITEM_REPEAT,
            ITEM_BACK,
            MAIN_ITEM_COUNT
        };

        Menu(Player &player, PlayerView &view);

        bool isOpen() const { return currentPage != Page::Closed; }
        Page page() const { return currentPage; }
        int selected() const { return selection; }

        void open();               // abre na pagina principal e volta o encoder ao modo Normal
        void close();
        void rotate(int delta);
        void select();
        void back();

    private:
        Player &player;
        PlayerView &view;
        Page currentPage = Page::Closed;
        int selection = 0;         // indice do item na pagina atual

        int itemCount() const;
        std::string itemText(int item) const;
        std::string title() const;
        void render();
};

#endif
