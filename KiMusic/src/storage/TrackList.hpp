// =============================================================================
//  storage/TrackList.hpp  -  Lista de musicas do cartao SD
// -----------------------------------------------------------------------------
//  Implementa TrackSource sobre uma lista de caminhos de MP3. A mesma classe
//  serve para "todas as musicas" (scanFolder) e para uma playlist .m3u
//  (setPaths, ver SdCatalog).
//
//    - load()      le tags e duracao com Mp3Info (titulo vazio = nome do arquivo)
//    - loadCover() capa: JPEG embutido no MP3 -> <pasta de capas>/<nome>.jpg
//
//  So os caminhos ficam na memoria; os dados de cada musica sao lidos do SD
//  na hora em que ela e carregada.
// =============================================================================
#ifndef STORAGE_TRACKLIST_HPP
#define STORAGE_TRACKLIST_HPP

#include <Arduino.h>
#include <FS.h>
#include <vector>

#include "core/TrackSource.hpp"

class TrackList : public TrackSource
{
    public:
        static constexpr size_t MAX_TRACKS = 200;

        TrackList(fs::FS &fs, const char *coverFolder);

        // Lista os .mp3 de uma pasta (nao recursivo, ordem alfabetica).
        size_t scanFolder(const char *folder);

        // Substitui a lista (playlist). Os caminhos devem ser absolutos.
        void setPaths(std::vector<String> &&newPaths);

        // ---- TrackSource ----
        size_t count() const override { return paths.size(); }
        std::string name(size_t index) const override;
        std::string path(size_t index) const override;
        bool load(size_t index, Mp3Info &info) const override;
        bool loadCover(size_t index, const Mp3Info &info, uint16_t *out) const override;

        // "/music/Faixa 01.mp3" -> "Faixa 01"
        static String baseName(const String &path);

    private:
        fs::FS &fs;
        String coverFolder;
        std::vector<String> paths;

        bool decodeCoverFile(const String &path, uint32_t offset, uint32_t size, uint16_t *out) const;
};

#endif
