// Teste da capa (APIC + decodificacao JPEG comum/progressivo e PNG) no PC.
//
// Compilar e rodar todos os testes (na pasta KiMusic):
//   powershell -File test/host/run_tests.ps1

#include "test_util.hpp"
#include "CoverDecoder.hpp"
#include "UnsyncReader.hpp"

#include <cstdlib>
#include <string>

using CoverDecoder::COVER_SIZE;

static std::vector<uint16_t> pixels(CoverDecoder::BUFFER_PIXELS);

// Salva o RGB565 como BMP 24 bits para conferir a imagem.
static void writeBmp(const char *path, const std::vector<uint16_t> &px)
{
    FILE *f = std::fopen(path, "wb");
    if (!f) return;
    const int rowBytes = COVER_SIZE * 3;               // 420, ja multiplo de 4
    const uint32_t dataSize = rowBytes * COVER_SIZE;
    uint8_t h[54] = {'B', 'M'};
    auto le32 = [&](int at, uint32_t v) { for (int i = 0; i < 4; i++) h[at + i] = (uint8_t)(v >> (8 * i)); };
    le32(2, 54 + dataSize); le32(10, 54); le32(14, 40);
    le32(18, COVER_SIZE); le32(22, COVER_SIZE);
    h[26] = 1; h[28] = 24; le32(34, dataSize);
    std::fwrite(h, 1, 54, f);
    for (int y = COVER_SIZE - 1; y >= 0; y--) {
        for (int x = 0; x < COVER_SIZE; x++) {
            uint16_t c = px[(size_t)y * COVER_SIZE + x];
            uint8_t bgr[3] = {(uint8_t)((c & 0x1F) << 3), (uint8_t)(((c >> 5) & 0x3F) << 2), (uint8_t)((c >> 11) << 3)};
            std::fwrite(bgr, 1, 3, f);
        }
    }
    std::fclose(f);
}

static std::vector<uint8_t> apicBody(uint8_t enc, const char *mime, uint8_t type,
                                     const std::vector<uint8_t> &desc, const std::vector<uint8_t> &image)
{
    std::vector<uint8_t> b{enc};
    b.insert(b.end(), mime, mime + std::strlen(mime) + 1);
    b.push_back(type);
    b.insert(b.end(), desc.begin(), desc.end());
    b.insert(b.end(), image.begin(), image.end());
    return b;
}

static void testDecodeFile(const std::vector<uint8_t> &jpeg, const char *bmpPath)
{
    std::printf("Decodifica JPEG real (%u bytes)\n", (unsigned)jpeg.size());
    MemoryReader r;
    r.data = jpeg;
    CHECK_EQ(CoverDecoder::decode(r, 0, 0, pixels.data(), false), true);

    // A imagem nao pode ficar toda preta (buffer zerado).
    size_t nonBlack = 0;
    for (uint16_t p : pixels) nonBlack += (p != 0);
    CHECK_EQ(nonBlack > CoverDecoder::BUFFER_PIXELS / 2, true);
    writeBmp(bmpPath, pixels);
    std::printf("  imagem salva em %s\n", bmpPath);
}

static void testApicInMp3(const std::vector<uint8_t> &jpeg)
{
    std::printf("APIC (capa embutida) + decodificacao a partir do offset\n");
    std::vector<uint8_t> frames;
    addFrame(frames, 3, "TIT2", textBody(0, ascii("Com capa")));
    // Descricao em UTF-16 com BOM, terminada em 00 00.
    std::vector<uint8_t> desc = utf16le(u"Capa", true);
    desc.push_back(0); desc.push_back(0);
    addFrame(frames, 3, "APIC", apicBody(1, "image/jpeg", 3, desc, jpeg));

    MemoryReader r;
    r.data = id3v2(3, frames, 32);
    addAudioFrames(r.data, 20);

    Mp3Info info;
    CHECK_EQ(readMp3Info(r, info), true);
    CHECK_EQ(info.title, std::string("Com capa"));
    CHECK_EQ(info.coverFormat == CoverFormat::Jpeg, true);
    CHECK_EQ(info.coverSize, (uint32_t)jpeg.size());
    CHECK_EQ(std::memcmp(r.data.data() + info.coverOffset, jpeg.data(), jpeg.size()), 0);

    std::vector<uint16_t> fromMp3(CoverDecoder::BUFFER_PIXELS);
    CHECK_EQ(CoverDecoder::decode(r, info.coverOffset, info.coverSize, fromMp3.data(), false), true);
    CHECK_EQ(fromMp3 == pixels, true);   // igual a decodificar o arquivo JPEG sozinho
}

