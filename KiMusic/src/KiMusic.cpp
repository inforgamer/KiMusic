#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "Screen.hpp"
#include "Library.hpp"

// ---------------- Pinagem (ver Documentação/Pinagem.md) ----------------
constexpr uint8_t PIN_ENCODER_A   = 6;
constexpr uint8_t PIN_ENCODER_B   = 7;
constexpr uint8_t PIN_CENTER      = 5;
constexpr uint8_t PIN_VOLUME      = 16;
constexpr uint8_t PIN_POSITION    = 17;
constexpr uint8_t PIN_PREVIOUS    = 18;
constexpr uint8_t PIN_NEXT        = 8;
constexpr uint8_t PIN_BUZZER      = 4;
constexpr uint8_t PIN_SD_CS       = 15;


class Encoder
{
    private:
        uint8_t pinA;
        uint8_t pinB;
        int lastState;

    public:
        Encoder(uint8_t pA, uint8_t pB) : pinA(pA), pinB(pB), lastState(HIGH) {}

        void begin()
        {
            pinMode(pinA, INPUT_PULLUP);
            pinMode(pinB, INPUT_PULLUP);
            lastState = digitalRead(pinA);
        }

        // Retorna +1, -1 ou 0 (sem movimento).
        int readRotate()
        {
            int stateA = digitalRead(pinA);
            if (stateA == lastState) {
                return 0;
            }
            lastState = stateA;

            if (stateA != LOW) {
                return 0;
            }
            return (digitalRead(pinB) != stateA) ? 1 : -1;
        }
};

class Button
{
    private:
        static constexpr uint32_t DEBOUNCE_MS = 30;

        uint8_t pinNumber;
        bool stableState;      // true = pressionado
        bool lastReading;
        uint32_t lastChange;

    public:
        explicit Button(uint8_t pin)
            : pinNumber(pin), stableState(false), lastReading(false), lastChange(0) {}

        void begin()
        {
            pinMode(pinNumber, INPUT_PULLUP);
        }

        // Debounce sem bloquear: retorna true uma vez, no momento em que o botao e pressionado.
        bool checkClick()
        {
            bool reading = (digitalRead(pinNumber) == LOW);
            uint32_t now = millis();

            if (reading != lastReading) {
                lastReading = reading;
                lastChange = now;
            }

            if ((now - lastChange) >= DEBOUNCE_MS && reading != stableState) {
                stableState = reading;
                return stableState;
            }
            return false;
        }
};

class Buzzer
{
    private:
        uint8_t pin;

    public:
        explicit Buzzer(uint8_t p) : pin(p) {}

        void begin()
        {
            pinMode(pin, OUTPUT);
        }

        void click()
        {
            tone(pin, 2000, 15);   // nao bloqueia
        }
};


// Musica usada quando o SD falha ou nao tem MP3 (ex.: simulacao no Wokwi).
Mp3Info demoTrack()
{
    Mp3Info info;
    info.title = "Full Moon Full Life";
    info.artist = "Demo (sem cartao SD)";
    info.album = "Persona 3 Reload Original Soundtrack";
    info.duration = 293;
    return info;
}


class Player
{
    private:
        static constexpr int VOLUME_MIN = 0;
        static constexpr int VOLUME_MAX = 100;
        static constexpr uint32_t POSITION_STEP = 5;      // segundos por clique do encoder
        static constexpr uint32_t RESTART_THRESHOLD = 3;  // "Anterior" reinicia se ja tocou mais que isso

        Screen &screen;
        Library &library;
        Encoder encoder;
        Button centerButton;
        Button previousButton;
        Button nextButton;
        Button volumeButton;
        Button positionButton;
        Buzzer buzzer;

        Mp3Info track;
        size_t trackIndex;
        bool hasLibrary;
        bool isPlay;
        PlayerMode mode;
        int volume;
        uint32_t elapsed;      // segundos tocados
        uint32_t lastSecond;

        void refreshProgress()
        {
            screen.setProgress(elapsed, track.duration);
        }

        void refreshStatus()
        {
            screen.setStatus(isPlay, mode);
        }

        void showTrack()
        {
            screen.setMusicInfo(track.title.c_str(), track.artist.c_str(), track.album.c_str());
            refreshProgress();
        }

        void toggleMode(PlayerMode target)
        {
            mode = (mode == target) ? PlayerMode::Normal : target;
            Serial.println(mode == PlayerMode::Volume   ? "Modo Volume!" :
                           mode == PlayerMode::Position ? "Modo Posicao!" : "Modo Normal!");
            refreshStatus();
        }

    public:
        Player(Screen &s, Library &l)
            : screen(s),
              library(l),
              encoder(PIN_ENCODER_A, PIN_ENCODER_B),
              centerButton(PIN_CENTER),
              previousButton(PIN_PREVIOUS),
              nextButton(PIN_NEXT),
              volumeButton(PIN_VOLUME),
              positionButton(PIN_POSITION),
              buzzer(PIN_BUZZER),
              trackIndex(0),
              hasLibrary(false),
              isPlay(false),
              mode(PlayerMode::Normal),
              volume(25),
              elapsed(0),
              lastSecond(0) {}

