// =============================================================================
//  storage/SdCatalog.cpp  -  Implementacao (ver SdCatalog.hpp)
// =============================================================================
#include "storage/SdCatalog.hpp"

#include <algorithm>

SdCatalog::SdCatalog(fs::FS &f, const char *music, const char *cover,
                     const char *playlists, const char *favorites)
    : fs(f), musicFolder(music), playlistFolder(playlists), favoritesName(favorites),
      all(f, cover), playlist(f, cover) {}

size_t SdCatalog::scan()
{
    size_t found = all.scanFolder(musicFolder.c_str());
    scanPlaylists();
    return found;
}

// -----------------------------------------------------------------------------
//  Lista de playlists
// -----------------------------------------------------------------------------

void SdCatalog::scanPlaylists()
{
    playlistFiles.clear();

    File dir = fs.open(playlistFolder);
    if (!dir || !dir.isDirectory()) {
        return;
    }

    File entry = dir.openNextFile();
    while (entry && playlistFiles.size() < MAX_PLAYLISTS) {
        String fileName = entry.name();
        fileName = fileName.substring(fileName.lastIndexOf('/') + 1);
        String lower = fileName;
        lower.toLowerCase();
        if (!entry.isDirectory() && !lower.startsWith(".") &&
            (lower.endsWith(".m3u") || lower.endsWith(".m3u8"))) {
            playlistFiles.push_back(playlistFolder + "/" + fileName);
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();

    // Favoritas primeiro; o resto em ordem alfabetica.
    const String fav = favoritesName;
    std::sort(playlistFiles.begin(), playlistFiles.end(), [&fav](const String &a, const String &b) {
        bool aFav = TrackList::baseName(a) == fav;
        bool bFav = TrackList::baseName(b) == fav;
        if (aFav != bFav) return aFav;
        return a.compareTo(b) < 0;
    });
}

std::string SdCatalog::playlistName(size_t index) const
{
    if (index >= playlistFiles.size()) {
        return std::string();
    }
    return std::string(TrackList::baseName(playlistFiles[index]).c_str());
}

// -----------------------------------------------------------------------------
//  Abrir uma playlist
// -----------------------------------------------------------------------------

const TrackSource *SdCatalog::openPlaylist(size_t index)
{
    if (index >= playlistFiles.size()) {
        return nullptr;
    }

    std::vector<String> paths;
    if (!readPlaylist(playlistFiles[index], paths) || paths.empty()) {
        return nullptr;   // a playlist carregada antes continua intacta
    }
    playlist.setPaths(std::move(paths));
    Serial.printf("Playlist %s: %u musicas\r\n",
                  playlistName(index).c_str(), (unsigned)playlist.count());
    return &playlist;
}

bool SdCatalog::readPlaylist(const String &m3uPath, std::vector<String> &out) const
{
    File file = fs.open(m3uPath);
    if (!file) {
        return false;
    }

    while (file.available() && out.size() < TrackList::MAX_TRACKS) {
        String line = file.readStringUntil('\n');
        line.trim();                                   // tira \r e espacos
        if (line.startsWith("\xEF\xBB\xBF")) {         // BOM do UTF-8
            line = line.substring(3);
        }
        if (line.length() == 0 || line.startsWith("#")) {
            continue;
        }
        String resolved = resolve(line);
        if (resolved.length() > 0) {
            out.push_back(resolved);
        }
    }
    file.close();
    return true;
}

// Converte uma linha do .m3u em caminho absoluto existente ("" se nao achar).
String SdCatalog::resolve(const String &line) const
{
    String path = line;
    path.replace('\\', '/');

    if (!path.startsWith("/")) {
        path = playlistFolder + "/" + path;
    }

    // Resolve "." e ".." (ex.: /playlists/../music/a.mp3 -> /music/a.mp3).
    std::vector<String> parts;
    int start = 1;
    while (start <= (int)path.length()) {
        int slash = path.indexOf('/', start);
        if (slash < 0) slash = path.length();
        String part = path.substring(start, slash);
        if (part == "..") {
            if (!parts.empty()) parts.pop_back();
        } else if (part.length() > 0 && part != ".") {
            parts.push_back(part);
        }
        start = slash + 1;
    }
    String normalized;
    for (const String &part : parts) {
        normalized += "/" + part;
    }

    if (fs.exists(normalized)) {
        return normalized;
    }

    // So o nome do arquivo: procura na pasta de musicas.
    String byName = musicFolder + "/" + normalized.substring(normalized.lastIndexOf('/') + 1);
    return fs.exists(byName) ? byName : String();
}

// -----------------------------------------------------------------------------
//  Favoritas
// -----------------------------------------------------------------------------

bool SdCatalog::addToFavorites(const TrackSource &source, size_t index)
{
    std::string trackPath = source.path(index);
    if (trackPath.empty()) {
        return false;
    }
    String target = String(trackPath.c_str());
    String favPath = playlistFolder + "/" + favoritesName + ".m3u";

    // Ja esta na lista?
    std::vector<String> current;
    if (fs.exists(favPath) && readPlaylist(favPath, current) &&
        std::find(current.begin(), current.end(), target) != current.end()) {
        return false;
    }

    if (!fs.exists(playlistFolder)) {
        fs.mkdir(playlistFolder);
    }
    bool isNew = !fs.exists(favPath);
    File file = fs.open(favPath, FILE_APPEND);
    if (!file) {
        return false;
    }
    if (isNew) {
        file.println("#EXTM3U");
    }
    file.println(target);
    file.close();

    if (isNew) {
        scanPlaylists();   // a playlist Favoritas passa a aparecer no menu
    }
    Serial.printf("Favoritas: + %s\r\n", target.c_str());
    return true;
}
