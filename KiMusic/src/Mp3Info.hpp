#ifndef MP3INFO_HPP
#define MP3INFO_HPP

#include <stdint.h>
#include <stddef.h>
#include <string>

// Fonte de bytes independente do Arduino (permite testar o leitor fora da placa).
class ByteReader
{
    public:
        virtual ~ByteReader() {}
        virtual uint32_t size() = 0;
        virtual bool readAt(uint32_t pos, uint8_t *buf, size_t len) = 0;
};

struct Mp3Info
{
    std::string title;      // UTF-8
    std::string artist;     // UTF-8
    std::string album;      // UTF-8
    uint32_t duration = 0;  // segundos (0 = desconhecida)
};

// Le as tags ID3 (v2.2/v2.3/v2.4 e v1) e calcula a duracao do MP3.
// Retorna false se nao encontrar nenhum frame MPEG valido.
bool readMp3Info(ByteReader &reader, Mp3Info &out);

#endif
