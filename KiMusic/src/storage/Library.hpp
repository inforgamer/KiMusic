// =============================================================================
//  storage/Library.hpp  -  Musicas do cartao SD
// -----------------------------------------------------------------------------
//  Implementa TrackSource sobre uma pasta do SD:
//    - scan() lista os .mp3 da pasta de musicas (nao recursivo, ordem alfabetica)
//    - load() le tags e duracao com Mp3Info (titulo vazio = nome do arquivo)
//    - loadCover() procura a capa: JPEG embutido no MP3 -> <capas>/<nome>.jpg
//
//  So os caminhos ficam na memoria; os dados de cada musica sao lidos do SD
//  na hora em que ela e carregada.
// =============================================================================
#ifndef STORAGE_LIBRARY_HPP
#define STORAGE_LIBRARY_HPP

#include <Arduino.h>
#include <FS.h>
#include <vector>

#include "core/TrackSource.hpp"

class Library : public TrackSource
{
    public:
        static constexpr size_t MAX_TRACKS = 200;

        // fs: sistema de arquivos ja iniciado (ex.: SD depois de SD.begin()).
        Library(fs::FS &fs, const char *musicFolder, const char *coverFolder);

        // Varre a pasta de musicas. Retorna quantos MP3 encontrou.
        size_t scan();

        // Caminho completo no SD (para mensagens de erro).
        const String &path(size_t index) const { return paths[index]; }

        // ---- TrackSource ----
        size_t count() const override { return paths.size(); }
        std::string name(size_t index) const override;
        bool load(size_t index, Mp3Info &info) const override;
        bool loadCover(size_t index, const Mp3Info &info, uint16_t *out) const override;

    private:
        fs::FS &fs;
        String musicFolder;
        String coverFolder;
        std::vector<String> paths;

        bool decodeCoverFile(const String &path, uint32_t offset, uint32_t size, uint16_t *out) const;
};

#endif
