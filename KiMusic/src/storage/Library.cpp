// =============================================================================
//  storage/Library.cpp  -  Implementacao (ver Library.hpp)
// =============================================================================
#include "storage/Library.hpp"
#include "storage/FileReader.hpp"
#include "CoverDecoder.hpp"

#include <algorithm>

namespace {

bool isMp3(const String &fileName)
{
    String lower = fileName;
    lower.toLowerCase();
    return lower.endsWith(".mp3") && !lower.startsWith(".");   // ignora "._arquivo" do macOS
}

// "/music/Faixa 01.mp3" -> "Faixa 01"
String baseName(const String &path)
{
    String fileName = path.substring(path.lastIndexOf('/') + 1);
    int dot = fileName.lastIndexOf('.');
    return dot > 0 ? fileName.substring(0, dot) : fileName;
}

} // namespace

Library::Library(fs::FS &f, const char *music, const char *cover)
    : fs(f), musicFolder(music), coverFolder(cover) {}

// -----------------------------------------------------------------------------
//  Lista de musicas
// -----------------------------------------------------------------------------

size_t Library::scan()
{
    paths.clear();

    File dir = fs.open(musicFolder);
    if (!dir || !dir.isDirectory()) {
        return 0;
    }

    String base = musicFolder;
    if (!base.endsWith("/")) {
        base += "/";
    }

    File entry = dir.openNextFile();
    while (entry && paths.size() < MAX_TRACKS) {
        if (!entry.isDirectory()) {
            String fileName = entry.name();
            fileName = fileName.substring(fileName.lastIndexOf('/') + 1);
            if (isMp3(fileName)) {
                paths.push_back(base + fileName);
            }
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();

    std::sort(paths.begin(), paths.end(), [](const String &a, const String &b) {
        return a.compareTo(b) < 0;
    });
    return paths.size();
}

std::string Library::name(size_t index) const
{
    return index < paths.size() ? std::string(baseName(paths[index]).c_str()) : std::string();
}

// -----------------------------------------------------------------------------
//  Dados de uma musica
// -----------------------------------------------------------------------------

bool Library::load(size_t index, Mp3Info &info) const
{
    info = Mp3Info();
    if (index >= paths.size()) {
        return false;
    }

    bool ok = false;
    File file = fs.open(paths[index]);
    if (file) {
        FileReader reader(file);
        ok = readMp3Info(reader, info);
        file.close();
    }

    // Valores padrao para o que nao veio nas tags.
    if (info.title.empty())  info.title  = name(index);
    if (info.artist.empty()) info.artist = "Artista desconhecido";
    if (info.album.empty())  info.album  = "Álbum desconhecido";
    return ok;
}

// -----------------------------------------------------------------------------
//  Capa
// -----------------------------------------------------------------------------

bool Library::loadCover(size_t index, const Mp3Info &info, uint16_t *out) const
{
    if (index >= paths.size()) {
        return false;
    }

    // 1) JPEG embutido no proprio MP3.
    if (info.coverFormat == CoverFormat::Jpeg &&
        decodeCoverFile(paths[index], info.coverOffset, info.coverSize, out)) {
        return true;
    }

    // 2) Arquivo com o mesmo nome da musica na pasta de capas.
    String coverPath = coverFolder + "/" + baseName(paths[index]) + ".jpg";
    return fs.exists(coverPath) && decodeCoverFile(coverPath, 0, 0, out);
}

bool Library::decodeCoverFile(const String &path, uint32_t offset, uint32_t size, uint16_t *out) const
{
    File file = fs.open(path);
    if (!file) {
        return false;
    }
    FileReader reader(file);
    // swapBytes = true: formato que o LVGL espera com LV_COLOR_16_SWAP = 1.
    bool ok = CoverDecoder::decodeJpeg(reader, offset, size, out, true);
    file.close();
    return ok;
}