        void begin(bool useLibrary)
        {
            encoder.begin();
            centerButton.begin();
            previousButton.begin();
            nextButton.begin();
            volumeButton.begin();
            positionButton.begin();
            buzzer.begin();

            hasLibrary = useLibrary && library.count() > 0;
            if (hasLibrary) {
                loadTrack(0);
            } else {
                track = demoTrack();
                elapsed = 0;
                showTrack();
            }

            screen.setVolume(volume);
            refreshStatus();
        }

        void loadTrack(size_t index)
        {
            elapsed = 0;
            lastSecond = millis();

            if (!hasLibrary) {
                refreshProgress();
                return;
            }

            trackIndex = index % library.count();
            if (!library.load(SD, trackIndex, track)) {
                Serial.printf("Aviso: MP3 invalido ou sem frames: %s\r\n", library.path(trackIndex).c_str());
            }

            Serial.printf("[%u/%u] %s - %s (%lu s)\r\n",
                          (unsigned)(trackIndex + 1), (unsigned)library.count(),
                          track.artist.c_str(), track.title.c_str(),
                          (unsigned long)track.duration);
            showTrack();
        }

        void next()
        {
            loadTrack(trackIndex + 1);
        }

        // Volta ao inicio da musica; se ela acabou de comecar, vai para a anterior.
        void previous()
        {
            if (elapsed > RESTART_THRESHOLD || !hasLibrary) {
                elapsed = 0;
                refreshProgress();
                return;
            }
            loadTrack(trackIndex == 0 ? library.count() - 1 : trackIndex - 1);
        }

        void control()
        {
            int dif = encoder.readRotate();
            if (dif != 0) {
                if (mode == PlayerMode::Volume) {
                    setVolume(dif);
                } else if (mode == PlayerMode::Position) {
                    setPosition(dif);
                }
            }

            if (centerButton.checkClick()) {
                buzzer.click();
                isPlay = !isPlay;
                lastSecond = millis();
                Serial.println(isPlay ? "Play!" : "Pause!");
                refreshStatus();
            }
            else if (positionButton.checkClick() && mode != PlayerMode::Volume) {
                buzzer.click();
                toggleMode(PlayerMode::Position);
            }
            else if (volumeButton.checkClick() && mode != PlayerMode::Position) {
                buzzer.click();
                toggleMode(PlayerMode::Volume);
            }
            else if (previousButton.checkClick() && mode == PlayerMode::Normal) {
                buzzer.click();
                Serial.println("Musica Anterior!");
                previous();
            }
            else if (nextButton.checkClick() && mode == PlayerMode::Normal) {
                buzzer.click();
                Serial.println("Proxima musica!");
                next();
            }

            tick();
        }

        // Simula a reproducao enquanto o decodificador de audio nao existe.
        void tick()
        {
            if (!isPlay) {
                return;
            }
            uint32_t now = millis();
            if (now - lastSecond < 1000) {
                return;
            }
            lastSecond += 1000;

            if (elapsed < track.duration) {
                elapsed++;
                refreshProgress();
            }
            if (elapsed >= track.duration) {
                Serial.println("Fim da musica!");
                bool wasLast = !hasLibrary || trackIndex + 1 >= library.count();
                next();
                if (wasLast) {           // fim da playlist: volta para a primeira e pausa
                    isPlay = false;
                    refreshStatus();
                }
            }
        }

        void setVolume(int delta)
        {
            volume = constrain(volume + delta, VOLUME_MIN, VOLUME_MAX);
            Serial.printf("Volume atual: %d\r\n", volume);
            screen.setVolume(volume);
        }

        // Avanca/retrocede a musica de acordo com a rotacao do encoder.
        void setPosition(int delta)
        {
            int32_t target = (int32_t)elapsed + delta * (int32_t)POSITION_STEP;
            elapsed = (uint32_t)constrain(target, (int32_t)0, (int32_t)track.duration);
            Serial.printf("Posicao: %lu s\r\n", (unsigned long)elapsed);
            refreshProgress();
        }
};


Screen screen;
Library library;
Player kiMusic(screen, library);

// Retorna true se o SD iniciou e tem pelo menos um MP3 em /music.
bool initSD()
{
    // A tela e o SD dividem o barramento SPI. O TFT_eSPI ja chamou SPI.begin()
    // com os pinos do platformio.ini, entao basta manter a tela desselecionada.
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);

    if (!SD.begin(PIN_SD_CS, SPI)) {
        Serial.println("Erro ao iniciar SD! Usando musica demo.");
        return false;
    }

    Serial.println("SD iniciado!");
    Serial.printf("Capacidade do SD: %llu bytes\n", SD.cardSize());

    size_t found = library.scan(SD, "/music");
    Serial.printf("Musicas encontradas em /music: %u\r\n", (unsigned)found);
    return found > 0;
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    screen.init();
    Serial.println("KiMusic Iniciado no ESP32...");

    kiMusic.begin(initSD());
}

void loop()
{
    screen.update();
    kiMusic.control();
    delay(1);
}
