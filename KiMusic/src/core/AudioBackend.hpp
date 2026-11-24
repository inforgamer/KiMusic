// =============================================================================
//  core/AudioBackend.hpp  -  O que o Player precisa de um tocador de audio
// -----------------------------------------------------------------------------
//  Implementacoes:
//    - system/AudioEngine : decodifica MP3 do SD e envia ao DAC PCM5102A (I2S)
//    - testes no PC       : FakeAudio em test/host/core_test.cpp
//
//  Todos os metodos sao chamados do loop principal e devem retornar rapido;
//  a decodificacao em si acontece em outra tarefa (ver AudioEngine).
// =============================================================================
#ifndef CORE_AUDIOBACKEND_HPP
#define CORE_AUDIOBACKEND_HPP

#include <stdint.h>
#include <string>

#include "Mp3Info.hpp"

class AudioBackend
{
    public:
        virtual ~AudioBackend() {}

        // Comeca a tocar 'path' a partir de startSeconds (info da o byte certo).
        // Retorna false se o arquivo nao puder ser aberto.
        virtual bool start(const std::string &path, const Mp3Info &info, uint32_t startSeconds) = 0;

        virtual void pause() = 0;    // silencia e para de decodificar (mantem a posicao)
        virtual void resume() = 0;   // continua de onde parou
        virtual void stop() = 0;     // fecha o arquivo

        virtual void setVolume(int volume) = 0;   // 0..100

        // Segundos tocados desde o inicio da musica.
        virtual uint32_t position() const = 0;

        // true quando a musica terminou (ou deu erro de leitura) desde o ultimo start().
        virtual bool finished() const = 0;
};

#endif
