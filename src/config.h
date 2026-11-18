#ifndef CONFIG_H
#define CONFIG_H

// TELA
#define LARGURA_TELA 960
#define ALTURA_TELA 540

// PLAYER
#define PLAYER_VIDA 100
#define PLAYER_VELOCIDADE 5
#define PLAYER_DANO_BASE 10

#define COOLDOWN_DEFESA 3.0f
#define COOLDOWN_ESQUIVA 0.65f

// PROGRESSAO DO JOGO
#define CONTINUES_INICIAIS 3
#define PLAYER_VIDA_REVIVER (PLAYER_VIDA / 2)
#define TEMPO_TRANSICAO 10.0f

// PONTUACAO
#define PONTOS_SOCO 10
#define PONTOS_SOCO_CONTRA_ATAQUE 30   // soco com dano dobrado após esquiva perfeita
#define PONTOS_ESQUIVA 25              // esquiva perfeita

// NOCAUTE, JUIZ E REVIVER
#define NOCAUTE_DURACAO_QUEDA 1.4f      // animação de queda
#define NOCAUTE_DURACAO_JUIZ 1.0f       // juiz entrando em cena
#define NOCAUTE_DURACAO_LEVANTAR 0.7f   // animação de levantar
#define NOCAUTE_DURACAO_FIM 1.2f        // pausa no nocaute definitivo
#define NOCAUTE_CONTAGEM_JOGADOR 10     // segundos para apertar ESPAÇO
#define NOCAUTE_CONTAGEM_BOSS 5         // o chefe se levanta quando o juiz chega a 5

// BOSSES
#define BOBROVSKY_VIDA 150
#define BOBROVSKY_DANO 15
#define BOBROVSKY_VELOCIDADE 3

#define VLADIMIR_VIDA 250
#define VLADIMIR_DANO 20
#define VLADIMIR_VELOCIDADE 7

#define IVAN_VIDA 400
#define IVAN_DANO 40
#define IVAN_VELOCIDADE 1

#define ANASTASIA_VIDA 700
#define ANASTASIA_DANO 50
#define ANASTASIA_DANO_ESPECIAL 70
#define ANASTASIA_VELOCIDADE 4

// ESTADO DO COMBATE
#define COMBATE_EM_ANDAMENTO 0
#define COMBATE_VITORIA 1
#define COMBATE_DERROTA -1
#define COMBATE_EMPATE 2

#endif