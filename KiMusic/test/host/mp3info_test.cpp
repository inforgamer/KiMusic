// Teste do leitor de MP3 no PC (fora da placa).
//
// Compilar e rodar todos os testes (na pasta KiMusic):
//   powershell -File test/host/run_tests.ps1

#include "test_util.hpp"

#include <string>

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

    std::printf(testFailures() ? "\n%d FALHA(S)\n" : "\nTodos os testes passaram.\n", testFailures());
    return testFailures() ? 1 : 0;
}
