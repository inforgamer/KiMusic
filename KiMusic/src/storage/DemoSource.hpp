// =============================================================================
//  storage/DemoSource.hpp  -  Musicas de exemplo quando nao ha cartao SD
// -----------------------------------------------------------------------------
//  Algumas musicas sem arquivo e sem capa. Usadas quando o SD falha ou a pasta
//  de musicas esta vazia (por exemplo na simulacao do Wokwi), para a interface,
//  os botoes e a troca de musica (Anterior/Proxima/menu) continuarem testaveis.
// =============================================================================
#ifndef STORAGE_DEMOSOURCE_HPP
#define STORAGE_DEMOSOURCE_HPP

#include "core/TrackSource.hpp"

class DemoSource : public TrackSource
{
    public:
        size_t count() const override { return COUNT; }

        std::string name(size_t index) const override
        {
            return std::string(track(index).title) + " (demo)";
        }

        bool load(size_t index, Mp3Info &info) const override
        {
            const Track &t = track(index);
            info = Mp3Info();
            info.title = t.title;
            info.artist = "Demo (sem cartão SD)";
            info.album = t.album;
            info.duration = t.duration;
            return true;
        }

        bool loadCover(const std::string &, const Mp3Info &, uint16_t *) const override
        {
            return false;
        }

    private:
        struct Track
        {
            const char *title;
            const char *album;
            uint32_t duration;   // segundos
        };

        static constexpr size_t COUNT = 3;

        static const Track &track(size_t index)
        {
            static const Track tracks[COUNT] = {
                {"Full Moon Full Life", "Persona 3 Reload Original Soundtrack", 293},
                {"SoundHelix Song 1",   "SoundHelix",                           373},
                {"SoundHelix Song 2",   "SoundHelix",                           426},
            };
            return tracks[index % COUNT];
        }
};

#endif
