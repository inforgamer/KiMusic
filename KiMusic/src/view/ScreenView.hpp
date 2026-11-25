// =============================================================================
//  view/ScreenView.hpp  -  Liga o nucleo (Player/Menu) ao display
// -----------------------------------------------------------------------------
//  Implementa PlayerView: cada aviso do nucleo vira uma chamada da Screen e
//  uma linha de log no Serial. Tambem decodifica a capa da musica, mas so depois
//  que ela fica COVER_DELAY_MS na tela (quem esta pulando musicas nao precisa
//  de capas que nem vai ver). A decodificacao (PNG leva ~1 s) roda numa tarefa
//  propria, para a tela e os botoes nao travarem enquanto isso.
// =============================================================================
#ifndef VIEW_SCREENVIEW_HPP
#define VIEW_SCREENVIEW_HPP

#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
        static constexpr uint32_t COVER_TASK_STACK = 8 * 1024;

        Screen &screen;

        // Capa pedida e ainda nao entregue a tarefa (source == nullptr: nenhuma).
        const TrackSource *coverSource = nullptr;
        std::string coverPath;
        Mp3Info coverInfo;
        uint32_t coverRequestedAt = 0;
        uint32_t trackId = 0;          // muda a cada musica: descarta capa atrasada

        // Trabalho da tarefa. O loop so mexe nisto com decoding == false;
        // a tarefa so mexe com decoding == true e zera no fim.
        TaskHandle_t coverTask = nullptr;
        std::atomic<bool> decoding{false};   // atomico: o resultado gravado antes fica visivel depois
        bool waitingResult = false;    // (loop) entregou um trabalho e nao viu o fim
        const TrackSource *jobSource = nullptr;
        std::string jobPath;
        Mp3Info jobInfo;
        uint32_t jobId = 0;
        bool jobOk = false;
        uint32_t jobMs = 0;

        static void coverTaskEntry(void *self);
};

#endif
