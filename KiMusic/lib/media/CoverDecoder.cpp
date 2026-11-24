// =============================================================================
//  CoverDecoder.cpp  -  Implementacao (ver CoverDecoder.hpp)
// -----------------------------------------------------------------------------
//  Fluxo: jd_prepare (le o cabecalho) -> escolhe a reducao -> monta as tabelas
//  srcX/srcY (qual pixel da imagem reduzida vai para cada pixel da capa) ->
//  jd_decomp chama output() bloco a bloco e so os pixels usados sao copiados.
// =============================================================================
#include "CoverDecoder.hpp"

#include <string.h>

extern "C" {
    #include "tjpgd.h"
}

namespace {

using CoverDecoder::COVER_SIZE;

constexpr size_t WORK_POOL = 4096;   // memoria de trabalho do tjpgd
uint8_t workPool[WORK_POOL];

struct Session
{
    ByteReader *reader;
    uint32_t pos;
    uint32_t end;
    uint16_t *out;
    bool swapBytes;
    uint16_t srcX[COVER_SIZE];   // coluna da imagem reduzida usada por cada coluna da capa
    uint16_t srcY[COVER_SIZE];
};

size_t input(JDEC *jd, uint8_t *buf, size_t len)
{
    Session *s = static_cast<Session *>(jd->device);
    uint32_t left = s->end - s->pos;
    if (len > left) {
        len = left;
    }
    if (buf && len > 0 && !s->reader->readAt(s->pos, buf, len)) {
        return 0;
    }
    s->pos += (uint32_t)len;   // buf == NULL: so pula os bytes
    return len;
}

// Primeiro/ultimo indice da capa cujo pixel de origem cai em [lo, hi].
bool rangeOf(const uint16_t *src, uint16_t lo, uint16_t hi, int &first, int &last)
{
    first = -1;
    for (int i = 0; i < COVER_SIZE; i++) {
        if (src[i] >= lo && src[i] <= hi) {
            if (first < 0) first = i;
            last = i;
        } else if (first >= 0) {
            break;   // src e crescente
        }
    }
    return first >= 0;
}

int output(JDEC *jd, void *bitmap, JRECT *rect)
{
    Session *s = static_cast<Session *>(jd->device);
    const uint8_t *rgb = static_cast<const uint8_t *>(bitmap);   // JD_FORMAT 0: RGB888
    const int rectWidth = rect->right - rect->left + 1;

    int x0, x1, y0, y1;
    if (!rangeOf(s->srcX, rect->left, rect->right, x0, x1) ||
        !rangeOf(s->srcY, rect->top, rect->bottom, y0, y1)) {
        return 1;
    }

    for (int ty = y0; ty <= y1; ty++) {
        const uint8_t *row = rgb + (size_t)(s->srcY[ty] - rect->top) * rectWidth * 3;
        uint16_t *dst = s->out + (size_t)ty * COVER_SIZE;
        for (int tx = x0; tx <= x1; tx++) {
            const uint8_t *p = row + (size_t)(s->srcX[tx] - rect->left) * 3;
            uint16_t c = (uint16_t)(((p[0] & 0xF8) << 8) | ((p[1] & 0xFC) << 3) | (p[2] >> 3));
            dst[tx] = s->swapBytes ? (uint16_t)((c << 8) | (c >> 8)) : c;
        }
    }
    return 1;
}

} // namespace

bool CoverDecoder::decodeJpeg(ByteReader &reader, uint32_t offset, uint32_t size,
                              uint16_t *out, bool swapBytes)
{
    uint32_t fileSize = reader.size();
    if (offset >= fileSize) {
        return false;
    }
    if (size == 0 || offset + size > fileSize) {
        size = fileSize - offset;
    }

    static Session s;   // ~600 bytes: fora da pilha
    s.reader = &reader;
    s.pos = offset;
    s.end = offset + size;
    s.out = out;
    s.swapBytes = swapBytes;

    memset(out, 0, CoverDecoder::BUFFER_PIXELS * sizeof(uint16_t));

    JDEC jd;
    if (jd_prepare(&jd, input, workPool, WORK_POOL, &s) != JDR_OK) {
        return false;
    }

    // Maior reducao (1/2, 1/4, 1/8) que ainda deixa o lado menor >= COVER_SIZE.
    uint8_t scale = 0;
    uint16_t shortSide = jd.width < jd.height ? jd.width : jd.height;
    while (scale < 3 && (shortSide >> (scale + 1)) >= COVER_SIZE) {
        scale++;
    }

    uint32_t w = (jd.width + (1u << scale) - 1) >> scale;
    uint32_t h = (jd.height + (1u << scale) - 1) >> scale;
    uint32_t side = w < h ? w : h;           // corte quadrado no centro
    uint32_t offX = (w - side) / 2;
    uint32_t offY = (h - side) / 2;

    for (int i = 0; i < COVER_SIZE; i++) {
        uint32_t v = (uint32_t)i * side / COVER_SIZE;
        s.srcX[i] = (uint16_t)(offX + v);
        s.srcY[i] = (uint16_t)(offY + v);
    }

    return jd_decomp(&jd, output, scale) == JDR_OK;
}
