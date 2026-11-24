// =============================================================================
//  view/ScreenView.cpp  -  Implementacao (ver ScreenView.hpp)
// =============================================================================
#include "view/ScreenView.hpp"
#include "core/Menu.hpp"

static_assert(Screen::MENU_ROWS == Menu::VISIBLE_ROWS,
              "A tela precisa ter uma linha para cada linha visivel do menu");

ScreenView::ScreenView(Screen &s) : screen(s) {}

void ScreenView::trackChanged(const TrackSource &source, size_t index, const Mp3Info &info)
{
    Serial.printf("[%u/%u] %s - %s (%lu s)\r\n",
                  (unsigned)(index + 1), (unsigned)source.count(),
                  info.artist.c_str(), info.title.c_str(), (unsigned long)info.duration);

    screen.setMusicInfo(info.title.c_str(), info.artist.c_str(), info.album.c_str());

    // A decodificacao da capa trava o loop por algumas centenas de ms.
    uint32_t start = millis();
    bool hasCover = source.loadCover(index, info, screen.coverBuffer());
    screen.showCover(hasCover);
    Serial.printf("Capa: %s (%lu ms)\r\n", hasCover ? "ok" : "nenhuma",
                  (unsigned long)(millis() - start));
}

void ScreenView::progressChanged(uint32_t elapsed, uint32_t duration)
{
    screen.setProgress(elapsed, duration);
}

void ScreenView::statusChanged(bool playing, PlayerMode mode)
{
    const char *modeName = mode == PlayerMode::Volume   ? "Volume" :
                           mode == PlayerMode::Position ? "Posicao" : "Normal";
    Serial.printf("%s | modo %s\r\n", playing ? "Play" : "Pause", modeName);
    screen.setStatus(playing, mode);
}

void ScreenView::volumeChanged(int volume)
{
    Serial.printf("Volume: %d\r\n", volume);
    screen.setVolume(volume);
}

void ScreenView::menuChanged(const std::string &title, const std::vector<std::string> &rows,
                             int selectedRow)
{
    const char *ptrs[Screen::MENU_ROWS];
    int count = (int)rows.size() < Screen::MENU_ROWS ? (int)rows.size() : Screen::MENU_ROWS;
    for (int i = 0; i < count; i++) {
        ptrs[i] = rows[i].c_str();
    }
    screen.showMenu(title.c_str(), ptrs, count, selectedRow);
}

void ScreenView::menuClosed()
{
    screen.hideMenu();
}
