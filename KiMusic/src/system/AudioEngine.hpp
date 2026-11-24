// =============================================================================
//  system/AudioEngine.hpp  -  Decodificador MP3 + DAC PCM5102A (I2S)
// -----------------------------------------------------------------------------
//  Implementa AudioBackend com a biblioteca ESP8266Audio:
//
//    SD (arquivo) -> buffer de 16 KB na RAM -> decodificador MP3 (libmad)
//                 -> saida I2S (conta as amostras) -> PCM5102A -> P2
//
//  - A decodificacao roda numa tarefa propria no nucleo 0; o loop do Arduino
//    (tela, botoes) fica no nucleo 1. Assim redesenhar a tela ou decodificar
//    a capa nao corta o som.
//  - Um mutex protege o decodificador: os comandos (start/pause/...) chegam
//    do loop e a tarefa so decodifica entre eles.
//  - O buffer de 16 KB (~0,4 s a 320 kbps) cobre as pausas do SD, que divide
//    o barramento SPI com o display.
//  - O pino XSMT do PCM5102A silencia o DAC em pause/stop (sem estalo).
//  - Tempo tocado = amostras enviadas ao DAC / taxa de amostragem.
// =============================================================================
#ifndef SYSTEM_AUDIOENGINE_HPP
#define SYSTEM_AUDIOENGINE_HPP

#include <Arduino.h>
#include <FS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "core/AudioBackend.hpp"

class AudioFileSourceFS;
class AudioFileSourceBuffer;
class AudioGeneratorMP3;
class CountingI2S;

class AudioEngine : public AudioBackend
{
    public:
        AudioEngine(fs::FS &fs, uint8_t pinBck, uint8_t pinLrck, uint8_t pinDout, uint8_t pinMute);

        // Configura o I2S e cria a tarefa de decodificacao. Chamar uma vez no setup().
        void begin();

        // ---- AudioBackend ----
        bool start(const std::string &path, const Mp3Info &info, uint32_t startSeconds) override;
        void pause() override;
        void resume() override;
        void stop() override;
        void setVolume(int volume) override;
        uint32_t position() const override;
        bool finished() const override { return isFinished; }

    private:
        enum class State : uint8_t { Idle, Playing, Paused };

        static constexpr uint32_t BUFFER_BYTES = 16 * 1024;
        static constexpr uint32_t TASK_STACK = 8 * 1024;   // o libmad usa bastante pilha
        static constexpr UBaseType_t TASK_PRIORITY = 3;   // acima do loop do Arduino (1)
        static constexpr BaseType_t TASK_CORE = 0;

        fs::FS &fs;
        uint8_t pinBck, pinLrck, pinDout, pinMute;

        SemaphoreHandle_t lock = nullptr;
        TaskHandle_t task = nullptr;

        CountingI2S *output = nullptr;
        AudioGeneratorMP3 *mp3 = nullptr;
        AudioFileSourceFS *file = nullptr;
        AudioFileSourceBuffer *buffered = nullptr;

        bool decoderOpen = false;          // mp3->begin() foi chamado e ainda nao teve stop()
        volatile State state = State::Idle;
        volatile bool isFinished = false;
        uint32_t startOffsetSeconds = 0;   // posicao em que a musica comecou a tocar

        static void taskEntry(void *self);
        void taskLoop();

        void closeLocked();                // fecha decodificador e arquivo (com o mutex)
        void mute(bool silent);
};

#endif
