// =============================================================================
//  Mp3Info.hpp  -  Metadados de um arquivo MP3
// -----------------------------------------------------------------------------
//  Le, sem carregar o arquivo inteiro na memoria:
//    - titulo, artista e album  (tags ID3 v2.2 / v2.3 / v2.4 e ID3 v1)
//    - duracao em segundos      (TLEN, cabecalho Xing/Info, VBRI ou calculo CBR)
//    - posicao da capa embutida (frame APIC / PIC)
//
//  Codigo puro C++ (sem Arduino): testado no PC em test/host/mp3info_test.cpp.
// =============================================================================
#ifndef MP3INFO_HPP
#define MP3INFO_HPP

#include <stdint.h>
#include <stddef.h>
#include <string>

#include "ByteReader.hpp"

// Formato da imagem de capa encontrada dentro do MP3.
enum class CoverFormat : uint8_t
{
    None,    // sem capa embutida
    Jpeg,    // suportado pelo CoverDecoder
    Png,     // detectado, mas ainda nao decodificado
    Other
};

struct Mp3Info
{
    std::string title;      // UTF-8
    std::string artist;     // UTF-8
    std::string album;      // UTF-8
    uint32_t duration = 0;  // segundos (0 = desconhecida)

    // Capa embutida. So a posicao e guardada: a imagem continua no arquivo
    // e e lida direto do SD na hora de desenhar (ver CoverDecoder).
    CoverFormat coverFormat = CoverFormat::None;
    uint32_t coverOffset = 0;   // byte do arquivo onde a imagem comeca
    uint32_t coverSize = 0;     // tamanho da imagem em bytes
};

// Preenche out com os dados do MP3.
// Retorna false se nao encontrar nenhum frame MPEG valido (arquivo invalido);
// mesmo assim, as tags encontradas ficam em out.
bool readMp3Info(ByteReader &reader, Mp3Info &out);

#endif
