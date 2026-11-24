#ifndef LIBRARY_HPP
#define LIBRARY_HPP

#include <Arduino.h>
#include <FS.h>
#include <vector>

#include "Mp3Info.hpp"

// Lista de musicas (.mp3) encontradas em uma pasta do cartao SD.
class Library
{
    private:
        static constexpr size_t MAX_TRACKS = 200;

        std::vector<String> paths;

    public:
        // Varre a pasta (nao recursivo) e ordena por nome. Retorna quantas musicas achou.
        size_t scan(fs::FS &fs, const char *folder);

        size_t count() const { return paths.size(); }
        const String &path(size_t index) const { return paths[index]; }

        // Le titulo/artista/album/duracao. Campos vazios recebem valores padrao
        // (titulo = nome do arquivo).
        bool load(fs::FS &fs, size_t index, Mp3Info &info) const;
};

#endif
