# Beyond the Fallen — Roguelike Boss Rush (projeto em C)

Descrição
--------
Beyond the Fallen é um roguelike do tipo "Boss Rush" escrito em linguagem C. O jogador enfrenta apenas chefões — sem inimigos comuns —, cada um com padrões de ataque únicos que precisam ser aprendidos e dominados (estilo Dark Souls). Partidas são curtas (até ~40 minutos). Ao morrer, o jogador recomeça do zero; entre chefes vencidos o jogador recebe upgrades temporários para a sessão. O objetivo do projeto é ter combates desafiadores, estrutura simples e alta rejogabilidade.

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
- Sistema simples de áudio (opcional) e efeitos visuais (usando raylib).
- Logs ou modo de depuração para teste de padrões dos chefes.

Regras de jogo e mecânicas (sugestão)
------------------------------------
- Vida do jogador reduzida por acertos dos chefes; se chegar a 0, reinício da sessão.
- Cada chefe possui 2–4 padrões de ataque (telegráficos) que o jogador deve identificar.
- Depois de derrotar um chefe, o jogador escolhe 1 upgrade entre 2–3 opções.
- Tempo máximo por partida: ~40 minutos (para sessões de avaliação/tempo limitado).
- Arcade: acumula pontos por chefe derrotado; após morrer, é exibido placar.
- Sequência: objetivos e desafios progressivos até o "final boss".

Controles (sugestão padrão — altere conforme a implementação)
------------------------------------------------------------
- Movimento: WASD ou setas
- Atacar: J ou tecla Z / Espaço
- Esquivar: Q ou tecla Shift
- Bloquear / Parry: L ou tecla C (opcional)
- Pausar/Menu: Esc

Observação: controles podem ser configuráveis e dependem da implementação usando raylib.

Dependências e ambiente
-----------------------
- Compilador C (GCC/Clang) com suporte a C11
- make (opcional)
- raylib (biblioteca gráfica) — instalar via gerenciador de pacotes ou compilar a partir do source
- math.h (libm) para funções matemáticas, conforme necessário

Instalação (exemplo)
--------------------
1. Instale um compilador C:
   - Linux: sudo apt install build-essential
   - macOS: Xcode Command Line Tools (xcode-select --install)
2. Instale raylib:
   - Ubuntu/Debian (pacote pode variar): sudo apt install libraylib-dev (ou siga instruções em https://www.raylib.com)
   - macOS: brew install raylib
   - Alternativa: construir raylib a partir do source conforme instruções oficiais: https://github.com/raysan5/raylib
3. Clone o repositório:
   - git clone https://github.com/caiofreitas23/jogo_pif.git
4. Entre na pasta do projeto:
   - cd jogo_pif

Observação: este repositório contém o arquivo `codigo_jogo.c` na raiz — os exemplos abaixo assumem este layout. Se `codigo_jogo.c` estiver em outro diretório (ex.: `src/`), ajuste os caminhos conforme necessário.

Compilação e execução (exemplos)
--------------------------------
Exemplo genérico com GCC (arquivo na raiz):
```bash
gcc -Wall -Wextra -O2 -std=c11 -o beyond_fallen codigo_jogo.c -lraylib -lm
./beyond_fallen
```

Usando pkg-config (mais portável):
```bash
gcc -Wall -Wextra -O2 -std=c11 $(pkg-config --cflags raylib) -o beyond_fallen codigo_jogo.c $(pkg-config --libs raylib) -lm
./beyond_fallen
```

Sugestão de Makefile (exemplo mínimo) — veja arquivo Makefile separado abaixo.

Estrutura sugerida do projeto
-----------------------------
- / (ou /src se preferir) — onde colocar `codigo_jogo.c` e outros arquivos .c/.h  
- /assets — imagens, sons, fontes (se aplicável)  
- /docs — documentação adicional / notas de design  
- Makefile  
- README.md

Checklist de desenvolvimento — O que precisa ser feito
-----------------------------------------------------
Alta prioridade
- Definir engine básica de combate (input, loop de jogo, colisões).
- Implementar sistema de entidades (player e boss).
- Criar 3 chefes iniciais com padrões distintos (tutorial + 2 chefs).
- Implementar seleção de upgrades entre chefes.
- Implementar morte e reinício de sessão (reset de progressão temporária).
- Implementar build/run simples e README (este arquivo).

Média prioridade
- Tela de menu e seleção de modo (Arcade / Sequência).
- Sistema de pontuação para Arcade.
- Salvamento local de melhores pontuações.
- Ferramentas de balanceamento (modos debug).

Baixa prioridade / melhorias
- Efeitos sonoros e música (raylib supports basic audio; considere mixers externos se precisar).
- Animações e sprites (usar formatos suportados por raylib).
- Mais chefes e variações de patterns.
- IA mais sofisticada, dificuldade adaptativa.
- Localização (i18n), telas de tutorial.

Como avaliar / instruções para professores
-----------------------------------------
Requisitos mínimos para rodar:
- GCC/Clang instalado
- raylib instalado (ou compilado) conforme o modo implementado

Para compilar e rodar:
- executar os comandos de compilação descritos acima (ou apenas `make` se houver Makefile)
- executar `./beyond_fallen` (ou o nome do executável gerado)

Dica: inclua no repositório um Makefile com targets `make` e `make run` para facilitar avaliação.  
Indique no README qualquer dependência específica e versões testadas.

Contribuição
------------
- Abra um issue descrevendo a proposta antes de implementar grandes mudanças.
- Faça branch com nome `feature/<nome>` ou `fix/<nome>`.
- Envie pull requests com descrição clara e testes (se aplicável).
- Mantenha commits pequenos e com mensagens descritivas.

Licença
-------
Escolha uma licença antes de publicar. Recomendado: MIT. Para adicionar MIT, crie um arquivo LICENSE com o texto padrão MIT e inclua seu nome/ano.

Contato
-------
- Maintainer: caiofreitas23  
- Para bugs e features: use o sistema de Issues do repositório

Notas finais
------------
Este README sintetiza a ideia do projeto e fornece um plano de implementação e avaliação. Ajuste controles, dependências e instruções de compilação conforme a arquitetura final e as bibliotecas escolhidas. Se `codigo_jogo.c` estiver em um subdiretório, atualize os comandos de compilação e o Makefile para apontar para o caminho correto.
