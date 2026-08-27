# Mapeamento de Pinos - ESP32-S3

## 📺 1. Tela ILI9341 (Display)

| Pino da Tela | Pino no ESP32-S3 |
| :--- | :--- |
| **CS** | 10 |
| **RST** | 14 |
| **D/C** | 9 |
| **MOSI** | 11 |
| **SCK** | 12 |
| **VCC / LED** | 3V3 |
| **GND** | GND |

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

## 🔊 4. Buzzer

| Pino do Buzzer | Pino no ESP32-S3 |
| :--- | :--- |
| **Pino Positivo (+)** | 4 |
| **Pino Negativo (-)** | GND |

---

## 🎛️ 5. Os 4 Botões (Cruz Direcional)

| Botão | Função Planejada | Pino no ESP32-S3 |
| :--- | :--- | :--- |
| **Botão Cima** (Vermelho) | Volume | 16 |
| **Botão Baixo** (Azul) | Posição da Música | 17 |
| **Botão Esquerda** (Verde) | Retroceder (Prev) | 18 |
| **Botão Direita** (Amarelo) | Avançar (Next) | 8 |