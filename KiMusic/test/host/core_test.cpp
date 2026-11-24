// =============================================================================
//  Teste do nucleo (Player + Menu) no PC, sem hardware.
//
//  Compilar e rodar todos os testes (na pasta KiMusic):
//    powershell -File test/host/run_tests.ps1
// =============================================================================
#include "test_util.hpp"
#include "core/Player.hpp"
#include "core/Menu.hpp"

#include <string>

// ---------- Fonte falsa: N musicas com duracoes conhecidas ----------
class FakeSource : public TrackSource
{
    public:
        std::vector<uint32_t> durations;

        explicit FakeSource(std::vector<uint32_t> d) : durations(d) {}

        size_t count() const override { return durations.size(); }
        std::string name(size_t i) const override { return "Faixa " + std::to_string(i + 1); }

        bool load(size_t i, Mp3Info &info) const override
        {
            info = Mp3Info();
            info.title = name(i);
            info.duration = durations[i];
            return true;
        }

        bool loadCover(size_t, const Mp3Info &, uint16_t *) const override { return false; }
};

// ---------- Tela falsa: guarda o ultimo estado recebido ----------
class FakeView : public PlayerView
{
    public:
        size_t trackIndex = 999;
        int trackChanges = 0;
        uint32_t elapsed = 0, duration = 0;
        bool playing = false;
        PlayerMode mode = PlayerMode::Normal;
        int volume = -1;
        bool menuOpen = false;
        std::string menuTitle;
        std::vector<std::string> menuRows;
        int menuSelected = -1;

        void trackChanged(const TrackSource &, size_t index, const Mp3Info &) override
        {
            trackIndex = index;
            trackChanges++;
        }
        void progressChanged(uint32_t e, uint32_t d) override { elapsed = e; duration = d; }
        void statusChanged(bool p, PlayerMode m) override { playing = p; mode = m; }
        void volumeChanged(int v) override { volume = v; }
        void menuChanged(const std::string &t, const std::vector<std::string> &rows, int sel) override
        {
            menuOpen = true; menuTitle = t; menuRows = rows; menuSelected = sel;
        }
        void menuClosed() override { menuOpen = false; }
};

// Avanca o relogio em passos de 100 ms (como o loop faria).
static void run(Player &p, uint32_t &now, uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += 100) {
        now += 100;
        p.update(now);
    }
}

// =============================================================================
//  Player
// =============================================================================

static void testBegin()
{
    std::printf("Player: begin carrega a primeira musica, pausado\n");
    FakeSource src({10, 20, 30});
    FakeView view;
    Player p(view);
    p.begin(src, 0);

    CHECK_EQ(view.trackIndex, (size_t)0);
    CHECK_EQ(view.duration, 10u);
    CHECK_EQ(view.playing, false);
    CHECK_EQ(view.volume, Player::VOLUME_DEFAULT);
}

static void testNextPrevious()
{
    std::printf("Player: proxima/anterior com volta nas pontas e reinicio\n");
    FakeSource src({100, 100, 100});
    FakeView view;
    Player p(view);
    uint32_t now = 0;
    p.begin(src, now);

    p.next(); CHECK_EQ(p.trackIndex(), (size_t)1);
    p.next(); CHECK_EQ(p.trackIndex(), (size_t)2);
    p.next(); CHECK_EQ(p.trackIndex(), (size_t)0);      // depois da ultima, a primeira

    p.previous(); CHECK_EQ(p.trackIndex(), (size_t)2);  // antes da primeira, a ultima

    // Tocou mais de 3 s: Anterior volta ao inicio da mesma musica.
    p.togglePlay();
    run(p, now, 5000);
    CHECK_EQ(p.elapsed(), 5u);
    p.previous();
    CHECK_EQ(p.trackIndex(), (size_t)2);
    CHECK_EQ(p.elapsed(), 0u);

    // Logo em seguida (<= 3 s): agora vai para a anterior.
    p.previous();
    CHECK_EQ(p.trackIndex(), (size_t)1);
}