static void testPrefersFrontCover()
{
    std::printf("Varias imagens: prefere a capa frontal (tipo 3)\n");
    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 1, 2, 3, 4};
    std::vector<uint8_t> jpg = {0xFF, 0xD8, 0xFF, 0xE0, 9, 9, 9, 9};
    std::vector<uint8_t> frames;
    addFrame(frames, 4, "APIC", apicBody(0, "image/png", 0, {'x', 0}, png));    // "outra"
    addFrame(frames, 4, "APIC", apicBody(3, "image/jpeg", 3, {0}, jpg));        // frontal, UTF-8
    addFrame(frames, 4, "APIC", apicBody(0, "image/png", 4, {0}, png));         // contracapa
    MemoryReader r;
    r.data = id3v2(4, frames, 0);
    addAudioFrames(r.data, 5);

    Mp3Info info;
    readMp3Info(r, info);
    CHECK_EQ(info.coverFormat == CoverFormat::Jpeg, true);
    CHECK_EQ(info.coverSize, (uint32_t)jpg.size());
}

static void testPngOnly()
{
    std::printf("Somente PNG: detecta o formato\n");
    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    std::vector<uint8_t> frames;
    addFrame(frames, 3, "APIC", apicBody(0, "image/png", 3, {0}, png));
    MemoryReader r;
    r.data = id3v2(3, frames, 0);
    addAudioFrames(r.data, 5);

    Mp3Info info;
    readMp3Info(r, info);
    CHECK_EQ(info.coverFormat == CoverFormat::Png, true);
}

// Diferenca media por canal (0..255) entre duas capas RGB565 (sem troca de bytes).
static double meanDiff(const std::vector<uint16_t> &a, const std::vector<uint16_t> &b)
{
    double sum = 0;
    for (size_t i = 0; i < a.size(); i++) {
        int ar = (a[i] >> 11) << 3, ag = ((a[i] >> 5) & 0x3F) << 2, ab = (a[i] & 0x1F) << 3;
        int br = (b[i] >> 11) << 3, bg = ((b[i] >> 5) & 0x3F) << 2, bb = (b[i] & 0x1F) << 3;
        sum += std::abs(ar - br) + std::abs(ag - bg) + std::abs(ab - bb);
    }
    return sum / (3.0 * a.size());
}

static void testOtherFormats(const char *dir)
{
    struct Case { const char *file; const char *label; double maxDiff; };
    // O JPEG comum de referencia e reduzido pela metade antes da amostragem e o PNG nao,
    // entao os pixels escolhidos diferem um pouco (~12). O progressivo vem em 1/8 do
    // tamanho (so a 1a passada) e fica mais suave, por isso tolera mais.
    const Case cases[] = {
        {"cover_progressive.jpg", "JPEG progressivo", 30.0},
        {"cover.png",             "PNG RGB",          15.0},
        {"cover_rgba.png",        "PNG RGBA",         15.0},
    };
    for (const Case &c : cases) {
        std::string path = std::string(dir) + "/" + c.file;
        std::vector<uint8_t> data = readFile(path.c_str());
        std::printf("%s (%u bytes)\n", c.label, (unsigned)data.size());
        CHECK_EQ(data.empty(), false);
        MemoryReader r;
        r.data = data;
        std::vector<uint16_t> out(CoverDecoder::BUFFER_PIXELS);
        CHECK_EQ(CoverDecoder::decode(r, 0, 0, out.data(), false), true);
        double diff = meanDiff(out, pixels);   // comparado com o JPEG comum da mesma capa
        std::printf("  diferenca media para o JPEG comum: %.1f\n", diff);
        CHECK_EQ(diff < c.maxDiff, true);

        std::string bmp = "test/host/cover_" + std::string(c.file) + ".bmp";
        writeBmp(bmp.c_str(), out);

        // swapBytes = true entrega os mesmos pixels com os bytes trocados (formato do LVGL).
        std::vector<uint16_t> swapped(CoverDecoder::BUFFER_PIXELS);
        CHECK_EQ(CoverDecoder::decode(r, 0, 0, swapped.data(), true), true);
        bool same = true;
        for (size_t i = 0; i < out.size(); i++) {
            if (swapped[i] != (uint16_t)((out[i] << 8) | (out[i] >> 8))) { same = false; break; }
        }
        CHECK_EQ(same, true);
    }
}

static void testPngInMp3(const char *dir)
{
    std::printf("PNG embutido no MP3 (APIC)\n");
    std::vector<uint8_t> png = readFile((std::string(dir) + "/cover.png").c_str());
    std::vector<uint8_t> frames;
    addFrame(frames, 3, "APIC", apicBody(0, "image/png", 3, {0}, png));
    MemoryReader mp3;
    mp3.data = id3v2(3, frames, 0);
    addAudioFrames(mp3.data, 5);

    Mp3Info info;
    readMp3Info(mp3, info);
    CHECK_EQ(info.coverFormat == CoverFormat::Png, true);

    std::vector<uint16_t> fromMp3(CoverDecoder::BUFFER_PIXELS), direct(CoverDecoder::BUFFER_PIXELS);
    CHECK_EQ(CoverDecoder::decode(mp3, info.coverOffset, info.coverSize, fromMp3.data(), false), true);
    MemoryReader file;
    file.data = png;
    CHECK_EQ(CoverDecoder::decode(file, 0, 0, direct.data(), false), true);
    CHECK_EQ(fromMp3 == direct, true);
}

