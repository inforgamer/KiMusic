// Utilitarios compartilhados pelos testes no PC.
#ifndef TEST_UTIL_HPP
#define TEST_UTIL_HPP

#include "Mp3Info.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

inline int &testFailures()
{
    static int failures = 0;
    return failures;
}

#define CHECK_EQ(a, b) do { \
    auto _a = (a); auto _b = (b); \
    if (!(_a == _b)) { \
        testFailures()++; \
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

inline std::vector<uint8_t> readFile(const char *path)
{
    std::vector<uint8_t> v;
    FILE *f = std::fopen(path, "rb");
    if (!f) return v;
    std::fseek(f, 0, SEEK_END);
    v.resize((size_t)std::ftell(f));
    std::fseek(f, 0, SEEK_SET);
    size_t got = std::fread(v.data(), 1, v.size(), f);
    v.resize(got);
    std::fclose(f);
    return v;
}

// ---------- geradores de MP3 sinteticos ----------

inline void put32(std::vector<uint8_t> &v, uint32_t x, bool syncsafe)
{
    if (syncsafe) x = ((x & 0xFE00000) << 3) | ((x & 0x1FC000) << 2) | ((x & 0x3F80) << 1) | (x & 0x7F);
    v.push_back(x >> 24); v.push_back(x >> 16); v.push_back(x >> 8); v.push_back(x);
}

inline void addFrame(std::vector<uint8_t> &tag, int version, const char *id, const std::vector<uint8_t> &body)
{
    tag.insert(tag.end(), id, id + 4);
    put32(tag, (uint32_t)body.size(), version == 4);
    tag.push_back(0); tag.push_back(0);
    tag.insert(tag.end(), body.begin(), body.end());
}

inline std::vector<uint8_t> textBody(uint8_t enc, const std::vector<uint8_t> &bytes)
{
    std::vector<uint8_t> b{enc};
    b.insert(b.end(), bytes.begin(), bytes.end());
    return b;
}

inline std::vector<uint8_t> ascii(const char *s) { return std::vector<uint8_t>(s, s + std::strlen(s)); }

inline std::vector<uint8_t> utf16le(const char16_t *s, bool bom)
{
    std::vector<uint8_t> v;
    if (bom) { v.push_back(0xFF); v.push_back(0xFE); }
    for (; *s; s++) { v.push_back(*s & 0xFF); v.push_back(*s >> 8); }
    return v;
}

inline std::vector<uint8_t> id3v2(int version, const std::vector<uint8_t> &frames, size_t padding)
{
    std::vector<uint8_t> v{'I', 'D', '3', (uint8_t)version, 0, 0};
    put32(v, (uint32_t)(frames.size() + padding), true);
    v.insert(v.end(), frames.begin(), frames.end());
    v.insert(v.end(), padding, 0);
    return v;
}

// MPEG1 Layer III, 128 kbps, 44.1 kHz, estereo: 417 bytes por frame.
inline void addAudioFrames(std::vector<uint8_t> &v, int frames)
{
    for (int i = 0; i < frames; i++) {
        size_t start = v.size();
        v.push_back(0xFF); v.push_back(0xFB); v.push_back(0x90); v.push_back(0x00);
        v.resize(start + 417, 0x55);
    }
}

#endif
