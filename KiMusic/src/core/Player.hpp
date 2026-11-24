// =============================================================================
//  core/Player.hpp  -  Estado e regras da reproducao
// -----------------------------------------------------------------------------
//  Guarda a musica atual, o tempo tocado, Play/Pause, volume, modo do encoder
//  e modo Repetir, e aplica as regras de Proxima/Anterior/fim da musica.
//
//  Nao le botoes nem desenha: recebe comandos (App, Menu) e avisa a
//  PlayerView. O tempo chega de fora em update(nowMs), entao tudo pode ser
//  testado no PC (test/host/core_test.cpp).
//
//  Enquanto nao existe decodificador de audio, a reproducao e simulada:
//  o tempo tocado anda 1 s a cada 1000 ms em Play.
// =============================================================================
#ifndef CORE_PLAYER_HPP
#define CORE_PLAYER_HPP

#include <stddef.h>
#include <stdint.h>

#include "Mp3Info.hpp"
#include "core/Types.hpp"
#include "core/TrackSource.hpp"
#include "core/PlayerView.hpp"

class Player
{
    public:
        static constexpr int VOLUME_MIN = 0;
        static constexpr int VOLUME_MAX = 100;
        static constexpr int VOLUME_DEFAULT = 25;
        static constexpr uint32_t POSITION_STEP = 5;      // segundos por passo do encoder
        static constexpr uint32_t RESTART_THRESHOLD = 3;  // Anterior reinicia se ja tocou mais que isso

        explicit Player(PlayerView &view);

        // Escolhe a fonte de musicas (count() >= 1) e carrega a primeira.
        void begin(const TrackSource &source, uint32_t nowMs);

        // Chamar sempre no loop: faz o tempo andar enquanto estiver em Play.
        void update(uint32_t nowMs);

        // ---- Comandos ----
        // Os que retornam bool devolvem false quando o comando foi ignorado
        // (ex.: Proxima com o modo volume ativo), para o App nao apitar.

        void togglePlay();
        bool next();                     // so no modo Normal
        bool previous();                 // so no modo Normal
        void playTrack(size_t index);    // escolhida no menu: carrega e da Play

        bool toggleVolumeMode();         // ignorado no modo Posicao
        bool togglePositionMode();       // ignorado no modo Volume
        void resetMode();                // volta ao modo Normal

        void rotate(int delta);          // encoder: volume ou posicao, conforme o modo
        void cycleRepeat();              // Desligado -> Todas -> Uma -> Desligado

        // ---- Consultas ----

        bool isPlaying() const { return playing; }
        PlayerMode mode() const { return currentMode; }
        RepeatMode repeat() const { return repeatMode; }
        int volume() const { return currentVolume; }
        uint32_t elapsed() const { return elapsedSeconds; }
        size_t trackIndex() const { return index; }
        size_t trackCount() const { return source ? source->count() : 0; }
        const Mp3Info &track() const { return info; }
        const TrackSource &tracks() const { return *source; }

    private:
        PlayerView &view;
        const TrackSource *source = nullptr;

        Mp3Info info;                    // dados da musica atual
        size_t index = 0;
        bool playing = false;
        PlayerMode currentMode = PlayerMode::Normal;
        RepeatMode repeatMode = RepeatMode::Off;
        int currentVolume = VOLUME_DEFAULT;

        uint32_t elapsedSeconds = 0;
        uint32_t now = 0;                // ultimo tempo recebido em update()
        uint32_t lastSecond = 0;         // quando o ultimo segundo foi contado

        void loadTrack(size_t newIndex);
        void restartTrack();
        void tickSecond();               // +1 s; trata o fim da musica
        void finishTrack();
        void setVolume(int value);
        void seek(int steps);

        void notifyProgress() { view.progressChanged(elapsedSeconds, info.duration); }
        void notifyStatus() { view.statusChanged(playing, currentMode); }
};

#endif
