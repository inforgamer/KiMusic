// =============================================================================
//  core/PlayerView.hpp  -  Como o nucleo avisa a interface
// -----------------------------------------------------------------------------
//  O Player e o Menu nao desenham nada: eles chamam estes metodos sempre que
//  algo muda. Implementacoes:
//    - view/ScreenView   : desenha no display (LVGL) e escreve no Serial
//    - testes no PC      : guardam as chamadas para conferir (core_test.cpp)
// =============================================================================
#ifndef CORE_PLAYERVIEW_HPP
#define CORE_PLAYERVIEW_HPP

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "Mp3Info.hpp"
#include "core/Types.hpp"
#include "core/TrackSource.hpp"

class PlayerView
{
    public:
        virtual ~PlayerView() {}

        // ---- Player ----

        // Nova musica carregada. source/index permitem buscar a capa.
        virtual void trackChanged(const TrackSource &source, size_t index, const Mp3Info &info) = 0;

        // Tempo tocado mudou (segundos).
        virtual void progressChanged(uint32_t elapsed, uint32_t duration) = 0;

        // Play/Pause ou modo do encoder mudou.
        virtual void statusChanged(bool playing, PlayerMode mode) = 0;

        // Volume mudou (0..100).
        virtual void volumeChanged(int volume) = 0;

        // ---- Menu ----

        // Menu aberto ou atualizado. rows sao so as linhas visiveis;
        // selectedRow e o indice da linha destacada dentro de rows.
        virtual void menuChanged(const std::string &title,
                                 const std::vector<std::string> &rows,
                                 int selectedRow) = 0;

        virtual void menuClosed() = 0;
};

#endif
