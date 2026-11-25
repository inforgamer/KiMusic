// =============================================================================
//  CoverDecoder.cpp  -  Implementacao (ver CoverDecoder.hpp)
// -----------------------------------------------------------------------------
//  Fluxo: le a assinatura -> abre com JPEGDEC ou PNGdec -> monta as tabelas
//  srcX/srcY (qual pixel da imagem decodificada vai para cada pixel da capa)
//  -> o decodificador entrega blocos/linhas e so os pixels usados sao copiados.
// =============================================================================
#include "CoverDecoder.hpp"

#include <new>
#include <string.h>

#include <JPEGDEC.h>
// As duas bibliotecas definem estas macros de jeitos diferentes; nao usamos nenhuma aqui.
#undef MOTOSHORT
#undef MOTOLONG
#undef INTELSHORT
#undef INTELLONG
#undef REGISTER_WIDTH
#include <PNGdec.h>

namespace {

using CoverDecoder::COVER_SIZE;

// Trecho do ByteReader visto pelos decodificadores como um "arquivo".
struct Source
{
    ByteReader *reader;
    uint32_t base;
    uint32_t size;
};

// Tabela de reamostragem: corte quadrado no centro + vizinho mais proximo.
struct Mapping
{
    uint16_t srcX[COVER_SIZE];
    uint16_t srcY[COVER_SIZE];
    uint16_t *out;

