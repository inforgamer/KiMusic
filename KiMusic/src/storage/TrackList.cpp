// =============================================================================
//  storage/TrackList.cpp  -  Implementacao (ver TrackList.hpp)
// =============================================================================
#include "storage/TrackList.hpp"
#include "storage/FileReader.hpp"
#include "CoverDecoder.hpp"
#include "UnsyncReader.hpp"

#include <algorithm>

namespace {

bool isMp3(const String &fileName)
{
    String lower = fileName;
    lower.toLowerCase();
    return lower.endsWith(".mp3") && !lower.startsWith(".");   // ignora "._arquivo" do macOS
}

} // namespace

TrackList::TrackList(fs::FS &f, const char *cover) : fs(f), coverFolder(cover) {}

String TrackList::baseName(const String &path)
{
    String fileName = path.substring(path.lastIndexOf('/') + 1);
    int dot = fileName.lastIndexOf('.');
    return dot > 0 ? fileName.substring(0, dot) : fileName;
}

// -----------------------------------------------------------------------------
//  Montar a lista
// -----------------------------------------------------------------------------

size_t TrackList::scanFolder(const char *folder)
{
    paths.clear();
    if (!fs.exists(folder)) {
        return 0;   // exists() nao gera log de erro; open() de pasta inexistente gera
    }

    File dir = fs.open(folder);
    if (!dir || !dir.isDirectory()) {
        return 0;
    }

    String base = folder;
    if (!base.endsWith("/")) {
        base += "/";
    }

    size_t found = 0;   // todos os MP3, mesmo os que passam do limite
    // So o nome: openNextFile() abriria cada arquivo (nova busca na pasta a cada um).
    bool isDir = false;
    for (String entry = dir.getNextFileName(&isDir); entry.length() > 0;
         entry = dir.getNextFileName(&isDir)) {
        String fileName = entry.substring(entry.lastIndexOf('/') + 1);
        if (!isDir && isMp3(fileName)) {
            found++;
            if (paths.size() < MAX_TRACKS) {
                paths.push_back(base + fileName);
            }
        }
    }
    dir.close();

    if (found > MAX_TRACKS) {
        // shortcut: ficam as primeiras na ordem do cartao (nao alfabetica); subir MAX_TRACKS se precisar.
        Serial.printf("Aviso: %u MP3 em %s, so %u carregados (limite)\r\n",
                      (unsigned)found, folder, (unsigned)MAX_TRACKS);
    }

    std::sort(paths.begin(), paths.end(), [](const String &a, const String &b) {
        return a.compareTo(b) < 0;
    });
    return paths.size();
}

void TrackList::setPaths(std::vector<String> &&newPaths)
{
    paths = std::move(newPaths);
    if (paths.size() > MAX_TRACKS) {
        paths.resize(MAX_TRACKS);
    }
}

std::string TrackList::name(size_t index) const
{
    return index < paths.size() ? std::string(baseName(paths[index]).c_str()) : std::string();
}

std::string TrackList::path(size_t index) const
{
    return index < paths.size() ? std::string(paths[index].c_str()) : std::string();
}

// -----------------------------------------------------------------------------
//  Dados de uma musica
// -----------------------------------------------------------------------------

bool TrackList::load(size_t index, Mp3Info &info) const
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
    // Curtos para caber na linha da tela ("Álbum desconhecido" aparecia como "Album...").
    if (info.artist.empty()) info.artist = "Sem artista";
    if (info.album.empty())  info.album  = "Sem álbum";
    return ok;
}

// -----------------------------------------------------------------------------
//  Capa
// -----------------------------------------------------------------------------

bool TrackList::loadCover(const std::string &trackPath, const Mp3Info &info, uint16_t *out) const
{
    if (trackPath.empty()) {
        return false;
    }
    String path(trackPath.c_str());

    // 1) Imagem embutida no proprio MP3 (JPEG ou PNG).
    if ((info.coverFormat == CoverFormat::Jpeg || info.coverFormat == CoverFormat::Png) &&
        decodeCoverFile(path, &info, out)) {
        return true;
    }

    // 2) Arquivo com o mesmo nome da musica na pasta de capas.
    static const char *const EXTENSIONS[] = {".jpg", ".jpeg", ".png"};
    String base = coverFolder + "/" + baseName(path);
    for (const char *ext : EXTENSIONS) {
        String coverPath = base + ext;
        if (fs.exists(coverPath) && decodeCoverFile(coverPath, nullptr, out)) {
            return true;
        }
    }
    return false;
}

// embedded == nullptr: o arquivo inteiro e a imagem. Senao: capa dentro do MP3.
bool TrackList::decodeCoverFile(const String &path, const Mp3Info *embedded, uint16_t *out) const
{
    File file = fs.open(path);
    if (!file) {
        return false;
    }
    FileReader reader(file);
    // swapBytes = true: formato que o LVGL espera com LV_COLOR_16_SWAP = 1.
    bool ok;
    if (!embedded) {
        ok = CoverDecoder::decode(reader, 0, 0, out, true);
    } else if (embedded->coverUnsync) {
        UnsyncReader unsync(reader, embedded->coverUnsyncStart, embedded->coverUnsyncEnd);
        ok = CoverDecoder::decode(unsync, embedded->coverOffset, embedded->coverSize, out, true);
    } else {
        ok = CoverDecoder::decode(reader, embedded->coverOffset, embedded->coverSize, out, true);
    }
    file.close();
    return ok;
}
