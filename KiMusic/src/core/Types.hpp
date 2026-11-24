// =============================================================================
//  core/Types.hpp  -  Tipos compartilhados entre o nucleo e a interface
// =============================================================================
#ifndef CORE_TYPES_HPP
#define CORE_TYPES_HPP

// O que o encoder controla quando o menu esta fechado.
enum class PlayerMode
{
    Normal,     // encoder sem funcao; Anterior/Proxima trocam de musica
    Volume,     // encoder ajusta o volume (barra de volume visivel)
    Position    // encoder avanca/volta a musica
};

// O que acontece quando a musica termina.
enum class RepeatMode
{
    Off,    // passa para a proxima; no fim da lista volta para a primeira e pausa
    All,    // passa para a proxima; no fim da lista volta para a primeira e continua
    One     // repete a musica atual
};

// Ordem de reproducao.
enum class ShuffleMode
{
    Off,        // ordem da lista
    Playlist,   // embaralha a lista atual (playlist escolhida)
    All         // troca para todas as musicas do SD e embaralha
};

inline const char *repeatModeName(RepeatMode mode)
{
    switch (mode) {
        case RepeatMode::All: return "Todas";
        case RepeatMode::One: return "Uma";
        default:              return "Desligado";
    }
}

inline const char *shuffleModeName(ShuffleMode mode)
{
    switch (mode) {
        case ShuffleMode::Playlist: return "Playlist";
        case ShuffleMode::All:      return "Geral";
        default:                    return "Desligado";
    }
}

#endif
