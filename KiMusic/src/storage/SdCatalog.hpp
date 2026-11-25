// =============================================================================
//  storage/SdCatalog.hpp  -  Todas as musicas + playlists .m3u do cartao SD
// -----------------------------------------------------------------------------
//  Implementa Catalog:
//    - allTracks():  todos os MP3 de /music
//    - playlists:    cada arquivo /playlists/<nome>.m3u vira uma playlist
//    - Favoritas:    /playlists/Favoritas.m3u, criada pelo "Favoritar musica"
//
//  Formato .m3u (texto, uma musica por linha; linhas com # sao ignoradas):
//      #EXTM3U
//      /music/Minha Musica.mp3        <- caminho absoluto no SD
//      ../music/Outra.mp3             <- relativo a pasta /playlists
//      Mais Uma.mp3                   <- so o nome: procurado em /music
//  Musicas que nao existirem no SD sao puladas (as de /music so na hora de tocar).
//
//  So uma playlist fica carregada por vez (a ultima aberta).
// =============================================================================
#ifndef STORAGE_SDCATALOG_HPP
#define STORAGE_SDCATALOG_HPP

#include <Arduino.h>
#include <FS.h>
#include <vector>

#include "core/Catalog.hpp"
#include "storage/TrackList.hpp"

class SdCatalog : public Catalog
{
    public:
        static constexpr size_t MAX_PLAYLISTS = 50;

        SdCatalog(fs::FS &fs, const char *musicFolder, const char *coverFolder,
                  const char *playlistFolder, const char *favoritesName);

        // Le /music e a lista de playlists. Retorna quantas musicas achou.
        size_t scan();

        // ---- Catalog ----
        const TrackSource &allTracks() const override { return all; }
        size_t playlistCount() const override { return playlistFiles.size(); }
        std::string playlistName(size_t index) const override;
        long favoritesIndex() const override;
        const TrackSource *openPlaylist(size_t index) override;
        bool addToFavorites(const TrackSource &source, size_t index) override;

    private:
        fs::FS &fs;
        String musicFolder;
        String playlistFolder;
        String favoritesName;

        TrackList all;                     // todas as musicas
        TrackList playlist;                // playlist aberta no momento
        std::vector<String> playlistFiles; // caminhos dos .m3u (ordem alfabetica)

        void scanPlaylists();
        bool readPlaylist(const String &m3uPath, std::vector<String> &out) const;
        String resolve(const String &normalized) const;
};

#endif
