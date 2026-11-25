// =============================================================================
//  Teste do nucleo (Player + Menu) no PC, sem hardware.
//
//  Compilar e rodar todos os testes (na pasta KiMusic):
//    powershell -File test/host/run_tests.ps1
// =============================================================================
#include "test_util.hpp"
#include "core/Player.hpp"
#include "core/Menu.hpp"
#include "input/Button.hpp"
#include "input/Encoder.hpp"
#include "storage/PlaylistPath.hpp"

#include <algorithm>
#include <string>

// ---------- Fonte falsa: N musicas com duracoes conhecidas ----------
class FakeSource : public TrackSource
{
    public:
        std::vector<uint32_t> durations;
        std::string prefix;        // muda os caminhos/nomes (para simular playlists)
        bool withFiles;

        explicit FakeSource(std::vector<uint32_t> d, std::string p = "Faixa ", bool files = false)
            : durations(d), prefix(p), withFiles(files) {}

        size_t count() const override { return durations.size(); }
        std::string name(size_t i) const override { return prefix + std::to_string(i + 1); }
        std::string path(size_t i) const override
        {
            return withFiles ? "/music/" + name(i) + ".mp3" : std::string();
        }

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
        std::string trackTitle;
        int trackChanges = 0;
        uint32_t elapsed = 0, duration = 0;
        bool playing = false;
        PlayerMode mode = PlayerMode::Normal;
        int volume = -1;
        bool menuOpen = false;
        std::string menuTitle;
        std::vector<std::string> menuRows;
        int menuSelected = -1;

