// =============================================================================
//  core/Menu.cpp  -  Implementacao (ver Menu.hpp)
// =============================================================================
#include "core/Menu.hpp"

Menu::Menu(Player &p, PlayerView &v) : player(p), view(v) {}

// -----------------------------------------------------------------------------
//  Abrir / fechar / navegar
// -----------------------------------------------------------------------------

void Menu::open()
{
    player.resetMode();   // sai do modo Volume/Posicao para o encoder ficar livre
    currentPage = Page::Main;
    selection = ITEM_TRACKS;
    render();
}

void Menu::close()
{
    if (currentPage == Page::Closed) {
        return;
    }
    currentPage = Page::Closed;
    view.menuClosed();
}

void Menu::rotate(int delta)
{
    if (!isOpen() || delta == 0) {
        return;
    }
    int count = itemCount();
    selection = ((selection + delta) % count + count) % count;   // da a volta nas pontas
    render();
}

void Menu::back()
{
    if (currentPage == Page::Tracks) {
        currentPage = Page::Main;
        selection = ITEM_TRACKS;
        render();
    } else {
        close();
    }
}

void Menu::select()
{
    switch (currentPage) {
        case Page::Main:
            if (selection == ITEM_TRACKS) {
                currentPage = Page::Tracks;
                selection = (int)player.trackIndex();   // comeca na musica atual
                render();
            } else if (selection == ITEM_REPEAT) {
                player.cycleRepeat();
                render();
            } else {
                close();
            }
            break;

        case Page::Tracks:
            player.playTrack((size_t)selection);
            close();
            break;

        case Page::Closed:
            break;
    }
}

// -----------------------------------------------------------------------------
//  Conteudo das paginas
// -----------------------------------------------------------------------------

int Menu::itemCount() const
{
    return currentPage == Page::Main ? MAIN_ITEM_COUNT : (int)player.trackCount();
}

std::string Menu::itemText(int item) const
{
    if (currentPage == Page::Main) {
        switch (item) {
            case ITEM_TRACKS: return "Músicas";
            case ITEM_REPEAT: return std::string("Repetir: ") + repeatModeName(player.repeat());
            default:          return "Voltar";
        }
    }

    // Pagina de musicas: "> " marca a que esta carregada no player.
    const char *mark = ((size_t)item == player.trackIndex()) ? "> " : "  ";
    return mark + player.tracks().name((size_t)item);
}

std::string Menu::title() const
{
    if (currentPage == Page::Main) {
        return "Menu";
    }
    return "Músicas (" + std::to_string(selection + 1) + "/" + std::to_string(itemCount()) + ")";
}

void Menu::render()
{
    const int count = itemCount();

    // Janela de VISIBLE_ROWS linhas com o item selecionado no meio (quando possivel).
    int first = selection - VISIBLE_ROWS / 2;
    if (first > count - VISIBLE_ROWS) first = count - VISIBLE_ROWS;
    if (first < 0) first = 0;
    int last = first + VISIBLE_ROWS;
    if (last > count) last = count;

    std::vector<std::string> rows;
    rows.reserve(last - first);
    for (int item = first; item < last; item++) {
        rows.push_back(itemText(item));
    }

    view.menuChanged(title(), rows, selection - first);
}
