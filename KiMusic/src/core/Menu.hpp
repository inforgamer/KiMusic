// =============================================================================
//  core/Menu.hpp  -  Navegacao do menu
// -----------------------------------------------------------------------------
//  Aberto com toque duplo no botao central (ver App). Paginas:
//
//    Menu
//     |- Musicas ............... musicas da lista atual; escolher uma toca ela (+ Voltar)
//     |- Playlists ............. Favoritas, Todas as musicas, playlists .m3u, Voltar
//     |- Aleatorio: <modo> ..... Desligado -> Playlist -> Geral
//     |- Repetir: <modo> ....... Desligado -> Todas -> Uma
//     |- Favoritar musica ...... adiciona a musica atual na playlist Favoritas
//     |- Configuracoes
//     |    |- Brilho: NN% ...... clique para ajustar com o encoder, clique para salvar
//     |    |- Desligar tela .... apaga so a tela; a musica continua
//     |    |- Desligar ......... desliga o aparelho (para ligar: segurar o central 2 s)
//     |    '- Voltar
//     '- Voltar ................ fecha o menu
//
//  Controles (o App traduz os botoes nestes comandos):
//    encoder         -> rotate()   move a selecao um item (ou ajusta o brilho em edicao)
//    Vol / Pos       -> up()/down()  sobe/desce (no brilho em edicao: mais/menos)
//    Prev            -> back()       volta uma pagina
//    toque central   -> select()   escolhe o item
//    toque duplo     -> close()    fecha de qualquer pagina
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
#include "core/Catalog.hpp"
#include "core/DeviceControl.hpp"

class Menu
{
    public:
        static constexpr int VISIBLE_ROWS = 7;

        enum class Page
        {
            Closed,
            Main,
            Tracks,
            Playlists,
            Settings
        };

        // Itens da pagina principal (na ordem da tela).
        enum MainItem
        {
            ITEM_TRACKS = 0,
            ITEM_PLAYLISTS,
            ITEM_SHUFFLE,
            ITEM_REPEAT,
            ITEM_FAVORITE,
            ITEM_SETTINGS,
            ITEM_BACK,
            MAIN_ITEM_COUNT
        };

        // Itens de Configuracoes.
        enum SettingsItem
        {
            SET_BRIGHTNESS = 0,
            SET_SCREEN_OFF,
            SET_POWER_OFF,
            SET_BACK,
            SETTINGS_ITEM_COUNT
        };

        // catalog e device podem ser nullptr (itens correspondentes nao fazem nada).
        Menu(Player &player, PlayerView &view, Catalog *catalog, DeviceControl *device);

        bool isOpen() const { return currentPage != Page::Closed; }
        Page page() const { return currentPage; }
        int selected() const { return selection; }
        bool isEditing() const { return editingBrightness; }

        void open();               // abre na pagina principal e volta o encoder ao modo Normal
        void close();
        void rotate(int delta);
        void up();
        void down();
        void select();
        void back();

    private:
        Player &player;
        PlayerView &view;
        Catalog *catalog;
        DeviceControl *device;

        Page currentPage = Page::Closed;
        int selection = 0;              // indice do item na pagina atual
        bool editingBrightness = false; // encoder ajusta o brilho em vez de mover a selecao
        std::string notice;             // aviso temporario no titulo (ex.: "Adicionada!")

        void goTo(Page page, int item);
        void selectMain();
        void selectPlaylist();
        void selectSettings();
        void cycleShuffle();

        // Linhas da pagina Playlists: Favoritas, Todas as musicas, demais playlists, Voltar.
        struct PlaylistItem
        {
            enum Kind { Favorites, AllTracks, Playlist, Back } kind;
            long playlist;   // indice no Catalog (-1 = nao existe)
        };
        std::vector<PlaylistItem> playlistItems() const;
        void adjust(int delta);                     // brilho em edicao

        int itemCount() const;
        std::string itemText(int item) const;
        std::string title() const;
        void render();
};

#endif
