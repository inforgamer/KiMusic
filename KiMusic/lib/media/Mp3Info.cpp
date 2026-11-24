// =============================================================================
//  Mp3Info.cpp  -  Implementacao do leitor de metadados (ver Mp3Info.hpp)
// -----------------------------------------------------------------------------
//  Ordem de leitura:
//    1. Tag ID3v2 no inicio do arquivo (texto, TLEN e capa APIC/PIC)
//    2. Tag ID3v1 nos ultimos 128 bytes (so preenche campos vazios)
//    3. Primeiro frame MPEG de audio -> duracao (TLEN > Xing/Info > VBRI > CBR)
// =============================================================================
#include "Mp3Info.hpp"

#include <stdlib.h>
#include <string.h>

namespace {

constexpr size_t MAX_TEXT_FRAME = 512;      // bytes lidos de cada frame de texto
constexpr uint32_t SYNC_SEARCH = 64 * 1024; // quanto procurar pelo primeiro frame MPEG

uint32_t syncsafe(const uint8_t *b)
{
    return ((uint32_t)(b[0] & 0x7F) << 21) | ((uint32_t)(b[1] & 0x7F) << 14) |
           ((uint32_t)(b[2] & 0x7F) << 7)  |  (uint32_t)(b[3] & 0x7F);
}

uint32_t be32(const uint8_t *b)
{
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
}

void appendUtf8(std::string &s, uint32_t cp)
{
    if (cp < 0x80) {
        s += (char)cp;
    } else if (cp < 0x800) {
        s += (char)(0xC0 | (cp >> 6));
        s += (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        s += (char)(0xE0 | (cp >> 12));
        s += (char)(0x80 | ((cp >> 6) & 0x3F));
        s += (char)(0x80 | (cp & 0x3F));
    } else {
        s += (char)(0xF0 | (cp >> 18));
        s += (char)(0x80 | ((cp >> 12) & 0x3F));
        s += (char)(0x80 | ((cp >> 6) & 0x3F));
        s += (char)(0x80 | (cp & 0x3F));
    }
}

void trim(std::string &s)
{
    while (!s.empty() && (s.back() == ' ' || s.back() == '\0')) {
        s.pop_back();
    }
}

std::string latin1ToUtf8(const uint8_t *b, size_t len)
{
    std::string s;
    for (size_t i = 0; i < len && b[i] != 0; i++) {
        appendUtf8(s, b[i]);
    }
    trim(s);
    return s;
}

std::string utf16ToUtf8(const uint8_t *b, size_t len, bool bigEndian)
{
    std::string s;
    for (size_t i = 0; i + 1 < len; i += 2) {
        uint32_t u = bigEndian ? ((b[i] << 8) | b[i + 1]) : ((b[i + 1] << 8) | b[i]);
        if (u == 0) {
            break;
        }
        if (u >= 0xD800 && u <= 0xDBFF && i + 3 < len) {   // par substituto
            uint32_t lo = bigEndian ? ((b[i + 2] << 8) | b[i + 3]) : ((b[i + 3] << 8) | b[i + 2]);
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00);
                i += 2;
            }
        }
        appendUtf8(s, u);
    }
    trim(s);
    return s;
}

// Frame de texto ID3v2: 1 byte de codificacao + texto.
std::string decodeText(const uint8_t *b, size_t len)
{
    if (len < 2) {
        return std::string();
    }
    uint8_t encoding = b[0];
    b++;
    len--;

    switch (encoding) {
        case 0:
            return latin1ToUtf8(b, len);
        case 1: {   // UTF-16 com BOM
            bool bigEndian = false;
            if (len >= 2 && b[0] == 0xFE && b[1] == 0xFF) { bigEndian = true; b += 2; len -= 2; }
            else if (len >= 2 && b[0] == 0xFF && b[1] == 0xFE) { b += 2; len -= 2; }
            return utf16ToUtf8(b, len, bigEndian);
        }
        case 2:
            return utf16ToUtf8(b, len, true);
        case 3: {
            std::string s((const char *)b, strnlen((const char *)b, len));
            trim(s);
            return s;
        }
        default:
            return std::string();
    }
}

