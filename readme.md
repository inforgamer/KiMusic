# 🎵 KiMusic

> Um MP3 Player *bare-metal* desenvolvido do zero utilizando ESP32-S3 e C++.

O **KiMusic** é um projeto pessoal focado na construção de um MP3 Player portátil 100% offline. O projeto une engenharia de hardware e desenvolvimento de software para entregar uma experiência de áudio customizada, servindo como um laboratório prático para o estudo de sistemas embarcados.

---

## 🔗 Central do Projeto (Notion)

Para manter este repositório focado no código-fonte, todo o gerenciamento de tarefas, arquitetura e documentação técnica estão hospedados publicamente no Notion. 

Acesse os links abaixo para entender os bastidores do projeto:

* 📊 **[Roadmap e Kanban](https://lavender-net-1b7.notion.site/3c1461dfd75280fab027f1dc8ac1a591?v=3c1461dfd75280969407000c7bf4fc61&source=copy_link)**: Acompanhe em tempo real o status das tarefas, o que está em desenvolvimento e os próximos passos.
* 📖 **[Documentação Oficial](https://lavender-net-1b7.notion.site/3c1461dfd752801bbebfe8c226dcdbdd?v=3c1461dfd752803693c8000c37c181f4&source=copy_link)**: Acesse o PRD (Visão Geral), decisões de arquitetura, lista completa de hardware e guias de compilação.

---

## 💻 Tecnologias e Hardware Base

O coração do projeto roda em **C++**, focando em controle de baixo nível e performance.

**Stack e Ferramentas:**
* C++ / Arduino framework
* PlatformIO
* LVGL 8.3 + SquareLine Studio (Interface)
* TFT_eSPI (Driver do display)
* ESP8266Audio (Decodificador MP3 + saída I2S)
* JPEGDEC e PNGdec (Capas do álbum)
* VS Code & Wokwi (Simulação)
* Git / GitHub

**Hardware Principal:**
* Microcontrolador: **ESP32-S3 (DevKitC-1)**
* Interface Visual: **Display ILI9341 (320x240)**
* Navegação: **Rotary Encoder + 4 botões**
* Armazenamento: **Módulo de Cartão SD**
* Saída: **DAC PCM5102A (I2S) e conexão P2**

A pinagem completa está em [`KiMusic/Documentação/Pinagem.md`](KiMusic/Documentação/Pinagem.md).

---

## 🎮 Controles

| Botão | Menu fechado | Menu aberto |
| :--- | :--- | :--- |
| **Encoder (girar)** | Volume ou posição, conforme o modo | Move a seleção (ou ajusta o brilho) |
| **Central (clique)** | Play / Pause | Escolhe o item |
| **Central (toque duplo)** | Abre o menu (não pausa nem dá Play) | Fecha o menu |
| **Central (segurar 2 s, desligado)** | Liga | — |
| **Vol** (cima) | Liga/desliga o modo Volume (mostra a barra) | Sobe (no brilho: aumenta) |
| **Pos** (baixo) | Liga/desliga o modo Posição (5 s por passo) | Desce (no brilho: diminui) |
| **Prev** (esquerda) | Volta ao início da música; se ela acabou de começar, vai para a anterior | Volta uma página |
| **Next** (direita) | Próxima música | — |

### Menu

```
Menu
 ├─ Músicas ................. músicas da lista atual; escolher uma começa a tocá-la (+ Voltar)
 ├─ Playlists ............... Favoritas, Todas as músicas, playlists .m3u do SD, Voltar
 ├─ Aleatório: <modo> ....... Desligado → Playlist → Geral
 ├─ Repetir: <modo> ......... Desligado → Todas → Uma
 ├─ Favoritar música ........ adiciona a música atual na playlist "Favoritas"
 ├─ Configurações
 │   ├─ Brilho: NN% ......... clique, gire para ajustar (10% a 100%), clique para salvar
 │   ├─ Desligar tela ....... apaga só a tela; a música continua (qualquer botão acende)
 │   ├─ Desligar ............ desliga o aparelho (para ligar: segurar o central 2 s)
 │   └─ Voltar
 └─ Voltar .................. fecha o menu
```

* **Aleatório Playlist:** embaralha a lista atual. **Geral:** passa para todas as músicas do SD e embaralha.
* **Repetir Desligado:** no fim da lista volta para a primeira música e pausa. **Todas:** continua tocando. **Uma:** repete a atual.
* Brilho, volume e Repetir ficam salvos e voltam iguais ao ligar.
* **Capas:** JPEG (comum ou progressivo) e PNG, embutidas no MP3 ou em `/cover/<nome da música>.jpg|.jpeg|.png`. JPEG progressivo aparece mais quadriculado; PNG entrelaçado, de 16 bits ou com mais de 1500 px de largura não aparece.
* **Limite:** até 200 músicas por lista e 50 playlists.

### Cartão SD

```
/music/*.mp3                    músicas (ordem alfabética, até 200)
/cover/<nome da música>.jpg     capa (.jpg, .jpeg ou .png), se o MP3 não tiver uma embutida
/playlists/<nome>.m3u           playlists (uma música por linha)
/playlists/Favoritas.m3u        criada pelo "Favoritar música"
```

Exemplo de playlist (`/playlists/Treino.m3u`):

```
#EXTM3U
/music/SoundHelix-Song-1.mp3
SoundHelix-Song-3.mp3
```

Linhas com `#` são ignoradas; um nome sem pasta é procurado em `/music`.

---

## 🗂️ Estrutura do Código

```
KiMusic/
├─ src/
│  ├─ main.cpp      setup()/loop(): cria e liga os módulos
│  ├─ App           lê os botões e chama Player/Menu/Device
│  ├─ Config.hpp    pinagem e constantes
│  ├─ core/         regras do player e do menu (sem Arduino, testadas no PC)
│  ├─ input/        Encoder, Button (clique e pressão longa)
│  ├─ storage/      TrackList, SdCatalog (músicas e playlists do SD), DemoSource
│  ├─ system/       AudioEngine (MP3 → PCM5102A), Device (brilho/ajustes), Power
│  ├─ view/         Screen (LVGL) e ScreenView (liga o núcleo à tela)
│  └─ UI/           gerado pelo SquareLine Studio — não editar à mão
├─ lib/
│  ├─ media/        Mp3Info (tags/duração) e CoverDecoder (capa JPEG)
└─ test/host/       testes no PC
```

Fluxo: **botões → App → Player/Menu → ScreenView → Screen** e **Player → AudioEngine → DAC**

### Simulação no Wokwi

O `wokwi.toml` copia a pasta `KiMusic/wokwi-sd/` para o cartão SD simulado. Ela é gerada a cada build (`tools/wokwi_sd.py`) a partir do `SD/`: cada MP3 fica com a tag inteira (título e capa) e só os primeiros 300 KB de áudio, porque o Wokwi não toca som e o `SD/` inteiro estoura a memória do simulador. Para testar uma música nova no Wokwi, ponha no `SD/` e compile. Músicas sem cabeçalho Xing/TLEN aparecem com duração curta por causa do corte.

### Testes no PC

Precisa do `g++` (MinGW-w64). Na pasta `KiMusic`:

```powershell
powershell -File test/host/run_tests.ps1
```

---

## 📌 Foco Atual de Desenvolvimento

> **Objetivo da Sprint:** Fazer o display funcionar perfeitamente e implementar a lógica matemática de navegação do menu principal e avanço de tempo utilizando o Rotary Encoder em C++. 

---

## 👤 Autor

**Hugo "Infor" Marcondes**
[GitHub](https://github.com/inforgamer)
