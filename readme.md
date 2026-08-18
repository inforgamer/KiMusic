# 🎵 KiMusic

> Um MP3 Player customizado desenvolvido do zero utilizando Raspberry Pi Pico.

O **KiMusic** é um projeto pessoal focado na construção de um MP3 Player portátil do zero, combinando **programação embarcada, eletrônica, interface, áudio e design físico**.

O projeto começou como um experimento utilizando **MicroPython** e atualmente está sendo desenvolvido em **C++**, buscando maior controle sobre o hardware e uma compreensão mais aprofundada de sistemas embarcados.

---

## 🚧 Status do Projeto

**Etapa atual: Desenvolvimento inicial**

Neste momento, o projeto está focado na construção das bases do sistema:

- Programação embarcada
- Comunicação entre componentes
- Interface gráfica
- Controle por encoder
- Arquitetura de áudio
- Gerenciamento de energia

O hardware está sendo desenvolvido gradualmente, utilizando simulações e protótipos antes da montagem física definitiva.

---

## 🧩 Estrutura do Projeto

O desenvolvimento do KiMusic foi dividido em cinco áreas principais.

### 🎨 1. Design Industrial

Responsável pelo formato físico do aparelho, posicionamento dos componentes e experiência de utilização.

**Concluído**
- Primeiro esboço físico
- Conceito inicial do dispositivo

**Próximos passos**
- Modelagem 3D da carcaça
- Definição do posicionamento dos componentes
- Desenvolvimento de uma estrutura para impressão 3D

---

### ⚡ 2. Eletrônica

Responsável pela comunicação entre os componentes e pelo funcionamento eletrônico do dispositivo.

**Concluído**
- Mapeamento inicial dos GPIOs
- Planejamento dos barramentos SPI
- Simulação inicial no Wokwi
- Separação da comunicação entre display e cartão SD

**Próximos passos**
- Esquemático definitivo
- Protótipo físico
- Projeto de PCB

---

### 💻 3. Programação Embarcada

Responsável pela lógica e pelo funcionamento do MP3 Player.

**Concluído**
- Configuração do ambiente de desenvolvimento
- Estudos iniciais de GPIO
- Primeiro protótipo desenvolvido em MicroPython
- Início da migração para C++

**Próximos passos**
- Interface do display ILI9341
- Sistema de menus
- Máquina de estados
- Controle pelo Rotary Encoder
- Leitura do cartão SD
- Sistema de reprodução de músicas

---

### 🎧 4. Áudio

Responsável pelo processamento e reprodução das músicas.

**Concluído**
- Pesquisa e planejamento inicial da arquitetura de áudio

**Próximos passos**
- Estudo da decodificação de áudio
- Integração de um DAC
- Desenvolvimento da saída para fones de ouvido
- Implementação do sistema de reprodução

---

### 🔋 5. Energia

Responsável pelo sistema de alimentação do dispositivo.

**Concluído**
- Planejamento inicial

**Próximos passos**
- Estimativa do consumo energético
- Escolha da bateria
- Circuito de carregamento
- Sistema de gerenciamento de energia

---

## 🛠️ Hardware

| Componente | Função |
|---|---|
| Raspberry Pi Pico | Microcontrolador principal |
| ILI9341 | Display |
| Rotary Encoder | Controle do usuário |
| Cartão SD | Armazenamento das músicas |
| DAC | Conversão de áudio |
| Bateria Li-ion | Alimentação portátil |

> Alguns componentes ainda estão em fase de pesquisa e podem ser alterados durante o desenvolvimento.

---

## 💻 Tecnologias

### Programação

- **C++** — desenvolvimento atual
- **MicroPython** — utilizado no protótipo inicial

### Ferramentas

- VS Code
- Wokwi
- Git / GitHub

### Hardware

- Raspberry Pi Pico
- ILI9341
- Rotary Encoder
- Cartão SD
- DAC

---

## 🗺️ Roadmap

- [x] Conceito inicial do projeto
- [x] Primeiros esboços físicos
- [x] Mapeamento inicial dos GPIOs
- [x] Planejamento da comunicação SPI
- [x] Configuração da simulação no Wokwi
- [x] Primeiro protótipo em MicroPython
- [ ] Estrutura inicial em C++
- [ ] Interface do ILI9341
- [ ] Navegação pelo Rotary Encoder
- [ ] Integração do cartão SD
- [ ] Sistema de reprodução de áudio
- [ ] Integração do DAC
- [ ] Sistema de bateria
- [ ] Primeiro protótipo físico
- [ ] Desenvolvimento da carcaça
- [ ] MP3 Player funcional

---

## 📚 Documentação

A documentação técnica detalhada, incluindo especificações, planejamento e decisões do projeto, está disponível em:

📄 **[KiMusic.pdf](Documentação/KiMusic.pdf)**

---

## 🎯 Objetivo

O objetivo final é construir um **MP3 Player portátil totalmente funcional**, desenvolvido do zero, utilizando o projeto como uma forma prática de estudar:

**Sistemas Embarcados → Eletrônica → C++ → Áudio → Desenvolvimento de Hardware**

Mais do que construir um MP3 Player, o KiMusic é uma oportunidade para entender **como software e hardware trabalham juntos para transformar um projeto em um dispositivo funcional.**

---

## 📌 Foco Atual

> **Fazer o display funcionar e implementar a navegação do menu principal utilizando o Rotary Encoder.**

Esse é o próximo marco do projeto antes de avançar para a montagem do hardware físico.

---

## 👤 Autor

**Hugo "Infor" Marcondes**
[GitHub](https://github.com/inforgamer)