static void testModes()
{
    std::printf("Player: modos Volume/Posicao e bloqueios\n");
    FakeSource src({100, 100});
    FakeView view;
    Player p(view);
    p.begin(src, 0);

    CHECK_EQ(p.toggleVolumeMode(), true);
    CHECK_EQ(view.mode == PlayerMode::Volume, true);
    CHECK_EQ(p.togglePositionMode(), false);   // bloqueado no modo Volume
    CHECK_EQ(p.next(), false);                 // Proxima/Anterior so no modo Normal
    CHECK_EQ(p.previous(), false);
    CHECK_EQ(p.trackIndex(), (size_t)0);

    p.rotate(+200);
    CHECK_EQ(p.volume(), Player::VOLUME_MAX);  // limite superior
    p.rotate(-500);
    CHECK_EQ(p.volume(), Player::VOLUME_MIN);  // limite inferior

    CHECK_EQ(p.toggleVolumeMode(), true);      // desliga
    CHECK_EQ(p.togglePositionMode(), true);
    CHECK_EQ(p.toggleVolumeMode(), false);     // bloqueado no modo Posicao

    p.rotate(+3);
    CHECK_EQ(p.elapsed(), 15u);                // 3 passos de 5 s
    p.rotate(-10);
    CHECK_EQ(p.elapsed(), 0u);                 // nao fica negativo
    p.rotate(+100);
    CHECK_EQ(p.elapsed(), 100u);               // nao passa da duracao

    p.resetMode();
    CHECK_EQ(view.mode == PlayerMode::Normal, true);
}

static void testClock()
{
    std::printf("Player: tempo so anda em Play\n");
    FakeSource src({100});
    FakeView view;
    Player p(view);
    uint32_t now = 0;
    p.begin(src, now);

    run(p, now, 3000);
    CHECK_EQ(p.elapsed(), 0u);                 // pausado

    p.togglePlay();
    run(p, now, 2500);
    CHECK_EQ(p.elapsed(), 2u);
    CHECK_EQ(view.elapsed, 2u);

    p.togglePlay();                            // pausa no meio do segundo
    run(p, now, 5000);
    CHECK_EQ(p.elapsed(), 2u);
}

static void testRepeatModes()
{
    std::printf("Player: fim da musica com Repetir Desligado/Todas/Uma\n");
    {
        FakeSource src({2, 2});
        FakeView view;
        Player p(view);
        uint32_t now = 0;
        p.begin(src, now);
        p.togglePlay();

        run(p, now, 2000);                     // fim da 1a -> vai para a 2a e continua
        CHECK_EQ(p.trackIndex(), (size_t)1);
        CHECK_EQ(p.isPlaying(), true);

        run(p, now, 2000);                     // fim da playlist (Desligado) -> 1a, pausado
        CHECK_EQ(p.trackIndex(), (size_t)0);
        CHECK_EQ(p.isPlaying(), false);
        CHECK_EQ(view.playing, false);
    }
    {
        FakeSource src({2, 2});
        FakeView view;
        Player p(view);
        uint32_t now = 0;
        p.begin(src, now);
        p.cycleRepeat();
        CHECK_EQ(p.repeat() == RepeatMode::All, true);
        p.next();                              // ja na ultima
        p.togglePlay();

        run(p, now, 2000);                     // Todas: volta para a 1a e continua tocando
        CHECK_EQ(p.trackIndex(), (size_t)0);
        CHECK_EQ(p.isPlaying(), true);
    }
    {
        FakeSource src({2, 2});
        FakeView view;
        Player p(view);
        uint32_t now = 0;
        p.begin(src, now);
        p.cycleRepeat();
        p.cycleRepeat();
        CHECK_EQ(p.repeat() == RepeatMode::One, true);
        p.togglePlay();
        int changesBefore = view.trackChanges;

        run(p, now, 2000);                     // Uma: recomeca a mesma
        CHECK_EQ(p.trackIndex(), (size_t)0);
        CHECK_EQ(p.elapsed(), 0u);
        CHECK_EQ(p.isPlaying(), true);
        CHECK_EQ(view.trackChanges, changesBefore);   // nao recarregou a musica

        p.cycleRepeat();
        CHECK_EQ(p.repeat() == RepeatMode::Off, true);
    }
}

static void testInvalidTrack()
{
    std::printf("Player: musica com duracao 0 e pulada\n");
    FakeSource src({0, 50});
    FakeView view;
    Player p(view);
    uint32_t now = 0;
    p.begin(src, now);
    p.togglePlay();
    run(p, now, 1000);
    CHECK_EQ(p.trackIndex(), (size_t)1);
    CHECK_EQ(p.isPlaying(), true);
}

