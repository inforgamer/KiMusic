// Teste do leitor de MP3 no PC (fora da placa).
//
// Compilar e rodar (na pasta KiMusic):
//   g++ -std=c++17 -Wall -Wextra -I src test/host/mp3info_test.cpp src/Mp3Info.cpp -o test/host/mp3info_test.exe
//   test/host/mp3info_test.exe SD/music/music.mp3

#include "Mp3Info.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int failures = 0;

#define CHECK_EQ(a, b) do { \
    auto _a = (a); auto _b = (b); \
    if (!(_a == _b)) { \
        failures++; \
        std::printf("  FALHOU %s:%d: %s\n", __FILE__, __LINE__, #a " == " #b); \
    } \
} while (0)

class MemoryReader : public ByteReader
{
    public:
        std::vector<uint8_t> data;

        uint32_t size() override { return (uint32_t)data.size(); }

        bool readAt(uint32_t pos, uint8_t *buf, size_t len) override
        {
            if ((size_t)pos + len > data.size()) return false;
            std::memcpy(buf, data.data() + pos, len);
            return true;
        }
};

class FileReader : public ByteReader
{
    private:
        FILE *f;
        uint32_t len;

    public:
        explicit FileReader(FILE *file) : f(file)
        {
            std::fseek(f, 0, SEEK_END);
            len = (uint32_t)std::ftell(f);
        }

        uint32_t size() override { return len; }

        bool readAt(uint32_t pos, uint8_t *buf, size_t n) override
        {
            return std::fseek(f, pos, SEEK_SET) == 0 && std::fread(buf, 1, n, f) == n;
        }
};

// ---------- geradores de arquivos sinteticos ----------

static void put32(std::vector<uint8_t> &v, uint32_t x, bool syncsafe)
{
    if (syncsafe) x = ((x & 0xFE00000) << 3) | ((x & 0x1FC000) << 2) | ((x & 0x3F80) << 1) | (x & 0x7F);
    v.push_back(x >> 24); v.push_back(x >> 16); v.push_back(x >> 8); v.push_back(x);
}

static void addFrame(std::vector<uint8_t> &tag, int version, const char *id, const std::vector<uint8_t> &body)
{
    tag.insert(tag.end(), id, id + 4);
    put32(tag, (uint32_t)body.size(), version == 4);
    tag.push_back(0); tag.push_back(0);
    tag.insert(tag.end(), body.begin(), body.end());
}

static std::vector<uint8_t> textBody(uint8_t enc, const std::vector<uint8_t> &bytes)
{
    std::vector<uint8_t> b{enc};
    b.insert(b.end(), bytes.begin(), bytes.end());
    return b;
}

static std::vector<uint8_t> ascii(const char *s) { return std::vector<uint8_t>(s, s + std::strlen(s)); }

static std::vector<uint8_t> utf16le(const char16_t *s, bool bom)
{
    std::vector<uint8_t> v;
    if (bom) { v.push_back(0xFF); v.push_back(0xFE); }
    for (; *s; s++) { v.push_back(*s & 0xFF); v.push_back(*s >> 8); }
    return v;
}

static std::vector<uint8_t> id3v2(int version, const std::vector<uint8_t> &frames, size_t padding)
{
    std::vector<uint8_t> v{'I', 'D', '3', (uint8_t)version, 0, 0};
    put32(v, (uint32_t)(frames.size() + padding), true);
    v.insert(v.end(), frames.begin(), frames.end());
    v.insert(v.end(), padding, 0);
    return v;
}

// MPEG1 Layer III, 128 kbps, 44.1 kHz, estereo: 417 bytes por frame.
static void addAudioFrames(std::vector<uint8_t> &v, int frames)
{
    for (int i = 0; i < frames; i++) {
        size_t start = v.size();
        v.push_back(0xFF); v.push_back(0xFB); v.push_back(0x90); v.push_back(0x00);
        v.resize(start + 417, 0x55);
    }
}

// ---------- testes ----------

