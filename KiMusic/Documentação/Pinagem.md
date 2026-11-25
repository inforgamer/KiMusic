# Mapeamento de Pinos - ESP32-S3

## 📺 1. Tela ILI9341 (Display)

| Pino da Tela | Pino no ESP32-S3 |
| :--- | :--- |
| **CS** | 10 |
| **RST** | 14 |
| **D/C** | 9 |
| **MOSI** | 11 |
| **SCK** | 12 |
| **MISO** | 13 |
| **LED** (luz de fundo) | 21 (PWM = brilho) |
| **VCC** | 3V3 |
| **GND** | GND |

> **Brilho:** o pino LED agora vai no GPIO 21, controlado por PWM (antes ia direto no 3V3 e não dava para ajustar). Os módulos ILI9341 comuns (placa vermelha) já têm um transistor no pino LED, então o GPIO só fornece o sinal. Se o seu módulo não tiver (o LED puxaria mais de 20 mA do pino), use um transistor NPN (ex.: BC547 + resistor de 1 kΩ na base) entre o GPIO 21 e o LED.

---

## 💾 2. Leitor MicroSD Card

> **Nota Técnica:** O barramento SPI (MOSI e SCK) é compartilhado com a tela para otimizar o uso do microcontrolador. O pino CS é o que define quem está "falando" no momento.

| Pino do Leitor SD | Pino no ESP32-S3 |
| :--- | :--- |
| **CS** | 15 |
| **SCK** | 12 |
| **DI (MOSI)** | 11 |
| **DO (MISO)** | 13 |
| **VCC** | 3V3 |
| **GND** | GND |

---

## 🔄 3. Encoder Rotativo (KY-040)

| Pino do Encoder | Pino no ESP32-S3 |
| :--- | :--- |
| **CLK** (A) | 6 |
| **DT** (B) | 7 |
| **SW** (Clique central) | 5 |
| **VCC** | 3V3 |
| **GND** | GND |

---

## 🎛️ 4. Os 4 Botões (Cruz Direcional)

| Botão | Função Planejada | Pino no ESP32-S3 |
| :--- | :--- | :--- |
| **Botão Cima** (Vermelho) | Volume | 16 |
| **Botão Baixo** (Azul) | Posição da Música | 17 |
| **Botão Esquerda** (Verde) | Retroceder (Prev) | 18 |
| **Botão Direita** (Amarelo) | Avançar (Next) | 8 |

---

## 🎧 5. DAC PCM5102A (saída de áudio I2S → P2)

| Pino do PCM5102A | Ligação |
| :--- | :--- |
| **VIN** | 3V3 (ou 5V, o módulo tem regulador) |
| **GND** | GND |
| **BCK** | GPIO 40 |
| **LCK** (LRCK / WS) | GPIO 41 |
| **DIN** | GPIO 42 |
| **XSMT** (mudo) | GPIO 39 (HIGH = som, LOW = mudo) |
| **SCK** | GND (usa o PLL interno do DAC) |
| **FMT** | GND (formato I2S) |
| **FLT** | GND (filtro normal) |
| **DEMP** | GND (sem de-ênfase) |
| **L / G / R** (saída) | conector P2 (esquerdo / terra / direito) |

> **Módulo roxo com jumpers:** em muitos módulos SCK, FMT, FLT e DEMP já são configurados pelos jumpers de solda da parte de trás (H1L, H2L, H3L, H4L). Nesse caso, feche-os para o lado "L" (baixo) e ligue só VIN, GND, BCK, LCK, DIN e XSMT.
>
> O Wokwi não tem o PCM5102A: na simulação o firmware funciona normalmente, mas sem som.

---

## ⏻ 6. Ligar / Desligar

Não há botão de energia separado.

| Ação | Resultado |
| :--- | :--- |
| **Menu → Configurações → Desligar** | Salva os ajustes e desliga (deep sleep) |
| **Menu → Configurações → Desligar tela** | Apaga só a tela; a música continua. Qualquer botão acende de novo |
| Segurar o **botão central (GPIO 5) por 2 s** com o aparelho desligado | Liga |
| Ligar a alimentação ou apertar RESET | Liga direto |

> **Consumo:** no deep sleep o ESP32-S3 gasta poucos microampères, mas o display, o SD e o DAC continuam ligados no 3V3. Para a bateria durar com o aparelho "desligado", ligue o 3V3 desses módulos através de uma chave de carga (MOSFET canal P, ex.: AO3401) comandada por um GPIO livre — o firmware pode ser adaptado para cortar essa chave antes de dormir.

---

## 🔋 7. Bateria e USB-C (carga + dados)

| Ligação | Pino no ESP32-S3 |
| :--- | :--- |
| **USB-C D−** | GPIO 19 (USB nativa, fixo) |
| **USB-C D+** | GPIO 20 (USB nativa, fixo) |
| **USB-C VBUS (5 V)** → divisor 100 kΩ / 100 kΩ | GPIO 2 (HIGH = cabo ligado) |
| **Bateria (+)** → divisor 100 kΩ / 100 kΩ | GPIO 1 (ADC: lê metade da tensão) |
| **USB-C CC1 e CC2** | 5,1 kΩ cada para o GND (sem isso, carregador USB-C não libera os 5 V) |
| **USB-C VBUS** | entrada do carregador da bateria |

> **Carregador:** o TP4056 carrega, mas o player não roda direito ligado na carga ao mesmo tempo. Um CI com caminho de energia (power-path, ex.: IP5306 ou BQ24074) deixa tocar música com o cabo ligado.
>
> **Proteção:** use uma bateria com placa de proteção (corta em ~3 V) e, de preferência, um ESD nas linhas de dados (ex.: USBLC6-2).
>
> **Na DevKitC de testes:** a porta "USB" da placa já é a USB nativa (GPIO 19/20), então o Modo USB funciona nela. Sem os divisores nos GPIO 1 e 2, a bateria aparece como 0% e o cabo como desligado.

---

## 📌 GPIOs livres

4, 38, 47 e 48 (o 48 é o LED RGB da DevKitC). Evite 0, 3, 45 e 46 (pinos de boot).