// Frame de imagem:
//   v2.3/v2.4 APIC: codificacao, MIME\0, tipo, descricao\0, dados
//   v2.2 PIC:       codificacao, formato (3 letras), tipo, descricao\0, dados
// Prefere a capa frontal (tipo 3) quando ha mais de uma imagem.
void readPicture(ByteReader &r, uint32_t pos, uint32_t size, int version, Mp3Info &out, bool &haveFront)
{
    uint8_t head[MAX_TEXT_FRAME];
    size_t len = size < sizeof(head) ? size : sizeof(head);
    if (len < 4 || !r.readAt(pos, head, len)) {
        return;
    }

    uint8_t encoding = head[0];
    size_t i = 1;

    if (version == 2) {
        i += 3;
    } else {
        while (i < len && head[i] != 0) i++;   // MIME
        i++;
    }
    if (i >= len) return;

    uint8_t pictureType = head[i++];

    // Descricao: termina em 00 (Latin-1/UTF-8) ou 00 00 alinhado (UTF-16).
    bool wide = (encoding == 1 || encoding == 2);
    if (wide) {
        while (i + 1 < len && !(head[i] == 0 && head[i + 1] == 0)) i += 2;
        i += 2;
    } else {
        while (i < len && head[i] != 0) i++;
        i++;
    }
    if (i + 4 > len || i >= size) return;

    bool front = (pictureType == 3);
    if (out.coverFormat != CoverFormat::None && (haveFront || !front)) {
        return;   // ja temos uma imagem melhor ou igual
    }

    const uint8_t *data = head + i;
    if (data[0] == 0xFF && data[1] == 0xD8) {
        out.coverFormat = CoverFormat::Jpeg;
    } else if (data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G') {
        out.coverFormat = CoverFormat::Png;
    } else {
        out.coverFormat = CoverFormat::Other;
    }
    out.coverOffset = pos + (uint32_t)i;
    out.coverSize = size - (uint32_t)i;
    haveFront = front;
}