        void trackChanged(const TrackSource &, size_t index, const Mp3Info &info) override
        {
            trackIndex = index;
            trackTitle = info.title;
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

// ---------- Audio falso: registra os comandos ----------
class FakeAudio : public AudioBackend
{
    public:
        std::string path;
        uint32_t startedAt = 0;
        int starts = 0, pauses = 0, resumes = 0, stops = 0;
        bool failNext = false;
        bool failAlways = false;
        bool isPaused = false;
        uint32_t pos = 0;
        bool done = false;
        int volume = -1;

        bool start(const std::string &p, const Mp3Info &, uint32_t s) override
        {
            starts++;
            if (failNext || failAlways) { failNext = false; return false; }
            path = p; startedAt = s; pos = s; done = false; isPaused = false;
            return true;
        }
        void pause() override { pauses++; isPaused = true; }
        void resume() override { resumes++; isPaused = false; }
        void stop() override { stops++; path.clear(); }
        void setVolume(int v) override { volume = v; }
        uint32_t position() const override { return pos; }
        bool finished() const override { return done; }
};

// ---------- Catalogo falso: todas as musicas + playlists ----------
class FakeCatalog : public Catalog
{
    public:
        FakeSource all;
        std::vector<FakeSource> lists;
        std::vector<std::string> names;
        std::vector<std::string> favorites;

        FakeCatalog() : all({100, 100, 100, 100, 100, 100}, "Geral ", true) {}

        const TrackSource &allTracks() const override { return all; }
        size_t playlistCount() const override { return lists.size(); }
        std::string playlistName(size_t i) const override { return names[i]; }
        long favoritesIndex() const override
        {
            for (size_t i = 0; i < names.size(); i++) if (names[i] == "Favoritas") return (long)i;
            return -1;
        }
        const TrackSource *openPlaylist(size_t i) override
        {
            return lists[i].count() ? &lists[i] : nullptr;
        }
        bool addToFavorites(const TrackSource &src, size_t index) override
        {
            std::string p = src.path(index);
            if (std::find(favorites.begin(), favorites.end(), p) != favorites.end()) return false;
            favorites.push_back(p);
            return true;
        }
};

// ---------- Aparelho falso ----------
class FakeDevice : public DeviceControl
{
    public:
        int level = 80;
        int powerOffs = 0;
        bool screenIsOff = false;

        int brightness() const override { return level; }
        void setBrightness(int p) override
        {
            level = std::max(BRIGHTNESS_MIN, std::min(BRIGHTNESS_MAX, p));
        }
        void screenOff() override { screenIsOff = true; }
        void screenOn() override { screenIsOff = false; }
        bool isScreenOff() const override { return screenIsOff; }
        void powerOff() override { powerOffs++; }
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
//  Player: regras basicas (tempo simulado)
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
    std::printf("Player: tempo simulado so anda em Play\n");
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

        run(p, now, 2000);                     // fim da lista (Desligado) -> 1a, pausado
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
//  Player: audio real (AudioBackend)
// =============================================================================

static void testAudioPlayback()
{
    std::printf("Audio: play/pause/volume, tempo vindo do decodificador, fim da musica\n");
    FakeSource src({100, 100}, "Faixa ", true);
    FakeView view;
    FakeAudio audio;
    Player p(view);
    p.setAudio(&audio);
    uint32_t now = 0;
    p.begin(src, now);

    CHECK_EQ(audio.volume, Player::VOLUME_DEFAULT);
    CHECK_EQ(audio.starts, 0);                 // pausado: nada tocando

    p.togglePlay();
    CHECK_EQ(audio.starts, 1);
    CHECK_EQ(audio.path, std::string("/music/Faixa 1.mp3"));
    CHECK_EQ(audio.startedAt, 0u);

    audio.pos = 42;                            // o tempo vem do decodificador...
    run(p, now, 100);
    CHECK_EQ(p.elapsed(), 42u);
    CHECK_EQ(view.elapsed, 42u);
    run(p, now, 5000);                         // ...e nao do relogio
    CHECK_EQ(p.elapsed(), 42u);

    p.togglePlay();
    CHECK_EQ(audio.pauses, 1);
    p.togglePlay();
    CHECK_EQ(audio.resumes, 1);
    CHECK_EQ(audio.starts, 1);                 // retomou, nao reabriu

    p.toggleVolumeMode();
    p.rotate(+5);
    CHECK_EQ(audio.volume, Player::VOLUME_DEFAULT + 5);
    p.toggleVolumeMode();

    audio.done = true;                         // decodificador chegou ao fim
    run(p, now, 100);
    CHECK_EQ(p.trackIndex(), (size_t)1);
    CHECK_EQ(audio.path, std::string("/music/Faixa 2.mp3"));
    CHECK_EQ(audio.starts, 2);
}

static void testAudioSeek()
{
    std::printf("Audio: posicao pelo encoder so salta quando o encoder para\n");
    FakeSource src({300}, "Faixa ", true);
    FakeView view;
    FakeAudio audio;
    Player p(view);
    p.setAudio(&audio);
    uint32_t now = 0;
    p.begin(src, now);
    p.togglePlay();
    audio.pos = 10;
    run(p, now, 100);

    p.togglePositionMode();
    p.rotate(+1);                              // 15 s
    p.rotate(+1);                              // 20 s
    CHECK_EQ(p.elapsed(), 20u);
    CHECK_EQ(audio.starts, 1);                 // ainda nao saltou
    run(p, now, 200);
    CHECK_EQ(p.elapsed(), 20u);                // nao foi sobrescrito pelo audio antigo
    run(p, now, 300);
    CHECK_EQ(audio.starts, 2);                 // saltou uma vez so
    CHECK_EQ(audio.startedAt, 20u);

    // Pausado: o salto acontece ao dar Play, ja na nova posicao.
    p.togglePlay();
    p.rotate(+2);                              // 30 s
    p.togglePlay();
    CHECK_EQ(audio.startedAt, 30u);
    int starts = audio.starts;
    run(p, now, 1000);
    CHECK_EQ(audio.starts, starts);            // nao abriu de novo
}

static void testAudioError()
{
    std::printf("Audio: arquivo que nao abre e pulado\n");
    FakeSource src({100, 100}, "Faixa ", true);
    FakeView view;
    FakeAudio audio;
    Player p(view);
    p.setAudio(&audio);
    uint32_t now = 0;
    p.begin(src, now);
    audio.failNext = true;
    p.togglePlay();
    run(p, now, 100);
    CHECK_EQ(p.trackIndex(), (size_t)1);
    CHECK_EQ(audio.path, std::string("/music/Faixa 2.mp3"));
    CHECK_EQ(p.isPlaying(), true);
}

static void testAudioAllFail()
{
    std::printf("Audio: nenhum arquivo abre (SD removido) com Repetir Todas: pausa\n");
    FakeSource src({100, 100, 100}, "Faixa ", true);
    FakeView view;
    FakeAudio audio;
    Player p(view);
    p.setAudio(&audio);
    uint32_t now = 0;
    p.begin(src, now);
    p.cycleRepeat();                           // Todas: sem a correcao, giraria para sempre
    audio.failAlways = true;
    p.togglePlay();
    run(p, now, 2000);
    CHECK_EQ(p.isPlaying(), false);
    CHECK_EQ(view.playing, false);
    CHECK_EQ(audio.starts, 3);                 // tentou cada musica uma vez e parou
    run(p, now, 2000);
    CHECK_EQ(audio.starts, 3);                 // e nao tenta mais sozinho

    // Falhas da lista anterior nao contam na nova: a 1a falha, a 2a toca.
    FakeSource other({100, 100}, "Outra ", true);
    audio.failAlways = false;
    audio.failNext = true;
    p.setSource(other, 0, true);
    run(p, now, 200);
    CHECK_EQ(p.isPlaying(), true);
    CHECK_EQ(audio.path, std::string("/music/Outra 2.mp3"));
}

// =============================================================================
//  Player: aleatorio e troca de lista
// =============================================================================

static bool isPermutation(std::vector<size_t> order, size_t n)
{
    std::sort(order.begin(), order.end());
    for (size_t i = 0; i < n; i++) if (order[i] != i) return false;
    return order.size() == n;
}

static void testShuffle()
{
    std::printf("Aleatorio: ordem embaralhada, atual na frente, todas tocam uma vez\n");
    std::vector<uint32_t> d(10, 1);
    FakeSource src(d);
    FakeView view;
    Player p(view);
    p.setRandomSeed(1234);
    uint32_t now = 0;
    p.begin(src, now);
    p.next(); p.next();                        // musica 2

    p.setShuffle(ShuffleMode::Playlist);
    CHECK_EQ(p.trackIndex(), (size_t)2);       // continua na mesma musica
    CHECK_EQ(p.playOrder()[0], (size_t)2);
    CHECK_EQ(isPermutation(p.playOrder(), 10), true);

    std::vector<size_t> sequential = {2, 3, 4, 5, 6, 7, 8, 9, 0, 1};
    CHECK_EQ(p.playOrder() != sequential, true);   // de fato embaralhou

    // Tocando tudo (Repetir Desligado): cada musica aparece uma vez e no fim pausa.
    std::vector<size_t> played = {p.trackIndex()};
    p.togglePlay();
    for (int i = 0; i < 9; i++) {
        run(p, now, 1000);
        played.push_back(p.trackIndex());
    }
    CHECK_EQ(isPermutation(played, 10), true);
    run(p, now, 1000);
    CHECK_EQ(p.isPlaying(), false);

    p.setShuffle(ShuffleMode::Off);
    size_t cur = p.trackIndex();
    p.next();
    CHECK_EQ(p.trackIndex(), (cur + 1) % 10);  // voltou a ordem normal
}

static void testShuffleAllAndSource()
{
    std::printf("Aleatorio Geral e troca de playlist\n");
    FakeCatalog cat;
    FakeSource playlist({100, 100}, "Lista ", true);
    FakeView view;
    Player p(view);
    uint32_t now = 0;
    p.begin(playlist, now);

    p.setShuffle(ShuffleMode::All, &cat.all);
    CHECK_EQ(p.trackCount(), (size_t)6);       // passou para todas as musicas
    CHECK_EQ(p.shuffle() == ShuffleMode::All, true);
    CHECK_EQ(view.trackTitle, std::string("Geral 1"));   // a atual nao estava la: comecou outra

    // Escolher uma playlist sai do Geral (vira aleatorio da playlist).
    p.setSource(playlist, 1, true);
    CHECK_EQ(p.trackCount(), (size_t)2);
    CHECK_EQ(p.trackIndex(), (size_t)1);
    CHECK_EQ(p.isPlaying(), true);
    CHECK_EQ(p.shuffle() == ShuffleMode::Playlist, true);

    // Geral mantendo a musica atual quando ela existe na lista geral.
    p.setSource(cat.all, 3, false);
    p.setShuffle(ShuffleMode::Off);
    p.setShuffle(ShuffleMode::All, &cat.all);
    CHECK_EQ(p.trackIndex(), (size_t)3);
}

// =============================================================================
//  Menu
// =============================================================================

static void testMenuMain()
{
    std::printf("Menu: pagina principal, Aleatorio, Repetir e Voltar\n");
    FakeSource src({100, 100, 100}, "Faixa ", true);
    FakeView view;
    FakeCatalog cat;
    FakeDevice dev;
    Player p(view);
    Menu m(p, view, &cat, &dev);
    p.begin(src, 0);

    p.toggleVolumeMode();
    m.open();
    CHECK_EQ(view.mode == PlayerMode::Normal, true);   // abrir sai do modo Volume
    CHECK_EQ(view.menuOpen, true);
    CHECK_EQ(view.menuTitle, std::string("Menu"));
    std::vector<std::string> expected = {
        "Músicas", "Playlists", "Aleatório: Desligado", "Repetir: Desligado",
        "Favoritar música", "Configurações", "Voltar"
    };
    CHECK_EQ(view.menuRows, expected);
    CHECK_EQ(view.menuSelected, 0);

    m.rotate(-1);
    CHECK_EQ(m.selected(), (int)Menu::ITEM_BACK);   // da a volta para o ultimo

    m.rotate(-3);                              // Repetir
    m.select();
    CHECK_EQ(view.menuRows[Menu::ITEM_REPEAT], std::string("Repetir: Todas"));
    m.select();
    CHECK_EQ(view.menuRows[Menu::ITEM_REPEAT], std::string("Repetir: Uma"));
    m.select();
    CHECK_EQ(view.menuRows[Menu::ITEM_REPEAT], std::string("Repetir: Desligado"));

    m.rotate(-1);                              // Aleatorio
    m.select();
    CHECK_EQ(view.menuRows[Menu::ITEM_SHUFFLE], std::string("Aleatório: Playlist"));
    m.select();
    CHECK_EQ(view.menuRows[Menu::ITEM_SHUFFLE], std::string("Aleatório: Geral"));
    CHECK_EQ(p.trackCount(), (size_t)6);       // Geral = todas as musicas do catalogo
    m.select();
    CHECK_EQ(view.menuRows[Menu::ITEM_SHUFFLE], std::string("Aleatório: Desligado"));

    m.rotate(+4);                              // Voltar
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
    Menu m(p, view, nullptr, nullptr);
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

    m.rotate(+9);                              // indice 19: janela no fim (14..20, com Voltar)
    CHECK_EQ(view.menuRows[0], std::string("  Faixa 15"));
    CHECK_EQ(view.menuSelected, 5);

    m.rotate(+1);                              // indice 20: Voltar (ultimo item)
    CHECK_EQ(view.menuRows.back(), std::string("Voltar"));
    CHECK_EQ(view.menuTitle, std::string("Músicas"));
    m.rotate(+1);                              // volta para o indice 0
    CHECK_EQ(m.selected(), 0);
    m.up();                                    // Vol sobe: vai para Voltar
    CHECK_EQ(m.selected(), 20);
    m.select();                                // Voltar -> pagina principal
    CHECK_EQ(m.page() == Menu::Page::Main, true);
    m.select();
    m.down();                                  // Pos desce
    CHECK_EQ(m.selected(), 3);

    m.back();                                  // Prev: volta para a pagina principal
    CHECK_EQ(m.page() == Menu::Page::Main, true);
    CHECK_EQ(m.selected(), (int)Menu::ITEM_TRACKS);

    m.select();                                // Musicas de novo
    m.rotate(+3);                              // indice 5
    m.select();                                // toca e fecha
    CHECK_EQ(p.trackIndex(), (size_t)5);
    CHECK_EQ(p.isPlaying(), true);
    CHECK_EQ(m.isOpen(), false);

    m.open();
    m.back();                                  // Anterior na pagina principal fecha
    CHECK_EQ(m.isOpen(), false);
}

static void testMenuPlaylists()
{
    std::printf("Menu: playlists em lista, Vol/Pos e Favoritar\n");
    FakeCatalog cat;
    cat.names = {"Favoritas", "Treino", "Vazia"};
    cat.lists = {FakeSource({100}, "Fav ", true), FakeSource({100, 100, 100}, "Treino ", true),
                 FakeSource({}, "X ", true)};
    FakeView view;
    Player p(view);
    Menu m(p, view, &cat, nullptr);
    p.begin(cat.all, 0);

    m.open();
    m.down(); m.select();                      // Pos desce ate Playlists e escolhe
    CHECK_EQ(m.page() == Menu::Page::Playlists, true);
    std::vector<std::string> expected = {"Favoritas", "Todas as músicas", "Treino", "Vazia", "Voltar"};
    CHECK_EQ(view.menuRows, expected);         // Favoritas primeiro, Voltar no fim

    m.down(); m.down(); m.down();              // Vazia
    m.select();
    CHECK_EQ(view.menuTitle, std::string("Playlist vazia"));
    CHECK_EQ(m.isOpen(), true);

    m.up();                                    // Vol sobe: Treino
    m.select();
    CHECK_EQ(m.isOpen(), false);
    CHECK_EQ(p.trackCount(), (size_t)3);
    CHECK_EQ(view.trackTitle, std::string("Treino 1"));
    CHECK_EQ(p.isPlaying(), true);

    m.open(); m.down(); m.select();
    m.select();                                // Favoritas
    CHECK_EQ(view.trackTitle, std::string("Fav 1"));

    // Favoritar a musica atual (uma vez so).
    m.open();
    m.rotate(+Menu::ITEM_FAVORITE);
    m.select();
    CHECK_EQ(view.menuTitle, std::string("Adicionada às Favoritas"));
    CHECK_EQ(cat.favorites.size(), (size_t)1);
    CHECK_EQ(cat.favorites[0], std::string("/music/Fav 1.mp3"));
    m.select();
    CHECK_EQ(view.menuTitle, std::string("Já está nas Favoritas"));
    m.down();
    CHECK_EQ(view.menuTitle, std::string("Menu"));   // aviso some na proxima acao

    // Item Voltar da pagina Playlists volta para o item Playlists.
    m.rotate(-(Menu::ITEM_SETTINGS - Menu::ITEM_PLAYLISTS));
    m.select();
    m.up();                                    // da a volta: ultimo item (Voltar)
    m.select();
    CHECK_EQ(m.page() == Menu::Page::Main, true);
    CHECK_EQ(m.selected(), (int)Menu::ITEM_PLAYLISTS);

    // Sem Favoritas.m3u a linha continua la, mas avisa que esta vazia.
    FakeCatalog empty;
    Menu m2(p, view, &empty, nullptr);
    m2.open(); m2.down(); m2.select();
    std::vector<std::string> few = {"Favoritas", "Todas as músicas", "Voltar"};
    CHECK_EQ(view.menuRows, few);
    m2.select();
    CHECK_EQ(view.menuTitle, std::string("Playlist vazia"));
}

static void testMenuSettings()
{
    std::printf("Menu: configuracoes - brilho, desligar tela e desligar\n");
    FakeSource src({100});
    FakeView view;
    FakeDevice dev;
    Player p(view);
    Menu m(p, view, nullptr, &dev);
    p.begin(src, 0);

    m.open();
    m.rotate(+Menu::ITEM_SETTINGS);
    m.select();
    CHECK_EQ(m.page() == Menu::Page::Settings, true);
    std::vector<std::string> expected = {"Brilho: 80%", "Desligar tela", "Desligar", "Voltar"};
    CHECK_EQ(view.menuRows, expected);

    m.select();                                // entra na edicao do brilho
    CHECK_EQ(m.isEditing(), true);
    CHECK_EQ(view.menuRows[0], std::string("Brilho: < 80% >"));
    m.rotate(-3);
    CHECK_EQ(dev.level, 50);
    m.rotate(-10);
    CHECK_EQ(dev.level, DeviceControl::BRIGHTNESS_MIN);   // nunca apaga de vez
    m.rotate(+20);
    CHECK_EQ(dev.level, DeviceControl::BRIGHTNESS_MAX);
    CHECK_EQ(m.selected(), 0);                 // a selecao nao andou durante a edicao
    m.select();                                // confirma
    CHECK_EQ(m.isEditing(), false);
    CHECK_EQ(view.menuRows[0], std::string("Brilho: 100%"));

    m.rotate(+1);                              // Desligar tela
    m.select();
    CHECK_EQ(dev.screenIsOff, true);
    CHECK_EQ(dev.powerOffs, 0);                // so a tela
    CHECK_EQ(m.isOpen(), false);
    dev.screenOn();

    m.open();
    m.rotate(+Menu::ITEM_SETTINGS);
    m.select();
    m.rotate(+Menu::SET_POWER_OFF);            // Desligar
    m.select();
    CHECK_EQ(dev.powerOffs, 1);
    CHECK_EQ(m.isOpen(), false);

    m.open();
    m.rotate(+Menu::ITEM_SETTINGS);
    m.select();
    m.back();                                  // volta para Configuracoes no menu principal
    CHECK_EQ(m.selected(), (int)Menu::ITEM_SETTINGS);
}

// =============================================================================
//  Botao: toque simples x toque duplo
// =============================================================================

// Simula o pino a cada 5 ms. 'pressedAt' diz se o botao esta apertado no instante t.
template <typename F>
static std::vector<std::pair<ButtonEvent, uint32_t>> runButton(Button &b, uint32_t untilMs, F pressedAt)
{
    std::vector<std::pair<ButtonEvent, uint32_t>> events;
    for (uint32_t t = 1000; t <= 1000 + untilMs; t += 5) {
        ButtonEvent e = b.update(pressedAt(t - 1000), t);
        if (e != ButtonEvent::None) events.push_back({e, t - 1000});
    }
    return events;
}

static void testButton()
{
    std::printf("Botao: toque simples, toque duplo e repique\n");
    {
        Button b(0, 400);   // toque simples: Click so depois da janela
        auto ev = runButton(b, 1500, [](uint32_t t) { return t < 100; });
        CHECK_EQ(ev.size(), (size_t)1);
        CHECK_EQ(ev[0].first == ButtonEvent::Click, true);
        CHECK_EQ(ev[0].second >= 500u, true);          // soltou em ~130 + 400
    }
    {
        Button b(0, 400);   // toque duplo: so DoubleClick, nunca Click
        auto ev = runButton(b, 2000, [](uint32_t t) {
            return t < 80 || (t >= 200 && t < 300);
        });
        CHECK_EQ(ev.size(), (size_t)1);
        CHECK_EQ(ev[0].first == ButtonEvent::DoubleClick, true);
    }
    {
        Button b(0, 400);   // 2o toque tarde demais: dois toques simples
        auto ev = runButton(b, 3000, [](uint32_t t) {
            return t < 80 || (t >= 1000 && t < 1080);
        });
        CHECK_EQ(ev.size(), (size_t)2);
        CHECK_EQ(ev[0].first == ButtonEvent::Click, true);
        CHECK_EQ(ev[1].first == ButtonEvent::Click, true);
    }
    {
        Button b(0, 400);   // repique de 10 ms no meio do aperto nao vira toque duplo
        auto ev = runButton(b, 1500, [](uint32_t t) {
            return t < 200 && !(t >= 100 && t < 110);
        });
        CHECK_EQ(ev.size(), (size_t)1);
        CHECK_EQ(ev[0].first == ButtonEvent::Click, true);
    }
    {
        Button b(0);        // botao simples: Click na hora do aperto
        auto ev = runButton(b, 500, [](uint32_t t) { return t >= 50 && t < 150; });
        CHECK_EQ(ev.size(), (size_t)1);
        CHECK_EQ(ev[0].first == ButtonEvent::Click, true);
        CHECK_EQ(ev[0].second < 100u, true);
    }
}

// =============================================================================
//  Encoder: tabela de 4 estados
// =============================================================================

// Passa uma sequencia de estados (bit 1 = pino A, bit 0 = pino B) e devolve os passos.
static int runEncoder(std::initializer_list<int> states)
{
    Encoder e(0, 0);
    for (int s : states) e.step((s & 2) != 0, (s & 1) != 0);
    return e.takeSteps();
}

static void testEncoder()
{
    std::printf("Encoder: sentido, repique e meio passo\n");
    CHECK_EQ(runEncoder({3, 1, 0, 2, 3}), +1);                  // horario (A desce antes de B)
    CHECK_EQ(runEncoder({3, 2, 0, 1, 3}), -1);                  // anti-horario
    CHECK_EQ(runEncoder({3, 1, 0, 2, 3, 1, 0, 2, 3}), +2);      // dois cliques
    CHECK_EQ(runEncoder({3, 1, 3, 1, 3, 1, 0, 2, 3}), +1);      // repique em A no comeco: 1 so
    CHECK_EQ(runEncoder({3, 1, 0, 2, 0, 2, 3}), +1);            // repique em A no fim: 1 so
    CHECK_EQ(runEncoder({3, 1, 0, 1, 3}), 0);                   // meio passo e volta: nada
}

// =============================================================================
//  Playlists: linha do .m3u -> caminho
// =============================================================================

static void testPlaylistPaths()
{
    std::printf("Playlist .m3u: caminhos absolutos, relativos, Windows, BOM e comentarios\n");
    const std::string folder = "/playlists";
    CHECK_EQ(m3uLineToPath("/music/a.mp3", folder), std::string("/music/a.mp3"));
    CHECK_EQ(m3uLineToPath("../music/a.mp3", folder), std::string("/music/a.mp3"));
    CHECK_EQ(m3uLineToPath("a.mp3", folder), std::string("/playlists/a.mp3"));
    CHECK_EQ(m3uLineToPath("./sub/../a.mp3", folder), std::string("/playlists/a.mp3"));
    CHECK_EQ(m3uLineToPath("..\\music\\Minha Musica.mp3", folder), std::string("/music/Minha Musica.mp3"));
    CHECK_EQ(m3uLineToPath("D:\\Musicas\\a.mp3", folder), std::string("/playlists/D:/Musicas/a.mp3"));
    CHECK_EQ(m3uLineToPath("  /music//a.mp3 \r", folder), std::string("/music/a.mp3"));
    CHECK_EQ(m3uLineToPath("\xEF\xBB\xBF/music/a.mp3", folder), std::string("/music/a.mp3"));
    CHECK_EQ(m3uLineToPath("../../../a.mp3", folder), std::string("/a.mp3"));   // nao sobe acima da raiz
    CHECK_EQ(m3uLineToPath("#EXTM3U", folder), std::string());
    CHECK_EQ(m3uLineToPath("#EXTINF:123,Artista - Musica", folder), std::string());
    CHECK_EQ(m3uLineToPath("   \r", folder), std::string());
    CHECK_EQ(m3uLineToPath("", folder), std::string());
}

int main()
{
    testButton();
    testEncoder();
    testPlaylistPaths();
    testBegin();
    testNextPrevious();
    testModes();
    testClock();
    testRepeatModes();
    testInvalidTrack();
    testAudioPlayback();
    testAudioSeek();
    testAudioError();
    testAudioAllFail();
    testShuffle();
    testShuffleAllAndSource();
    testMenuMain();
    testMenuTracks();
    testMenuPlaylists();
    testMenuSettings();

    std::printf(testFailures() ? "\n%d FALHA(S)\n" : "\nTodos os testes passaram.\n", testFailures());
    return testFailures() ? 1 : 0;
}
