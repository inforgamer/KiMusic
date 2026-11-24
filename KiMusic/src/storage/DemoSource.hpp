// =============================================================================
//  storage/DemoSource.hpp  -  Musica de exemplo quando nao ha cartao SD
// -----------------------------------------------------------------------------
//  Uma unica musica, sem arquivo e sem capa. Usada quando o SD falha ou a
//  pasta de musicas esta vazia (por exemplo na simulacao do Wokwi), para a
//  interface e os botoes continuarem funcionando.
// =============================================================================
#ifndef STORAGE_DEMOSOURCE_HPP
#define STORAGE_DEMOSOURCE_HPP

#include "core/TrackSource.hpp"

class DemoSource : public TrackSource
{
    public:
        size_t count() const override { return 1; }

        std::string name(size_t) const override
        {
            return "Full Moon Full Life (demo)";
        }

        bool load(size_t, Mp3Info &info) const override
        {
            info = Mp3Info();
            info.title = "Full Moon Full Life";
            info.artist = "Demo (sem cartão SD)";
            info.album = "Persona 3 Reload Original Soundtrack";
            info.duration = 293;
            return true;
        }

        bool loadCover(size_t, const Mp3Info &, uint16_t *) const override
        {
            return false;
        }
};

#endif