// Le a tag ID3v2. Retorna o offset onde o audio comeca (0 se nao houver tag).
uint32_t readId3v2(ByteReader &r, Mp3Info &out, uint32_t &tlenMs)
{
    uint8_t h[10];
    if (!r.readAt(0, h, 10) || memcmp(h, "ID3", 3) != 0) {
        return 0;
    }

    uint8_t version = h[3];
    uint8_t flags = h[5];
    uint32_t tagSize = syncsafe(h + 6);
    uint32_t end = 10 + tagSize;
    uint32_t audioStart = end + ((flags & 0x10) ? 10 : 0);   // rodape (v2.4)

    if (version < 2 || version > 4 || (flags & 0x80)) {      // 0x80 = unsynchronisation (raro)
        return audioStart;
    }

    uint32_t pos = 10;
    if (flags & 0x40) {   // cabecalho estendido
        uint8_t e[4];
        if (!r.readAt(pos, e, 4)) {
            return audioStart;
        }
        pos += (version == 4) ? syncsafe(e) : be32(e) + 4;
    }

    const size_t headerLen = (version == 2) ? 6 : 10;
    uint8_t fh[10];
    uint8_t data[MAX_TEXT_FRAME];
    bool haveFront = false;

    while (pos + headerLen <= end) {
        if (!r.readAt(pos, fh, headerLen) || fh[0] == 0) {
            break;   // padding
        }

        char id[5] = {0};
        uint32_t size;
        uint16_t frameFlags = 0;

        if (version == 2) {
            memcpy(id, fh, 3);
            size = ((uint32_t)fh[3] << 16) | ((uint32_t)fh[4] << 8) | fh[5];
        } else {
            memcpy(id, fh, 4);
            size = (version == 4) ? syncsafe(fh + 4) : be32(fh + 4);
            frameFlags = (fh[8] << 8) | fh[9];
        }

        uint32_t dataPos = pos + headerLen;
        pos = dataPos + size;
        if (size == 0 || pos > end) {
            break;
        }

        std::string *target = nullptr;
        bool isLength = false;
        if (!strcmp(id, "TIT2") || !strcmp(id, "TT2")) target = &out.title;
        else if (!strcmp(id, "TPE1") || !strcmp(id, "TP1")) target = &out.artist;
        else if (!strcmp(id, "TALB") || !strcmp(id, "TAL")) target = &out.album;
        else if (!strcmp(id, "TLEN") || !strcmp(id, "TLE")) isLength = true;
        bool isPicture = !strcmp(id, "APIC") || !strcmp(id, "PIC");

        if (!target && !isLength && !isPicture) {
            continue;
        }

        // Ignora frames comprimidos/criptografados.
        bool compressed = (version == 3) ? (frameFlags & 0x00C0) : (frameFlags & 0x000C);
        if (version != 2 && compressed) {
            continue;
        }
        if (version == 4 && (frameFlags & 0x0001)) {   // data length indicator
            dataPos += 4;
            size = (size > 4) ? size - 4 : 0;
        }

        if (isPicture) {
            readPicture(r, dataPos, size, version, out, haveFront);
            continue;
        }

        size_t len = size < MAX_TEXT_FRAME ? size : MAX_TEXT_FRAME;
        if (len == 0 || !r.readAt(dataPos, data, len)) {
            continue;
        }

        std::string text = decodeText(data, len);
        if (target) {
            *target = text;
        } else {
            tlenMs = (uint32_t)strtoul(text.c_str(), nullptr, 10);
        }
    }

    return audioStart;
}

bool readId3v1(ByteReader &r, Mp3Info &out)
{
    uint32_t size = r.size();
    if (size < 128) {
        return false;
    }
    uint8_t t[128];
    if (!r.readAt(size - 128, t, 128) || memcmp(t, "TAG", 3) != 0) {
        return false;
    }
    if (out.title.empty())  out.title  = latin1ToUtf8(t + 3, 30);
    if (out.artist.empty()) out.artist = latin1ToUtf8(t + 33, 30);
    if (out.album.empty())  out.album  = latin1ToUtf8(t + 63, 30);
    return true;
}

struct FrameHeader
{
    bool mpeg1;
    uint32_t bitrate;      // kbps
    uint32_t sampleRate;   // Hz
    uint32_t samplesPerFrame;
    uint32_t length;       // bytes
    bool mono;
};

