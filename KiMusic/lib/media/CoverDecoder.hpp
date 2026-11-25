// =============================================================================
//  CoverDecoder.hpp  -  Decodificador da capa do album
// -----------------------------------------------------------------------------
//  Transforma uma imagem (embutida no MP3 ou arquivo de capa) em um quadrado de
//  COVER_SIZE x COVER_SIZE pixels RGB565, pronto para o LVGL.
//
//  Formatos (detectados pela assinatura do arquivo):
//    - JPEG comum (baseline) e progressivo   -> biblioteca JPEGDEC
//      (progressivo: so a 1a passada, em 1/8 do tamanho, depois ampliada)
//    - PNG (8 bits por canal, nao entrelacado, ate MAX_PNG_WIDTH) -> PNGdec
//
//  A imagem e lida aos poucos do ByteReader; os decodificadores sao alocados
//  so durante a decodificacao (JPEG ~20 KB, PNG ~55 KB) e liberados no fim.
//  Depois a imagem e cortada em quadrado no centro e ajustada para COVER_SIZE.
//
//  Testado no PC em test/host/cover_test.cpp.
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

    // Largura maxima de PNG (limite do buffer de linhas do PNGdec, ver platformio.ini).
    constexpr int MAX_PNG_WIDTH = 1500;

    // Decodifica a imagem (JPEG ou PNG) que esta em [offset, offset + size) do reader.
    //   size = 0     -> vai ate o fim do reader
    //   out          -> BUFFER_PIXELS pixels RGB565
    //   swapBytes    -> true para o LVGL com LV_COLOR_16_SWAP = 1
    // Retorna false se a imagem for invalida, de formato nao suportado ou se faltar memoria.
    bool decode(ByteReader &reader, uint32_t offset, uint32_t size,
                uint16_t *out, bool swapBytes);
}

#endif
