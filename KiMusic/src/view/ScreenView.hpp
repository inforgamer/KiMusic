// =============================================================================
//  view/ScreenView.hpp  -  Liga o nucleo (Player/Menu) ao display
// -----------------------------------------------------------------------------
//  Implementa PlayerView: cada aviso do nucleo vira uma chamada da Screen e
//  uma linha de log no Serial. Tambem decodifica a capa quando a musica muda.
// =============================================================================
#ifndef VIEW_SCREENVIEW_HPP
#define VIEW_SCREENVIEW_HPP

#include "core/PlayerView.hpp"
#include "view/Screen.hpp"

class ScreenView : public PlayerView
{
    public:
        explicit ScreenView(Screen &screen);

        void trackChanged(const TrackSource &source, size_t index, const Mp3Info &info) override;
        void progressChanged(uint32_t elapsed, uint32_t duration) override;
        void statusChanged(bool playing, PlayerMode mode) override;
        void volumeChanged(int volume) override;
        void menuChanged(const std::string &title, const std::vector<std::string> &rows,
                         int selectedRow) override;
        void menuClosed() override;

    private:
        Screen &screen;
};

#endif
