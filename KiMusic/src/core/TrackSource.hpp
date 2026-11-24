// =============================================================================
//  core/TrackSource.hpp  -  De onde vem as musicas
// -----------------------------------------------------------------------------
//  O Player e o Menu so conhecem esta interface. Implementacoes:
//    - storage/Library     : MP3 da pasta /music do cartao SD
//    - storage/DemoSource  : uma musica de exemplo (sem SD, ex.: no Wokwi)
//    - testes no PC        : fontes falsas em memoria (test/host/core_test.cpp)
// =============================================================================
#ifndef CORE_TRACKSOURCE_HPP
#define CORE_TRACKSOURCE_HPP

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "Mp3Info.hpp"

class TrackSource
{
    public:
        virtual ~TrackSource() {}

        // Quantidade de musicas (o Player exige pelo menos 1).
        virtual size_t count() const = 0;

        // Nome curto para listas (ex.: nome do arquivo sem extensao).
        virtual std::string name(size_t index) const = 0;

        // Caminho do arquivo de audio. Vazio = musica sem arquivo (demo):
        // o Player simula o tempo em vez de tocar.
        virtual std::string path(size_t index) const { (void)index; return std::string(); }

        // Preenche titulo/artista/album/duracao. Campos que faltarem devem vir
        // preenchidos com um valor padrao. Retorna false se o arquivo for invalido.
        virtual bool load(size_t index, Mp3Info &info) const = 0;

        // Decodifica a capa em out (CoverDecoder::BUFFER_PIXELS pixels RGB565).
        // Retorna false se a musica nao tiver capa.
        virtual bool loadCover(size_t index, const Mp3Info &info, uint16_t *out) const = 0;
};

#endif