// Aplica a "nao sincronizacao" do ID3: um 00 depois de cada FF.
static std::vector<uint8_t> unsynchronise(const std::vector<uint8_t> &in)
{
    std::vector<uint8_t> out;
    for (uint8_t b : in) {
        out.push_back(b);
        if (b == 0xFF) out.push_back(0x00);
    }
    return out;
}

static void checkUnsyncCover(MemoryReader &mp3, const std::vector<uint8_t> &jpeg)
{
    Mp3Info info;
    CHECK_EQ(readMp3Info(mp3, info), true);
    CHECK_EQ(info.title, std::string("Sincronia"));
    CHECK_EQ(info.coverFormat == CoverFormat::Jpeg, true);
    CHECK_EQ(info.coverUnsync, true);
    CHECK_EQ(info.coverSize, (uint32_t)jpeg.size());   // tamanho sem os 00 extras

    UnsyncReader unsync(mp3, info.coverUnsyncStart, info.coverUnsyncEnd);
    std::vector<uint16_t> out(CoverDecoder::BUFFER_PIXELS);
    CHECK_EQ(CoverDecoder::decode(unsync, info.coverOffset, info.coverSize, out.data(), false), true);
    CHECK_EQ(out == pixels, true);                      // igual a decodificar o JPEG sozinho
}

static void testUnsynchronisedTags(const std::vector<uint8_t> &jpeg)
{
    std::printf("Tag ID3v2.3 inteira nao sincronizada (titulo + capa)\n");
    {
        std::vector<uint8_t> frames;
        addFrame(frames, 3, "TIT2", textBody(0, ascii("Sincronia")));
        addFrame(frames, 3, "APIC", apicBody(0, "image/jpeg", 3, {0}, jpeg));
        std::vector<uint8_t> body = unsynchronise(frames);
        MemoryReader mp3;
        mp3.data = {'I', 'D', '3', 3, 0, 0x80};          // flag 0x80 = nao sincronizada
        put32(mp3.data, (uint32_t)body.size(), true);
        mp3.data.insert(mp3.data.end(), body.begin(), body.end());
        addAudioFrames(mp3.data, 5);
        checkUnsyncCover(mp3, jpeg);
    }

    std::printf("Frame ID3v2.4 nao sincronizado (flag 0x0002 so na capa)\n");
    {
        std::vector<uint8_t> frames;
        addFrame(frames, 4, "TIT2", textBody(3, ascii("Sincronia")));
        std::vector<uint8_t> pic = unsynchronise(apicBody(0, "image/jpeg", 3, {0}, jpeg));
        frames.insert(frames.end(), {'A', 'P', 'I', 'C'});
        put32(frames, (uint32_t)pic.size(), true);       // v2.4: tamanho ja com os 00 extras
        frames.push_back(0x00); frames.push_back(0x02);  // flags do frame: nao sincronizado
        frames.insert(frames.end(), pic.begin(), pic.end());
        MemoryReader mp3;
        mp3.data = id3v2(4, frames, 0);
        addAudioFrames(mp3.data, 5);
        checkUnsyncCover(mp3, jpeg);
    }
}

static void testInvalidJpeg()
{
    std::printf("Imagem invalida e recusada\n");
    MemoryReader r;
    r.data = ascii("nao e uma imagem jpeg de verdade................");
    CHECK_EQ(CoverDecoder::decode(r, 0, 0, pixels.data(), false), false);
}

int main(int argc, char **argv)
{
    const char *jpegPath = argc > 1 ? argv[1] : "SD/cover/music.jpg";
    const char *bmpPath = argc > 2 ? argv[2] : "test/host/cover_out.bmp";

    std::vector<uint8_t> jpeg = readFile(jpegPath);
    if (jpeg.empty()) {
        std::printf("Nao achei %s\n", jpegPath);
        return 1;
    }

    testDecodeFile(jpeg, bmpPath);
    testApicInMp3(jpeg);
    testPrefersFrontCover();
    testPngOnly();
    testOtherFormats("test/host/fixtures");
    testPngInMp3("test/host/fixtures");
    testUnsynchronisedTags(jpeg);
    testInvalidJpeg();

    std::printf(testFailures() ? "\n%d FALHA(S)\n" : "\nTodos os testes passaram.\n", testFailures());
    return testFailures() ? 1 : 0;
}
