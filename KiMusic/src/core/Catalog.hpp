// =============================================================================
//  core/Catalog.hpp  -  Todas as musicas e as playlists
// -----------------------------------------------------------------------------
//  Usado pelo Menu (paginas "Playlists" e "Favoritar") e pelo modo
//  Aleatorio Geral. Implementacao: storage/Catalog (arquivos .m3u no SD).
// =============================================================================
#ifndef CORE_CATALOG_HPP
#define CORE_CATALOG_HPP

#include <stddef.h>
#include <string>

#include "core/TrackSource.hpp"

class Catalog
{
    public:
        virtual ~Catalog() {}

        // Todas as musicas do cartao (pode ter count() == 0 sem SD).
        virtual const TrackSource &allTracks() const = 0;

        // Playlists personalizadas (nao inclui "Todas as musicas").
        virtual size_t playlistCount() const = 0;
        virtual std::string playlistName(size_t index) const = 0;

        // Indice da playlist Favoritas (-1 se ela ainda nao existe).
        virtual long favoritesIndex() const = 0;

        // Carrega a playlist. Retorna nullptr se ela estiver vazia ou nao abrir.
        // O ponteiro continua valido ate a proxima chamada de openPlaylist().
        virtual const TrackSource *openPlaylist(size_t index) = 0;

        // Acrescenta a musica na playlist "Favoritas" (cria se nao existir).
        // Retorna false se ela ja estiver la ou nao der para gravar.
        virtual bool addToFavorites(const TrackSource &source, size_t index) = 0;
};

#endif