    void setup(uint32_t w, uint32_t h)
    {
        uint32_t side = w < h ? w : h;
        uint32_t offX = (w - side) / 2;
        uint32_t offY = (h - side) / 2;
        for (int i = 0; i < COVER_SIZE; i++) {
            uint32_t v = (uint32_t)i * side / COVER_SIZE;
            srcX[i] = (uint16_t)(offX + v);
            srcY[i] = (uint16_t)(offY + v);
        }
    }
};

// Primeiro/ultimo indice da capa cujo pixel de origem cai em [lo, hi].
bool rangeOf(const uint16_t *src, int lo, int hi, int &first, int &last)
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

// ---- Leitura (mesma forma de callback nas duas bibliotecas) ----

template <typename FILE_T>
int32_t readCallback(FILE_T *file, uint8_t *buf, int32_t len)
{
    Source *s = static_cast<Source *>(file->fHandle);
    int32_t left = (int32_t)s->size - file->iPos;
    if (len > left) len = left;
    if (len <= 0 || !s->reader->readAt(s->base + (uint32_t)file->iPos, buf, (size_t)len)) {
        return 0;
    }
    file->iPos += len;
    return len;
}

template <typename FILE_T>
int32_t seekCallback(FILE_T *file, int32_t position)
{
    file->iPos = position;
    return position;
}

// ---- JPEG ----

int jpegDraw(JPEGDRAW *d)
{
    Mapping *m = static_cast<Mapping *>(d->pUser);
    int x0, x1, y0, y1;
    if (!rangeOf(m->srcX, d->x, d->x + d->iWidthUsed - 1, x0, x1) ||
        !rangeOf(m->srcY, d->y, d->y + d->iHeight - 1, y0, y1)) {
        return 1;
    }
    for (int ty = y0; ty <= y1; ty++) {
        const uint16_t *row = d->pPixels + (size_t)(m->srcY[ty] - d->y) * d->iWidth;
        uint16_t *dst = m->out + (size_t)ty * COVER_SIZE;
        for (int tx = x0; tx <= x1; tx++) {
            dst[tx] = row[m->srcX[tx] - d->x];
        }
    }
    return 1;
}

bool decodeJpeg(Source &src, Mapping &map, bool swapBytes)
{
    JPEGDEC *jpeg = new (std::nothrow) JPEGDEC;
    if (!jpeg) {
        return false;
    }
    bool ok = false;
    if (jpeg->open(&src, (int)src.size, nullptr, readCallback<JPEGFILE>, seekCallback<JPEGFILE>, jpegDraw)) {
        int w = jpeg->getWidth();
        int h = jpeg->getHeight();
        int shortSide = w < h ? w : h;

        // Progressivo: a biblioteca so decodifica em 1/8. Baseline: maior reducao
        // (1/2, 1/4, 1/8) que ainda deixa o lado menor >= COVER_SIZE.
        int shift = 0;
        int options = 0;
        if (jpeg->getJPEGType() == JPEG_MODE_PROGRESSIVE) {
            // shortcut: progressivo sai quadriculado (1/8 ampliado sem suavizar), interpolar se incomodar.
            shift = 3;
        } else {
            while (shift < 3 && (shortSide >> (shift + 1)) >= COVER_SIZE) shift++;
            options = shift == 1 ? JPEG_SCALE_HALF : shift == 2 ? JPEG_SCALE_QUARTER :
                      shift == 3 ? JPEG_SCALE_EIGHTH : 0;
        }
        map.setup((uint32_t)(w + (1 << shift) - 1) >> shift, (uint32_t)(h + (1 << shift) - 1) >> shift);

        jpeg->setPixelType(swapBytes ? RGB565_BIG_ENDIAN : RGB565_LITTLE_ENDIAN);
        jpeg->setUserPointer(&map);
        ok = jpeg->decode(0, 0, options) == 1;
        jpeg->close();
    }
    delete jpeg;
    return ok;
}

// ---- PNG ----

struct PngJob
{
    Mapping *map;
    PNG *png;
    uint16_t *line;     // uma linha convertida para RGB565
    bool swapBytes;
};

Source *pngSource = nullptr;   // o PNGdec so abre "por nome": passa a origem por aqui

void *pngOpen(const char *, int32_t *size)
{
    *size = (int32_t)pngSource->size;
    return pngSource;
}

void pngClose(void *) {}

int pngDraw(PNGDRAW *d)
{
    PngJob *job = static_cast<PngJob *>(d->pUser);
    int y0, y1;
    if (!rangeOf(job->map->srcY, d->y, d->y, y0, y1)) {
        return 1;   // linha nao usada na capa: nem converte
    }
    job->png->getLineAsRGB565(d, job->line,
                              job->swapBytes ? PNG_RGB565_BIG_ENDIAN : PNG_RGB565_LITTLE_ENDIAN,
                              0x00000000 /* fundo preto para transparencia */);
    for (int ty = y0; ty <= y1; ty++) {
        uint16_t *dst = job->map->out + (size_t)ty * COVER_SIZE;
        for (int tx = 0; tx < COVER_SIZE; tx++) {
            dst[tx] = job->line[job->map->srcX[tx]];
        }
    }
    return 1;
}

bool decodePng(Source &src, Mapping &map, bool swapBytes)
{
    PNG *png = new (std::nothrow) PNG;
    if (!png) {
        return false;
    }
    bool ok = false;
    pngSource = &src;
    if (png->open("", pngOpen, pngClose, readCallback<PNGFILE>, seekCallback<PNGFILE>, pngDraw) == PNG_SUCCESS) {
        int w = png->getWidth();
        int h = png->getHeight();
        uint16_t *line = (w > 0 && w <= CoverDecoder::MAX_PNG_WIDTH) ? new (std::nothrow) uint16_t[w] : nullptr;
        if (line) {
            map.setup((uint32_t)w, (uint32_t)h);
            PngJob job{&map, png, line, swapBytes};
            ok = png->decode(&job, 0) == PNG_SUCCESS;
            delete[] line;
        }
        png->close();
    }
    pngSource = nullptr;
    delete png;
    return ok;
}

} // namespace

bool CoverDecoder::decode(ByteReader &reader, uint32_t offset, uint32_t size,
                          uint16_t *out, bool swapBytes)
{
    uint32_t fileSize = reader.size();
    if (offset >= fileSize) {
        return false;
    }
    if (size == 0 || offset + size > fileSize) {
        size = fileSize - offset;
    }

    uint8_t magic[4];
    if (size < 4 || !reader.readAt(offset, magic, 4)) {
        return false;
    }

    memset(out, 0, BUFFER_PIXELS * sizeof(uint16_t));
    Source src{&reader, offset, size};
    static Mapping map;   // ~600 bytes: fora da pilha
    map.out = out;

    if (magic[0] == 0xFF && magic[1] == 0xD8) {
        return decodeJpeg(src, map, swapBytes);
    }
    if (magic[0] == 0x89 && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G') {
        return decodePng(src, map, swapBytes);
    }
    return false;
}
