# Jogo PIF — Beyond the Fallen (projeto em C)
Contribuintes: Caio Freitas, Gabriel Furtado, João Pedro Guedes

Descrição
--------
Beyond the Fallen é um roguelike do tipo "Boss Rush" escrito em linguagem C. O jogador enfrenta apenas chefões — sem inimigos comuns —, cada um com padrões de ataque únicos que precisam ser aprendidos e dominados (estilo Dark Souls). Partidas são curtas (até ~40 minutos). Ao morrer, o jogador recomeça do zero; entre chefes vencidos o jogador recebe upgrades permanentes temporários para a sessão. O objetivo do projeto é ter combates desafiadores, estrutura simples e alta rejogabilidade.

Principais ideias
-----------------
- Apenas chefões (bosses) com padrões distintos.
- Progressão baseada em upgrades obtidos a cada chefe derrotado.
- Dois modos propostos:
  - Arcade: sobreviver e derrotar o máximo de chefes possível em uma única sessão.
  - Campanha/Sequência: sequência fixa/progressiva de chefes até zerar.
- Partidas curtas, foco em aprendizado de padrões e tomada de decisão.
- Rejogabilidade por design: variação de chefes, upgrades e dificuldades.

Funcionalidades previstas
-------------------------
- Sistema de combate básico (ataque, esquiva, bloqueio/opcional).
- Inteligência/padrões diferentes por chefão.
- Sistema de upgrades entre lutas (melhorias de dano, defesa, esquiva, habilidades especiais).
- Modos de jogo: Arcade e Sequência.
- Tela de menu, seleção de modo, e placar (para Arcade).
- Balanceamento e ajuste de dificuldade.
- Sistema simples de áudio (opcional) e efeitos visuais (dependendo da biblioteca escolhida).
- Logs ou modo de depuração para teste de padrões dos chefes.

Regras de jogo e mecânicas (sugestão)
------------------------------------
- Vida do jogador reduzida por acertos dos chefes; se chegar a 0, perdição e reinício.
- Cada chefe possui 2–4 padrões de ataque (telegráficos) que o jogador deve identificar.
- Depois de derrotar um chefe, o jogador escolhe 1 upgrade entre 2–3 opções.
- Tempo máximo por partida: ~40 minutos (para sessões de avaliação/tempo limitado).
- Arcade: acumula pontos por chefe derrotado; após morrer, é exibido placar.
- Sequência: objetivos e desafios progressivos até o "final boss".

Controles (sugestão padrão — altere conforme a implementação)
------------------------------------------------------------
- Movimento: WASD ou setas
- Atacar: J ou tecla Z / Espaço
- Esquivar: K ou tecla X / Shift
- Bloquear / Parry: L ou tecla C (opcional)
- Pausar/Menu: Esc
Observação: controles podem ser configuráveis e dependem da biblioteca escolhida (terminal, SDL2 etc.).

Dependências e ambiente
-----------------------
Opções (dependem da UI escolhida):
- Compilador C (GCC/Clang) com suporte a C11
- make (opcional)
- Biblioteca de terminal: ncurses (se for versão em terminal)
- Biblioteca gráfica: SDL2 (se for versão com janelas/gráficos)
- math.h (libm) para funções matemáticas, conforme necessário

Instalação (exemplo)
--------------------
1. Instale um compilador C:
   - Linux: sudo apt install build-essential
   - macOS: Xcode Command Line Tools (xcode-select --install)
2. Instale dependências opcionais:
   - ncurses (terminal): sudo apt install libncurses5-dev libncursesw5-dev
   - SDL2 (gráficos): sudo apt install libsdl2-dev
3. Clone o repositório:
   - git clone https://github.com/caiofreitas23/jogo_pif.git
4. Entre na pasta do projeto:
   - cd codigo_jogo

Compilação e execução (exemplos)
--------------------------------
Exemplo genérico com GCC (todos os .c em src/):
- gcc -Wall -Wextra -O2 -std=c11 -o jogo src/*.c -lm
- ./jogo

Se usar ncurses:
- gcc -Wall -Wextra -O2 -std=c11 -o jogo src/*.c -lncurses -lm
- ./jogo

Se usar SDL2:
- gcc -Wall -Wextra -O2 -std=c11 `sdl2-config --cflags` -o jogo src/*.c `sdl2-config --libs` -lm
- ./jogo

Sugestão de Makefile (exemplo mínimo)
- all:
-     gcc -Wall -Wextra -O2 -std=c11 -o jogo src/*.c -lncurses -lm
- run:
-     ./jogo
(Adapte para SDL2 ou outras bibliotecas conforme necessário.)

Estrutura sugerida do projeto
-----------------------------
- /src         — código-fonte (.c, .h)
- /assets      — imagens, sons, fontes (se aplicável)
- /docs        — documentação adicional / notas de design
- Makefile
- README.md

Checklist de desenvolvimento — O que precisa ser feito
-----------------------------------------------------
Alta prioridade
- Definir engine básica de combate (input, loop de jogo, colisões).
- Implementar sistema de entidades (player e boss).
- Criar 3 chefs iniciais com padrões distintos (tutorial + 2 chefs).
- Implementar seleção de upgrades entre chefes.
- Implementar morte e reinício de sessão (reset de progressão temporária).
- Implementar build/run simples e README (este arquivo).

Média prioridade
- Tela de menu e seleção de modo (Arcade / Sequência).
- Sistema de pontuação para Arcade.
- Salvamento local de melhores pontuações.
- Ferramentas de balanceamento (modos debug).

Baixa prioridade / melhorias
- Efeitos sonoros e música (SDL2_mixer ou similar).
- Animações e sprites (se usar SDL2).
- Mais chefes e variações de patterns.
- IA mais sofisticada, dificuldade adaptativa.
- Localização (i18n), telas de tutorial.

Como avaliar / instruções para professores
-----------------------------------------
- Requisitos mínimos para rodar:
  - GCC/Clang instalado
  - Dependências (ncurses ou SDL2) conforme o modo implementado
- Para compilar e rodar:
  - executar os comandos de compilação descritos acima (ou apenas `make` se houver Makefile)
  - executar `./jogo`
- Dica: inclua no repositório um script ou Makefile com targets `make` e `make run` para facilitar avaliação.
- Indique no README qualquer dependência específica e versões testadas.

Contribuição
------------
- Abra um issue descrevendo a proposta antes de implementar grandes mudanças.
- Faça branch com nome `feature/<nome>` ou `fix/<nome>`.
- Envie pull requests com descrição clara e testes (se aplicável).

Licença
-------
Escolha uma licença antes de publicar (recomendado: MIT). Se preferir, inclua um arquivo LICENSE com o texto escolhido.

Contato
-------
- Maintainer: caiofreitas23, jpgacl, gb_miller
- Email / GitHub Issues: use o sistema de Issues do repositório para reportar bugs e solicitar features.

Notas finais
------------
Este README sintetiza a ideia do projeto e fornece um plano de implementação e avaliação. Ajuste controles, dependências e instruções de compilação conforme a arquitetura final (terminal vs. janela gráfica) e as bibliotecas escolhidas.
