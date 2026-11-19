# Beyond the Fallen

**Beyond the Fallen** é um jogo de ação e combate desenvolvido em C como projeto da matéria de Programação Imperativa e Funcional (PIF), da CESAR School. A aventura se passa em um gulag cercado pela neve, onde um prisioneiro precisa sobreviver aos confrontos, aprender habilidades e encontrar uma saída.

> **Status:** conceito pronto e desenvolvimento quase completo.

## História

Você é um prisioneiro soviético em um gulag. Para conquistar a liberdade, terá de enfrentar outros detentos — cada um com sua própria história, personalidade e estilo de luta.

Será que você conseguirá enfrentar os duros desafios que a gulag apresenta? E que surpresas o confronto final pode te apresentar?

A campanha apresenta confrontos contra chefes com características e padrões de combate próprios. Entre os adversários planejados estão:

- **Bobrovsky** — corpulento, preso por trair a pátria. Sua força e seu tamanho fazem dele um adversário difícil de derrubar.
- **Vladimir** — ágil e habilidoso. Apesar de rápido, o medo pode atrapalhar suas decisões.
- **Ivan** — guarda da prisão com muita força física, mas lento. Seus golpes são perigosos, porém podem ser evitados com atenção.
- **Anastasia** — o desafio final. O nome pode sugerir uma criança de oito anos, mas...

Ao vencer os desafios, o jogador vencerá o jogo.

## Controles
|Ação |Botão|
|:---:|:---:|
|Ataque|J/K|
|Defesa|W|
|Esquiva|A/D|
|Navegar (menu)|setas|
|Ok/Confirmar|Enter|

## Funcionalidades do jogo
* Janela gráfica 960×540 a 60 FPS desenvolvida em C com a biblioteca Raylib 
* Troca de telas completa (MENU → TRANSIÇÃO → COMBATE → FINAL / GAME OVER → NOME → PLACAR)
* 4 chefes progressivos com atributos, visuais e padrões próprios: Bobrovsky, Vladimir, Ivan e Anastasia 
* Sistema de combate em tempo real dos chefes (guarda, preparação de golpe e recuperação)
* Mecânicas de ataque (J/K), defesa (S), esquiva direcional (A/D) e contra-ataque com dano dobrado após esquiva perfeita
* Sequência animada de nocaute com entrada do juiz, contagem regressiva e chance de levantar uma vez por luta
* Matriz de estatísticas (4 × 3) que contabiliza socos, contra-ataques e esquivas por chefe
* Ranking ordenado em lista encadeada com arquivo de texto

 


A direção visual proposta é simples e fria, com neve, pedra e estruturas austeras. A batalha final terá uma arena de pedra marcada por tremores, reforçando a força do confronto.

Cada chefe exige atenção aos seus movimentos e ao momento certo para atacar ou se defender.

## Destaques

- Ambientação inspirada em um gulag da URSS;
- Chefes com histórias, personalidades e estilos de luta diferentes;
- Progressão com novas habilidades e armas;
- Exploração de um mapa fechado entre os combates;
- Mistérios espalhados pelo gulag;
- Diálogos com humor e uma surpresa no confronto final.

## Tecnologias

- **Linguagem:** C

## Estrutura do projeto
```
Jogo_Pif/
├── /src                           # Todos os arquivos principais do projeto
│   ├── boss.c/h                   # Chefões
│   ├── combat.c/h                 # Mecânicas da gameplay
│   ├── config.h                   # Todas as configurações do jogo
│   ├── estatisticas.c/h           # Tela de estatísticas gerais
|   ├── main.c                     # Estrutura principal do codigo e esqueleto do jogo
|   ├── placar.c/h                 # Tela de ranking de pontuação
|   ├── player.c/h                 # Configurações do jogador
|   └── recordes.txt               # Arquivo onde os recordes são salvos

```
## Como compilar e executar

### Pré-requisitos

- Compilador C: GCC ou Clang
- Raylib instalada
- `pkg-config` no Linux e macOS

Execute os comandos a partir da pasta do projeto, pois o jogo utiliza caminhos relativos para carregar recursos e arquivos.

Clone o repositório:

```bash
git clone https://github.com/caiofreitas23/jogo_pif.git
cd jogo_pif
```

### Linux (Ubuntu/Debian)

Instale o compilador e as dependências:

```bash
sudo apt install build-essential pkg-config libraylib-dev
```

Compile e execute:

```bash
gcc -Wall -Wextra -Isrc src/*.c -o jogo $(pkg-config --cflags --libs raylib)
./jogo
```

### macOS

Instale o compilador e a Raylib:

```bash
brew install raylib pkg-config
```

Compile e execute:

```bash
gcc -Wall -Wextra -Isrc src/*.c -o jogo $(pkg-config --cflags --libs raylib)
./jogo
```

### Windows

Instale o GCC, por exemplo pelo MinGW-w64, e a Raylib. Execute os comandos a partir da pasta do projeto.

Se a Raylib estiver em `C:\raylib` e não estiver configurada nos caminhos padrão do GCC:

```bash
gcc src/*.c -o jogo.exe -I"C:\raylib\include" -L"C:\raylib\lib" -lraylib -lopengl32 -lgdi32 -lwinmm && .\jogo.exe
```

Caso o GCC já esteja configurado para encontrar a Raylib:

```bash
gcc src/*.c -o jogo.exe -lraylib -lopengl32 -lgdi32 -lwinmm && .\jogo.exe
```


## Desenvolvimento

Este projeto está na fase final de desenvolvimento.

## Autoria

Caio Freitas de Andrade Medeiros

João Pedro Guedes Alcoforado Carneiro Leão

Gabriel Furtado Corrêa Miller
