# KiMusic - Projeto do MP3 Player Customizado

> ⚠️ **Status: Fase de Planejamento (Menos de 1% concluído)**
> O projeto começou agora. O objetivo deste repositório é centralizar a documentação, os estudos iniciais de código e os esboços físicos antes da montagem oficial. 

O **KiMusic** é um projeto pessoal de construção de um MP3 Player do zero utilizando o Raspberry Pi Pico. Para gerenciar a complexidade, o desenvolvimento do hardware e do software foi dividido em um plano de 5 áreas fundamentais. 

A documentação detalhada, contendo todas as especificações e regras do projeto, pode ser encontrada no arquivo oficial **KiMusic.pdf** (disponível na pasta de Documentação).

---

## 🗺️ O Plano de Ação (As 5 Áreas do Projeto)

O desenvolvimento vai atacar as seguintes frentes, passo a passo:

### 1. Design Industrial
Onde a estética encontra a ergonomia. Esta área cuida do visual do aparelho, onde os botões vão ficar e como será a "pegada" física do MP3.
*   **Feito:** Primeiro esboço de papel/conceito inicial (disponível na pasta de imagens).
*   **Próximos passos:** Modelagem 3D da carcaça e suporte para impressão.

### 2. Eletrônica
O cérebro e as conexões lógicas. Envolve a comunicação entre os módulos sem que um atropele o outro.
*   **Feito:** Mapeamento inicial dos pinos e separação dos barramentos SPI (Tela vs. Cartão SD) no simulador Wokwi.
*   **Próximos passos:** Desenhar o esquemático final e, futuramente, rotear uma placa de circuito impresso (PCB).

### 3. Programação
A alma do projeto . Aqui entra a lógica do sistema operacional do MP3.
*   **Feito:** Configuração do ambiente local no VS Code e estudos iniciais de leitura de pinos.
*   **Próximos passos:** Dominar a renderização de interface no display ILI9341, criar a máquina de estados do menu e implementar a leitura de arquivos `.txt` do cartão SD.

### 4. Som
O coração de um MP3 Player. O processamento e a saída das músicas.
*   **Feito:** Planejamento teórico.
*   **Próximos passos:** Estudar a decodificação de áudio pelo Raspberry Pi Pico, integrar um DAC (Conversor Digital-Analógico) e planejar a saída (Jack P2 para fones de ouvido).

### 5. Elétrica
A energia que mantém tudo vivo.
*   **Feito:** Planejamento teórico.
*   **Próximos passos:** Dimensionar o consumo de energia da tela e do Pico, escolher o circuito de carga e definir a capacidade da bateria de lítio.

---

## 🔬 Onde estamos agora?
Atualmente focando nos estudos base de **Programação** e **Eletrônica** através de simulação no VS Code. O objetivo de curto prazo é fazer a tela ligar e o menu principal ser navegado de forma fluida usando um encoder rotativo físico/virtual, antes de comprar ou soldar qualquer componente definitivo.