static void testRealFile(const char *path)
{
    std::printf("Arquivo real: %s\n", path);
    FILE *f = std::fopen(path, "rb");
    if (!f) { std::printf("  (nao encontrado, pulando)\n"); return; }
    FileReader r(f);
    Mp3Info info;
    bool ok = readMp3Info(r, info);
    std::fclose(f);

    std::printf("  titulo:  %s\n  artista: %s\n  album:   %s\n  duracao: %u s (%u:%02u)\n",
                info.title.c_str(), info.artist.c_str(), info.album.c_str(),
                (unsigned)info.duration, (unsigned)info.duration / 60, (unsigned)info.duration % 60);
    CHECK_EQ(ok, true);
    if (std::strstr(path, "music.mp3")) {
        CHECK_EQ(info.title, std::string("Full Moon Full Life"));
        CHECK_EQ(info.artist, std::string("Azumi Takahashi"));
        CHECK_EQ(info.album, std::string("Persona 3 Reload Original Soundtrack"));
        CHECK_EQ(info.duration, 294u);   // 293,52 s calculados em Python
    }
}

static void testId3v23Utf16AndCbr()
{
    std::printf("ID3v2.3 UTF-16 + CBR\n");
    std::vector<uint8_t> frames;
    addFrame(frames, 3, "TIT2", textBody(1, utf16le(u"Canção", true)));
    addFrame(frames, 3, "TPE1", textBody(0, {'J', 'o', 0xE3, 'o'}));   // Latin-1 "João"
    addFrame(frames, 3, "TALB", textBody(3, ascii("Álbum UTF-8")));
    MemoryReader r;
    r.data = id3v2(3, frames, 64);
    addAudioFrames(r.data, 100);

    Mp3Info info;
    CHECK_EQ(readMp3Info(r, info), true);
    CHECK_EQ(info.title, std::string("Canção"));
    CHECK_EQ(info.artist, std::string("João"));
    CHECK_EQ(info.album, std::string("Álbum UTF-8"));
    CHECK_EQ(info.duration, 3u);   // 100 * 417 * 8 / 128000 = 2,6 s, arredonda para 3
}

static void testXing()
{
    std::printf("Xing VBR\n");
    MemoryReader r;
    addAudioFrames(r.data, 3);
    // Xing apos 4 (cabecalho) + 32 (side info) bytes do primeiro frame.
    const uint8_t xing[] = {'X', 'i', 'n', 'g', 0, 0, 0, 1, 0, 0, 0x2B, 0x11};   // 11025 frames
    std::memcpy(r.data.data() + 36, xing, sizeof(xing));

    Mp3Info info;
    CHECK_EQ(readMp3Info(r, info), true);
    CHECK_EQ(info.duration, 288u);   // 11025 * 1152 / 44100 = 288
}

static void testTlenAndV24()
{
    std::printf("ID3v2.4 + TLEN\n");
    std::vector<uint8_t> frames;
    addFrame(frames, 4, "TIT2", textBody(3, ascii("Faixa")));
    addFrame(frames, 4, "TLEN", textBody(0, ascii("185400")));
    MemoryReader r;
    r.data = id3v2(4, frames, 0);
    addAudioFrames(r.data, 5);

    Mp3Info info;
    CHECK_EQ(readMp3Info(r, info), true);
    CHECK_EQ(info.title, std::string("Faixa"));
    CHECK_EQ(info.duration, 185u);
}

static void testId3v1()
{
    std::printf("ID3v1\n");
    MemoryReader r;
    addAudioFrames(r.data, 10);
    std::vector<uint8_t> tag(128, 0);
    std::memcpy(tag.data(), "TAG", 3);
    std::memcpy(tag.data() + 3, "Velha Tag   ", 12);
    std::memcpy(tag.data() + 33, "Artista", 7);
    std::memcpy(tag.data() + 63, "Disco", 5);
    r.data.insert(r.data.end(), tag.begin(), tag.end());

    Mp3Info info;
    CHECK_EQ(readMp3Info(r, info), true);
    CHECK_EQ(info.title, std::string("Velha Tag"));
    CHECK_EQ(info.artist, std::string("Artista"));
    CHECK_EQ(info.album, std::string("Disco"));
}

static void testInvalid()
{
    std::printf("Arquivo sem audio\n");
    MemoryReader r;
    r.data = ascii("isto nao e um mp3, so texto qualquer....");
    Mp3Info info;
    CHECK_EQ(readMp3Info(r, info), false);
    CHECK_EQ(info.duration, 0u);
}

int main(int argc, char **argv)
{
    testId3v23Utf16AndCbr();
    testXing();
    testTlenAndV24();
    testId3v1();
    testInvalid();
    testRealFile(argc > 1 ? argv[1] : "SD/music/music.mp3");

    std::printf(failures ? "\n%d FALHA(S)\n" : "\nTodos os testes passaram.\n", failures);
    return failures ? 1 : 0;
}
