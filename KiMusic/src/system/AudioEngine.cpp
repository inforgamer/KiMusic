// =============================================================================
//  system/AudioEngine.cpp  -  Implementacao (ver AudioEngine.hpp)
// =============================================================================
#include "system/AudioEngine.hpp"

#include <AudioFileSourceFS.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <driver/i2s.h>

// -----------------------------------------------------------------------------
//  Saida I2S que conta as amostras tocadas
// -----------------------------------------------------------------------------
//  Tambem troca o stop() da biblioteca: o original marca o driver como
//  desligado sem desinstalar, e o begin() da proxima musica tenta instalar
//  de novo (e registra erro). Aqui o driver fica instalado e so e zerado.
class CountingI2S : public AudioOutputI2S
{
    public:
        CountingI2S() : AudioOutputI2S(0, EXTERNAL_I2S, 16 /* buffers DMA */) {}

        volatile uint32_t samples = 0;

        bool ConsumeSample(int16_t sample[2]) override
        {
            if (!AudioOutputI2S::ConsumeSample(sample)) {
                return false;   // DMA cheio: a amostra sera reenviada depois
            }
            samples++;
            return true;
        }

        bool stop() override
        {
            i2s_zero_dma_buffer((i2s_port_t)portNo);
            return true;
        }

        uint32_t rate() const { return hertz; }
};

// Memoria do decodificador e do buffer alocada uma vez (sem fragmentar a RAM).
static uint8_t mp3Memory[AudioGeneratorMP3::preAllocSize()];
static uint8_t bufferMemory[16 * 1024];

AudioEngine::AudioEngine(fs::FS &f, uint8_t bck, uint8_t lrck, uint8_t dout, uint8_t muteP)
    : fs(f), pinBck(bck), pinLrck(lrck), pinDout(dout), pinMute(muteP) {}

void AudioEngine::begin()
{
    static_assert(sizeof(bufferMemory) == BUFFER_BYTES, "tamanho do buffer");

    pinMode(pinMute, OUTPUT);
    mute(true);

    output = new CountingI2S();
    output->SetPinout(pinBck, pinLrck, pinDout);
    output->SetGain(0.0f);
    mp3 = new AudioGeneratorMP3(mp3Memory, sizeof(mp3Memory));

    lock = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(taskEntry, "audio", TASK_STACK, this, TASK_PRIORITY, &task, TASK_CORE);
}

// -----------------------------------------------------------------------------
//  Tarefa de decodificacao (nucleo 0)
// -----------------------------------------------------------------------------

void AudioEngine::taskEntry(void *self)
{
    static_cast<AudioEngine *>(self)->taskLoop();
}

void AudioEngine::taskLoop()
{
    for (;;) {
        bool busy = false;

        xSemaphoreTake(lock, portMAX_DELAY);
        if (state == State::Playing && mp3->isRunning()) {
            buffered->loop();              // completa o buffer com o que puder ler do SD
            if (!mp3->loop()) {            // fim do arquivo (ou erro de leitura)
                closeLocked();
                isFinished = true;
            }
            busy = true;
        }
        xSemaphoreGive(lock);

        // Libera o nucleo um pouco: os 16 buffers DMA guardam ~40 ms de som.
        vTaskDelay(busy ? 1 : pdMS_TO_TICKS(10));
    }
}

// -----------------------------------------------------------------------------
//  Comandos (chamados do loop principal)
// -----------------------------------------------------------------------------

bool AudioEngine::start(const std::string &path, const Mp3Info &info, uint32_t startSeconds)
{
    xSemaphoreTake(lock, portMAX_DELAY);
    closeLocked();
    isFinished = false;

    file = new AudioFileSourceFS(fs);
    bool ok = file->open(path.c_str());
    if (ok) {
        // Salta direto para o byte correspondente ao segundo pedido.
        ok = file->seek((int32_t)info.offsetForSecond(startSeconds), SEEK_SET);
    }
    if (ok) {
        buffered = new AudioFileSourceBuffer(file, bufferMemory, BUFFER_BYTES);
        output->samples = 0;
        startOffsetSeconds = startSeconds;
        ok = mp3->begin(buffered, output);
        decoderOpen = true;   // mesmo se falhar, stop() precisa limpar o que o begin() abriu
    }

    if (ok) {
        state = State::Playing;
        mute(false);
    } else {
        Serial.printf("Audio: nao foi possivel tocar %s\r\n", path.c_str());
        closeLocked();
    }
    xSemaphoreGive(lock);
    return ok;
}

void AudioEngine::pause()
{
    xSemaphoreTake(lock, portMAX_DELAY);
    if (state == State::Playing) {
        state = State::Paused;
        mute(true);
        i2s_zero_dma_buffer(I2S_NUM_0);   // nao repete o ultimo pedaco
    }
    xSemaphoreGive(lock);
}

void AudioEngine::resume()
{
    xSemaphoreTake(lock, portMAX_DELAY);
    if (state == State::Paused) {
        state = State::Playing;
        mute(false);
    }
    xSemaphoreGive(lock);
}

void AudioEngine::stop()
{
    xSemaphoreTake(lock, portMAX_DELAY);
    closeLocked();
    isFinished = false;
    xSemaphoreGive(lock);
}

void AudioEngine::closeLocked()
{
    if (decoderOpen) {
        mp3->stop();          // libera o libmad e zera o DMA (CountingI2S::stop)
        decoderOpen = false;
    }
    delete buffered;
    buffered = nullptr;
    if (file) {
        file->close();
        delete file;
        file = nullptr;
    }
    state = State::Idle;
    mute(true);
}

void AudioEngine::setVolume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;

    // Curva quadratica: o ouvido percebe volume de forma logaritmica.
    // Ganho maximo 1.0 (sem distorcer); o minimo audivel e 1/64.
    float v = volume / 100.0f;
    float gain = v * v;
    if (volume > 0 && gain < 1.0f / 64) gain = 1.0f / 64;

    if (!lock) {
        return;   // begin() ainda nao foi chamado
    }
    xSemaphoreTake(lock, portMAX_DELAY);
    output->SetGain(gain);
    xSemaphoreGive(lock);
}

uint32_t AudioEngine::position() const
{
    uint32_t rate = output ? output->rate() : 0;
    if (rate == 0) {
        return startOffsetSeconds;
    }
    return startOffsetSeconds + output->samples / rate;
}

void AudioEngine::mute(bool silent)
{
    digitalWrite(pinMute, silent ? LOW : HIGH);
}
