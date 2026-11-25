// =============================================================================
//  view/ScreenView.hpp  -  Liga o nucleo (Player/Menu) ao display
// -----------------------------------------------------------------------------
//  Implementa PlayerView: cada aviso do nucleo vira uma chamada da Screen e
//  uma linha de log no Serial. Tambem decodifica a capa da musica, mas so depois
//  que ela fica COVER_DELAY_MS na tela: decodificar trava o loop (PNG ~1 s), e
//  quem esta pulando musicas nao precisa esperar por capas que nem vai ver.
// =============================================================================
#ifndef VIEW_SCREENVIEW_HPP
#define VIEW_SCREENVIEW_HPP

#include "core/PlayerView.hpp"
#include "view/Screen.hpp"

class ScreenView : public PlayerView
{
    public:
        explicit ScreenView(Screen &screen);

        // Chamar no loop: decodifica a capa pendente quando der o tempo.
        void poll(uint32_t nowMs);

        void trackChanged(const TrackSource &source, size_t index, const Mp3Info &info) override;
        void progressChanged(uint32_t elapsed, uint32_t duration) override;
        void statusChanged(bool playing, PlayerMode mode) override;
        void volumeChanged(int volume) override;
        void menuChanged(const std::string &title, const std::vector<std::string> &rows,
                         int selectedRow) override;
        void menuClosed() override;

    private:
        static constexpr uint32_t COVER_DELAY_MS = 500;

        Screen &screen;

        // Capa ainda nao decodificada (source == nullptr: nenhuma).
        const TrackSource *coverSource = nullptr;
        size_t coverIndex = 0;
        Mp3Info coverInfo;
        uint32_t coverRequestedAt = 0;
};

#endif