// =============================================================================
//  Menu
// =============================================================================

static void testMenuMain()
{
    std::printf("Menu: pagina principal, Repetir e Voltar\n");
    FakeSource src({100, 100, 100});
    FakeView view;
    Player p(view);
    Menu m(p, view);
    p.begin(src, 0);

    p.toggleVolumeMode();
    m.open();
    CHECK_EQ(view.mode == PlayerMode::Normal, true);   // abrir sai do modo Volume
    CHECK_EQ(view.menuOpen, true);
    CHECK_EQ(view.menuTitle, std::string("Menu"));
    CHECK_EQ(view.menuRows.size(), (size_t)3);
    CHECK_EQ(view.menuRows[0], std::string("Músicas"));
    CHECK_EQ(view.menuRows[1], std::string("Repetir: Desligado"));
    CHECK_EQ(view.menuRows[2], std::string("Voltar"));
    CHECK_EQ(view.menuSelected, 0);

    m.rotate(-1);
    CHECK_EQ(m.selected(), 2);                 // da a volta para o ultimo

    m.rotate(-1);                              // Repetir
    m.select();
    CHECK_EQ(view.menuRows[1], std::string("Repetir: Todas"));
    m.select();
    CHECK_EQ(view.menuRows[1], std::string("Repetir: Uma"));
    m.select();
    CHECK_EQ(view.menuRows[1], std::string("Repetir: Desligado"));

    m.rotate(+1);                              // Voltar
    m.select();
    CHECK_EQ(m.isOpen(), false);
    CHECK_EQ(view.menuOpen, false);
}

static void testMenuTracks()
{
    std::printf("Menu: lista de musicas rolando e escolha\n");
    std::vector<uint32_t> d(20, 100);
    FakeSource src(d);
    FakeView view;
    Player p(view);
    Menu m(p, view);
    p.begin(src, 0);
    p.next(); p.next();                        // musica atual: indice 2

    m.open();
    m.select();                                // Musicas
    CHECK_EQ(m.page() == Menu::Page::Tracks, true);
    CHECK_EQ(m.selected(), 2);                 // comeca na musica atual
    CHECK_EQ(view.menuTitle, std::string("Músicas (3/20)"));
    CHECK_EQ(view.menuRows.size(), (size_t)Menu::VISIBLE_ROWS);
    CHECK_EQ(view.menuRows[2], std::string("> Faixa 3"));   // marcada como atual
    CHECK_EQ(view.menuSelected, 2);

    m.rotate(+8);                              // indice 10: janela centrada (7..13)
    CHECK_EQ(view.menuRows[0], std::string("  Faixa 8"));
    CHECK_EQ(view.menuSelected, 3);

    m.rotate(+9);                              // indice 19: janela no fim (13..19)
    CHECK_EQ(view.menuRows[0], std::string("  Faixa 14"));
    CHECK_EQ(view.menuSelected, 6);

    m.rotate(+1);                              // volta para o indice 0
    CHECK_EQ(m.selected(), 0);
    CHECK_EQ(view.menuSelected, 0);

    m.back();                                  // volta para a pagina principal
    CHECK_EQ(m.page() == Menu::Page::Main, true);
    CHECK_EQ(view.menuTitle, std::string("Menu"));

    m.select();                                // Musicas de novo
    m.rotate(+3);                              // indice 5
    m.select();                                // toca e fecha
    CHECK_EQ(p.trackIndex(), (size_t)5);
    CHECK_EQ(p.isPlaying(), true);
    CHECK_EQ(m.isOpen(), false);
    CHECK_EQ(view.menuOpen, false);

    m.open();
    m.back();                                  // Anterior na pagina principal fecha
    CHECK_EQ(m.isOpen(), false);
}

int main()
{
    testBegin();
    testNextPrevious();
    testModes();
    testClock();
    testRepeatModes();
    testInvalidTrack();
    testMenuMain();
    testMenuTracks();

    std::printf(testFailures() ? "\n%d FALHA(S)\n" : "\nTodos os testes passaram.\n", testFailures());
    return testFailures() ? 1 : 0;
}
