// =============================================================================
//  ByteReader.hpp  -  Fonte de bytes com acesso aleatorio
// -----------------------------------------------------------------------------
//  Interface minima usada pelos leitores de midia (Mp3Info, CoverDecoder).
//  Na placa ela e implementada sobre um fs::File do cartao SD (Library.cpp);
//  nos testes no PC, sobre um arquivo comum ou um vetor em memoria.
//
//  Por nao depender do Arduino, todo o codigo de lib/media pode ser compilado
//  e testado no PC (ver test/host).
// =============================================================================
#ifndef BYTEREADER_HPP
#define BYTEREADER_HPP

#include <stdint.h>
#include <stddef.h>

class ByteReader
{
    public:
        virtual ~ByteReader() {}

        // Tamanho total da fonte em bytes.
        virtual uint32_t size() = 0;

        // Copia len bytes a partir de pos para buf.
        // Retorna false se nao conseguir ler todos os bytes.
        virtual bool readAt(uint32_t pos, uint8_t *buf, size_t len) = 0;
};

#endif