// So MPEG Layer III (MP3).
bool parseHeader(const uint8_t *b, FrameHeader &f)
{
    static const uint16_t BITRATE_V1[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
    static const uint16_t BITRATE_V2[16] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
    static const uint32_t SAMPLE_RATE[3] = {44100, 48000, 32000};

    if (b[0] != 0xFF || (b[1] & 0xE0) != 0xE0) return false;

    uint8_t version = (b[1] >> 3) & 3;   // 3 = MPEG1, 2 = MPEG2, 0 = MPEG2.5
    uint8_t layer = (b[1] >> 1) & 3;     // 1 = Layer III
    uint8_t bitrateIndex = b[2] >> 4;
    uint8_t rateIndex = (b[2] >> 2) & 3;

    if (version == 1 || layer != 1 || bitrateIndex == 0 || bitrateIndex == 15 || rateIndex == 3) {
        return false;
    }

    f.mpeg1 = (version == 3);
    f.bitrate = f.mpeg1 ? BITRATE_V1[bitrateIndex] : BITRATE_V2[bitrateIndex];
    f.sampleRate = SAMPLE_RATE[rateIndex] >> (version == 3 ? 0 : (version == 2 ? 1 : 2));
    f.samplesPerFrame = f.mpeg1 ? 1152 : 576;
    f.mono = (b[3] >> 6) == 3;

    uint32_t padding = (b[2] >> 1) & 1;
    f.length = (f.samplesPerFrame / 8) * f.bitrate * 1000 / f.sampleRate + padding;
    return f.length > 4;
}

// Procura o primeiro frame valido (confirmado pelo frame seguinte) a partir de start.
bool findFirstFrame(ByteReader &r, uint32_t start, uint32_t &framePos, FrameHeader &f)
{
    uint32_t fileSize = r.size();
    uint32_t limit = start + SYNC_SEARCH;
    if (limit > fileSize) limit = fileSize;

    uint8_t buf[512];
    for (uint32_t base = start; base + 4 <= limit; base += sizeof(buf) - 3) {
        size_t len = sizeof(buf);
        if (base + len > fileSize) len = fileSize - base;
        if (len < 4 || !r.readAt(base, buf, len)) return false;

        for (size_t i = 0; i + 4 <= len; i++) {
            if (buf[i] != 0xFF || !parseHeader(buf + i, f)) continue;

            uint32_t pos = base + i;
            uint8_t next[4];
            uint32_t nextPos = pos + f.length;
            FrameHeader g;
            bool confirmed = nextPos + 4 > fileSize ||
                             (r.readAt(nextPos, next, 4) && parseHeader(next, g) && g.sampleRate == f.sampleRate);
            if (confirmed) {
                framePos = pos;
                return true;
            }
        }
    }
    return false;
}

} // namespace

bool readMp3Info(ByteReader &reader, Mp3Info &out)
{
    out = Mp3Info();

    uint32_t tlenMs = 0;
    uint32_t audioStart = readId3v2(reader, out, tlenMs);
    bool hasId3v1 = readId3v1(reader, out);

    uint32_t framePos;
    FrameHeader f;
    if (!findFirstFrame(reader, audioStart, framePos, f)) {
        out.duration = tlenMs / 1000;
        return false;
    }

    // Trecho de audio (usado para saltar no arquivo ao mudar a posicao).
    uint32_t audioEnd = reader.size() - (hasId3v1 ? 128 : 0);
    out.audioOffset = framePos;
    out.audioBytes = audioEnd > framePos ? audioEnd - framePos : 0;
    out.sampleRate = f.sampleRate;
    out.bitrate = f.bitrate;

    if (tlenMs > 0) {
        out.duration = (tlenMs + 500) / 1000;
        return true;
    }

    // Cabecalho Xing/Info (VBR) logo apos o side info.
    uint32_t sideInfo = f.mpeg1 ? (f.mono ? 17 : 32) : (f.mono ? 9 : 17);
    uint8_t x[12];
    if (reader.readAt(framePos + 4 + sideInfo, x, 12) &&
        (!memcmp(x, "Xing", 4) || !memcmp(x, "Info", 4)) && (be32(x + 4) & 1)) {
        uint32_t frames = be32(x + 8);
        out.duration = (uint32_t)(((uint64_t)frames * f.samplesPerFrame + f.sampleRate / 2) / f.sampleRate);
        return true;
    }

    // Cabecalho VBRI (Fraunhofer), sempre 32 bytes apos o cabecalho.
    uint8_t v[18];
    if (reader.readAt(framePos + 4 + 32, v, 18) && !memcmp(v, "VBRI", 4)) {
        uint32_t frames = be32(v + 14);
        out.duration = (uint32_t)(((uint64_t)frames * f.samplesPerFrame + f.sampleRate / 2) / f.sampleRate);
        return true;
    }

    // CBR: tamanho do audio / taxa de bits.
    out.duration = (uint32_t)(((uint64_t)out.audioBytes * 8 + f.bitrate * 500) / (f.bitrate * 1000));
    return true;
}
