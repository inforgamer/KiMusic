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

    // Mostra a nota agora; a capa e decodificada em poll() se a musica ficar.
    screen.showCover(false);   // escondida: a tarefa pode escrever no buffer
    trackId++;
    coverSource = &source;
    coverPath = source.path(index);
    coverInfo = info;
    coverRequestedAt = millis();
}

void ScreenView::poll(uint32_t nowMs)
{
    if (decoding) {
        return;   // a tarefa ainda esta no buffer
    }

    if (waitingResult) {
        waitingResult = false;
        if (jobId == trackId) {   // senao a musica ja mudou: o buffer fica escondido
            screen.showCover(jobOk);
            Serial.printf("Capa: %s (%lu ms)\r\n", jobOk ? "ok" : "nenhuma", (unsigned long)jobMs);
        }
    }

    if (!coverSource || nowMs - coverRequestedAt < COVER_DELAY_MS) {
        return;
    }
    if (!coverTask) {
        // Nucleo 1, mesma prioridade do loop: os dois se revezam a cada tick e o loop
        // quase so dorme, entao a capa sai quase na velocidade cheia. No nucleo 0 teria
        // de ficar na prioridade da tarefa ociosa (vigiada pelo watchdog) e levou o dobro.
        xTaskCreatePinnedToCore(coverTaskEntry, "cover", COVER_TASK_STACK, this, 1, &coverTask, 1);
        if (!coverTask) {
            return;
        }
    }
    jobSource = coverSource;
    jobPath = coverPath;
    jobInfo = coverInfo;
    jobId = trackId;
    coverSource = nullptr;
    waitingResult = true;
    decoding = true;
    xTaskNotifyGive(coverTask);
}

void ScreenView::coverTaskEntry(void *arg)
{
    ScreenView *self = static_cast<ScreenView *>(arg);
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint32_t start = millis();
        self->jobOk = self->jobSource->loadCover(self->jobPath, self->jobInfo, self->screen.coverBuffer());
        self->jobMs = millis() - start;
        self->decoding = false;
    }
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
