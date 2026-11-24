// =============================================================================
//  core/Menu.cpp  -  Implementacao (ver Menu.hpp)
// =============================================================================
#include "core/Menu.hpp"

Menu::Menu(Player &p, PlayerView &v, Catalog *c, DeviceControl *d)
    : player(p), view(v), catalog(c), device(d) {}

// -----------------------------------------------------------------------------
//  Abrir / fechar / navegar
// -----------------------------------------------------------------------------

void Menu::open()
{
    player.resetMode();   // sai do modo Volume/Posicao para o encoder ficar livre
    notice.clear();
    editingBrightness = false;
    goTo(Page::Main, ITEM_TRACKS);
}

void Menu::close()
{
    if (currentPage == Page::Closed) {
        return;
    }
    currentPage = Page::Closed;
    editingBrightness = false;
    view.menuClosed();
}

void Menu::goTo(Page page, int item)
{
    currentPage = page;
    selection = item;
    render();
}

void Menu::rotate(int delta)
{
    if (!isOpen() || delta == 0) {
        return;
    }
    notice.clear();

    if (editingBrightness && device) {
        device->setBrightness(device->brightness() + delta * DeviceControl::BRIGHTNESS_STEP);
        render();
        return;
    }

    int count = itemCount();
    selection = ((selection + delta) % count + count) % count;   // da a volta nas pontas
    render();
}

void Menu::back()
{
    notice.clear();
    if (editingBrightness) {
        editingBrightness = false;      // sai da edicao (o brilho ja foi aplicado)
        render();
        return;
    }

    switch (currentPage) {
        case Page::Tracks:    goTo(Page::Main, ITEM_TRACKS);    break;
        case Page::Playlists: goTo(Page::Main, ITEM_PLAYLISTS); break;
        case Page::Settings:  goTo(Page::Main, ITEM_SETTINGS);  break;
        default:              close();                          break;
    }
}

void Menu::select()
{
    notice.clear();
    switch (currentPage) {
        case Page::Main:      selectMain();     break;
        case Page::Playlists: selectPlaylist(); break;
        case Page::Settings:  selectSettings(); break;

        case Page::Tracks:
            player.playTrack((size_t)selection);
            close();
            break;

        case Page::Closed:
            break;
    }
}

// -----------------------------------------------------------------------------
//  Acoes de cada pagina
// -----------------------------------------------------------------------------

void Menu::selectMain()
{
    switch (selection) {
        case ITEM_TRACKS:
            goTo(Page::Tracks, (int)player.trackIndex());   // comeca na musica atual
            break;

        case ITEM_PLAYLISTS:
            goTo(Page::Playlists, 0);
            break;

        case ITEM_SHUFFLE:
            cycleShuffle();
            render();
            break;

        case ITEM_REPEAT:
            player.cycleRepeat();
            render();
            break;

        case ITEM_FAVORITE:
            if (!catalog || player.tracks().path(player.trackIndex()).empty()) {
                notice = "Sem cartão SD";               // musica demo nao tem arquivo
            } else if (catalog->addToFavorites(player.tracks(), player.trackIndex())) {
                notice = "Adicionada às Favoritas";
            } else {
                notice = "Já está nas Favoritas";
            }
            render();
            break;

        case ITEM_SETTINGS:
            goTo(Page::Settings, SET_BRIGHTNESS);
            break;

        default:
            close();
            break;
    }
}

void Menu::cycleShuffle()
{
    switch (player.shuffle()) {
        case ShuffleMode::Off:
            player.setShuffle(ShuffleMode::Playlist);
            break;
        case ShuffleMode::Playlist:
            // Geral precisa de todas as musicas; sem SD continua em Playlist -> Desligado.
            if (catalog && catalog->allTracks().count() > 0) {
                player.setShuffle(ShuffleMode::All, &catalog->allTracks());
            } else {
                player.setShuffle(ShuffleMode::Off);
            }
            break;
        case ShuffleMode::All:
            player.setShuffle(ShuffleMode::Off);
            break;
    }
}

void Menu::selectPlaylist()
{
    if (!catalog) {
        return;
    }

    // Linha 0 = todas as musicas; linhas seguintes = playlists.
    const TrackSource *list = nullptr;
    if (selection == 0) {
        list = &catalog->allTracks();
    } else {
        list = catalog->openPlaylist((size_t)(selection - 1));
    }

    if (!list || list->count() == 0) {
        notice = "Playlist vazia";
        render();
        return;
    }
    player.setSource(*list, 0, true);
    close();
}

void Menu::selectSettings()
{
    switch (selection) {
        case SET_BRIGHTNESS:
            if (device) {
                editingBrightness = !editingBrightness;   // 1o clique edita, 2o confirma
                render();
            }
            break;

        case SET_POWER_OFF:
            close();
            if (device) {
                device->powerOff();
            }
            break;

        default:
            goTo(Page::Main, ITEM_SETTINGS);
            break;
    }
}

// -----------------------------------------------------------------------------
//  Conteudo das paginas
// -----------------------------------------------------------------------------

int Menu::itemCount() const
{
    switch (currentPage) {
        case Page::Main:      return MAIN_ITEM_COUNT;
        case Page::Settings:  return SETTINGS_ITEM_COUNT;
        case Page::Playlists: return 1 + (catalog ? (int)catalog->playlistCount() : 0);
        case Page::Tracks:    return (int)player.trackCount();
        default:              return 0;
    }
}

std::string Menu::itemText(int item) const
{
    switch (currentPage) {
        case Page::Main:
            switch (item) {
                case ITEM_TRACKS:    return "Músicas";
                case ITEM_PLAYLISTS: return "Playlists";
                case ITEM_SHUFFLE:   return std::string("Aleatório: ") + shuffleModeName(player.shuffle());
                case ITEM_REPEAT:    return std::string("Repetir: ") + repeatModeName(player.repeat());
                case ITEM_FAVORITE:  return "Favoritar música";
                case ITEM_SETTINGS:  return "Configurações";
                default:             return "Voltar";
            }

        case Page::Settings:
            switch (item) {
                case SET_BRIGHTNESS: {
                    std::string value = std::to_string(device ? device->brightness() : 100) + "%";
                    return editingBrightness ? "Brilho: < " + value + " >" : "Brilho: " + value;
                }
                case SET_POWER_OFF: return "Desligar";
                default:            return "Voltar";
            }

        case Page::Playlists:
            return item == 0 ? "Todas as músicas" : catalog->playlistName((size_t)(item - 1));

        case Page::Tracks: {
            // "> " marca a que esta carregada no player.
            const char *mark = ((size_t)item == player.trackIndex()) ? "> " : "  ";
            return mark + player.tracks().name((size_t)item);
        }

        default:
            return std::string();
    }
}

std::string Menu::title() const
{
    if (!notice.empty()) {
        return notice;
    }
    switch (currentPage) {
        case Page::Tracks:
            return "Músicas (" + std::to_string(selection + 1) + "/" + std::to_string(itemCount()) + ")";
        case Page::Playlists:
            return "Playlists";
        case Page::Settings:
            return editingBrightness ? "Gire para ajustar" : "Configurações";
        default:
            return "Menu";
    }
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
