// =============================================================================
//  UnsyncReader.hpp  -  Le um trecho "nao sincronizado" de uma tag ID3
// -----------------------------------------------------------------------------
//  Na "nao sincronizacao" (unsynchronisation) do ID3, quem grava a tag insere
//  um byte 00 depois de cada FF, para nenhum player confundir a tag com audio.
//  Este reader mostra o trecho [start, end) do arquivo ja sem esses 00 extras.
//
//  Pensado para leitura em sequencia (tags e decodificadores de imagem):
//  voltar para tras recomeca do inicio do trecho. O tamanho so e calculado
//  (lendo o trecho todo) se alguem pedir size().
// =============================================================================
#ifndef UNSYNCREADER_HPP
#define UNSYNCREADER_HPP

#include "ByteReader.hpp"

class UnsyncReader : public ByteReader
{
    public:
        UnsyncReader(ByteReader &source, uint32_t start, uint32_t end)
            : src(source), start(start), end(end)
        {
            rewind();
        }

        uint32_t size() override
        {
            if (!sized) {
                rewind();
                uint8_t b;
                uint32_t n = 0;
                while (next(b)) n++;
                logicalSize = n;
                sized = true;
                rewind();
            }
            return logicalSize;
        }

        bool readAt(uint32_t pos, uint8_t *buf, size_t len) override
        {
            if (pos < logical) {
                rewind();
            }
            uint8_t b;
            while (logical < pos) {
                if (!next(b)) return false;
            }
            for (size_t i = 0; i < len; i++) {
                if (!next(b)) return false;
                buf[i] = b;
            }
            return true;
        }

    private:
        ByteReader &src;
        uint32_t start, end;
        uint32_t phys = 0;           // proximo byte do arquivo
        uint32_t logical = 0;        // proximo byte ja sem os 00 extras
        bool prevFF = false;
        bool sized = false;
        uint32_t logicalSize = 0;
        uint8_t chunk[256];          // le o arquivo em blocos, nao byte a byte
        uint32_t chunkStart = 0;
        uint32_t chunkLen = 0;

        void rewind()
        {
            phys = start;
            logical = 0;
            prevFF = false;
        }

        bool next(uint8_t &b)
        {
            for (;;) {
                if (phys >= end) return false;
                if (phys < chunkStart || phys >= chunkStart + chunkLen) {
                    chunkStart = phys;
                    chunkLen = end - phys < sizeof(chunk) ? end - phys : (uint32_t)sizeof(chunk);
                    if (!src.readAt(phys, chunk, chunkLen)) {
                        chunkLen = 0;
                        return false;
                    }
                }
                uint8_t c = chunk[phys - chunkStart];
                phys++;
                if (prevFF && c == 0x00) {   // 00 inserido depois de FF: descarta
                    prevFF = false;
                    continue;
                }
                prevFF = (c == 0xFF);
                b = c;
                logical++;
                return true;
            }
        }
};

#endif
