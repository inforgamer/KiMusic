// =============================================================================
//  storage/PlaylistPath.hpp  -  Linha de um .m3u -> caminho no cartao SD
// -----------------------------------------------------------------------------
//  So mexe no texto (sem tocar no SD), para ser testado no PC
//  (test/host/core_test.cpp). Quem confere se o arquivo existe e o SdCatalog.
// =============================================================================
#ifndef STORAGE_PLAYLISTPATH_HPP
#define STORAGE_PLAYLISTPATH_HPP

#include <string>
#include <vector>

// Devolve o caminho absoluto e normalizado da linha, ou "" para linha vazia/comentario.
//   "/music/a.mp3"        -> "/music/a.mp3"
//   "../music/a.mp3"      -> "/music/a.mp3"      (relativo a playlistFolder)
//   "a.mp3"               -> "<playlistFolder>/a.mp3"
//   "music\\a.mp3"        -> barras do Windows viram '/'
// Tambem tira BOM do UTF-8, espacos e \r das pontas.
inline std::string m3uLineToPath(std::string line, const std::string &playlistFolder)
{
    if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
        line.erase(0, 3);
    }
    const char *space = " \t\r\n";
    size_t first = line.find_first_not_of(space);
    if (first == std::string::npos || line[first] == '#') {
        return std::string();
    }
    line = line.substr(first, line.find_last_not_of(space) - first + 1);

    for (char &c : line) {
        if (c == '\\') c = '/';
    }
    if (line[0] != '/') {
        line = playlistFolder + "/" + line;
    }

    // Resolve "." e ".." (ex.: /playlists/../music/a.mp3 -> /music/a.mp3).
    std::vector<std::string> parts;
    size_t start = 1;
    while (start <= line.size()) {
        size_t slash = line.find('/', start);
        if (slash == std::string::npos) slash = line.size();
        std::string part = line.substr(start, slash - start);
        if (part == "..") {
            if (!parts.empty()) parts.pop_back();
        } else if (!part.empty() && part != ".") {
            parts.push_back(part);
        }
        start = slash + 1;
    }
    std::string path;
    for (const std::string &part : parts) {
        path += "/" + part;
    }
    return path;
}

#endif
