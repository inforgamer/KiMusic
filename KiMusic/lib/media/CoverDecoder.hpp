// =============================================================================
//  CoverDecoder.hpp  -  Decodificador da capa do album
// -----------------------------------------------------------------------------
//  Transforma um JPEG (embutido no MP3 ou arquivo .jpg) em um quadrado de
//  COVER_SIZE x COVER_SIZE pixels RGB565, pronto para o LVGL.
//
//  - A imagem e lida aos poucos do ByteReader: so ~4 KB de RAM de trabalho.
//  - Usa a reducao do proprio decodificador (1/2, 1/4, 1/8) e depois ajusta
//    para COVER_SIZE com corte quadrado no centro.
//  - Usa o TJpgDec (lib/tjpgd). Nao suporta JPEG progressivo nem PNG.
//
//  Codigo puro C++ (sem Arduino): testado no PC em test/host/cover_test.cpp.
// =============================================================================
#ifndef COVERDECODER_HPP
#define COVERDECODER_HPP

#include <stdint.h>
#include <stddef.h>

#include "ByteReader.hpp"

namespace CoverDecoder
{
    // Lado da capa na tela (igual ao ui_AlbumCover do SquareLine).
    constexpr int COVER_SIZE = 140;
    constexpr size_t BUFFER_PIXELS = (size_t)COVER_SIZE * COVER_SIZE;

    // Decodifica o JPEG que esta em [offset, offset + size) do reader.
    //   size = 0     -> vai ate o fim do reader
    //   out          -> BUFFER_PIXELS pixels RGB565
    //   swapBytes    -> true para o LVGL com LV_COLOR_16_SWAP = 1
    // Retorna false se o JPEG for invalido ou nao suportado.
    bool decodeJpeg(ByteReader &reader, uint32_t offset, uint32_t size,
                    uint16_t *out, bool swapBytes);
}

#endif
