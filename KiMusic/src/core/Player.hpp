// =============================================================================
//  core/Player.hpp  -  Estado e regras da reproducao
// -----------------------------------------------------------------------------
//  Guarda a lista atual, a ordem de reproducao (normal ou embaralhada), a
//  musica atual, o tempo tocado, Play/Pause, volume, modo do encoder e os
//  modos Repetir/Aleatorio, e aplica as regras de Proxima/Anterior/fim.
//
//  Nao le botoes nem desenha: recebe comandos (App, Menu) e avisa a
//  PlayerView. O tempo chega de fora em update(nowMs), entao tudo pode ser
//  testado no PC (test/host/core_test.cpp).
//
//  Audio:
//    - com AudioBackend e musica com arquivo: o tempo vem do decodificador
//    - sem backend ou musica demo (sem arquivo): o tempo e simulado
//      (1 s a cada 1000 ms em Play)
// =============================================================================
#ifndef CORE_PLAYER_HPP
#define CORE_PLAYER_HPP

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "Mp3Info.hpp"
#include "core/Types.hpp"
#include "core/TrackSource.hpp"
#include "core/PlayerView.hpp"
#include "core/AudioBackend.hpp"

class Player
{
    public:
        static constexpr int VOLUME_MIN = 0;
        static constexpr int VOLUME_MAX = 100;
        static constexpr int VOLUME_DEFAULT = 25;
        static constexpr uint32_t POSITION_STEP = 5;      // segundos por passo do encoder
        static constexpr uint32_t RESTART_THRESHOLD = 3;  // Anterior reinicia se ja tocou mais que isso
        static constexpr uint32_t SEEK_DELAY_MS = 400;    // espera o encoder parar antes de saltar no audio

        explicit Player(PlayerView &view);

        // Opcional: sem ele o tempo e sempre simulado.
        void setAudio(AudioBackend *backend) { audio = backend; }

        // Semente do embaralhamento (na placa: esp_random()).
        void setRandomSeed(uint32_t seed) { rng = seed ? seed : 1; }

        // Escolhe a lista inicial (count() >= 1) e carrega a primeira musica, pausado.
        void begin(const TrackSource &source, uint32_t nowMs);

        // Chamar sempre no loop: faz o tempo andar e detecta o fim da musica.
        void update(uint32_t nowMs);

        // ---- Comandos ----
        // Os que retornam bool devolvem false quando o comando foi ignorado
        // (ex.: Proxima com o modo volume ativo), para o App nao apitar.

        void togglePlay();
        bool next();                     // so no modo Normal
        bool previous();                 // so no modo Normal
        void playTrack(size_t index);    // indice da lista atual: carrega e da Play

        // Troca a lista (ex.: playlist escolhida no menu) e toca a partir de startIndex.
        void setSource(const TrackSource &source, size_t startIndex, bool play);

        bool toggleVolumeMode();         // ignorado no modo Posicao
        bool togglePositionMode();       // ignorado no modo Volume
        void resetMode();                // volta ao modo Normal

        void rotate(int delta);          // encoder: volume ou posicao, conforme o modo
        void setVolume(int value);       // 0..100
        void setRepeat(RepeatMode mode) { repeatMode = mode; }
        void cycleRepeat();              // Desligado -> Todas -> Uma -> Desligado

        // Aleatorio. Para ShuffleMode::All e preciso passar todas as musicas:
        // a lista atual vira 'all' (mantendo a musica que esta tocando).
        void setShuffle(ShuffleMode mode, const TrackSource *all = nullptr);

        // ---- Consultas ----

        bool isPlaying() const { return playing; }
        PlayerMode mode() const { return currentMode; }
        RepeatMode repeat() const { return repeatMode; }
        ShuffleMode shuffle() const { return shuffleMode; }
        int volume() const { return currentVolume; }
        uint32_t elapsed() const { return elapsedSeconds; }
        size_t trackIndex() const { return order.empty() ? 0 : order[position]; }   // indice na lista
        size_t trackCount() const { return source ? source->count() : 0; }
        const Mp3Info &track() const { return info; }
        const TrackSource &tracks() const { return *source; }
        const std::vector<size_t> &playOrder() const { return order; }

    private:
        PlayerView &view;
        AudioBackend *audio = nullptr;
        const TrackSource *source = nullptr;

        // Ordem de reproducao: order[position] e o indice da musica atual na lista.
        std::vector<size_t> order;
        size_t position = 0;
        uint32_t rng = 0x4B694D75;     // estado do gerador pseudoaleatorio (xorshift32)

        Mp3Info info;                  // dados da musica atual
        std::string currentPath;       // arquivo da musica atual ("" = demo)
        bool playing = false;
        PlayerMode currentMode = PlayerMode::Normal;
        RepeatMode repeatMode = RepeatMode::Off;
        ShuffleMode shuffleMode = ShuffleMode::Off;
        int currentVolume = VOLUME_DEFAULT;

        uint32_t elapsedSeconds = 0;
        uint32_t now = 0;              // ultimo tempo recebido em update()
        uint32_t lastSecond = 0;       // relogio simulado: quando o ultimo segundo foi contado

        bool audioLoaded = false;      // o backend esta com a musica atual aberta
        bool audioError = false;       // start() falhou: pular a musica no proximo update()
        bool seekPending = false;      // posicao mudou e o audio ainda nao saltou
        uint32_t seekRequestedAt = 0;

        // ---- Ordem ----
        void buildOrder(size_t first);           // sequencial ou embaralhada, com 'first' na frente
        uint32_t nextRandom();

        // ---- Musica ----
        void loadAt(size_t orderPosition);       // carrega order[orderPosition]
        void restartTrack();
        void tickSecond();                       // relogio simulado: +1 s
        void finishTrack();
        void seek(int steps);

        // ---- Audio ----
        bool usesAudio() const { return audio && !currentPath.empty(); }
        void audioPlayFrom(uint32_t seconds);
        void audioHalt();

        void notifyProgress() { view.progressChanged(elapsedSeconds, info.duration); }
        void notifyStatus() { view.statusChanged(playing, currentMode); }
};

#endif
