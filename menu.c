#include <raylib.h>
#include <stdio.h>

#define LARGURA_TELA 800
#define ALTURA_TELA 450

typedef struct {
    Rectangle rect;
    Color cor;
    int vida;
    int vida_max;
    float velocidade;
    int esta_atacando;
    float tempo_ataque;
    Rectangle area_ataque;
} Personagem;

int main(void) {
    // Inicialização da Janela
    InitWindow(LARGURA_TELA, ALTURA_TELA, "Batalha Gráfica contra o Chefão - Raylib");
    SetTargetFPS(60);

    // Jogador (Quadrado Azul)
    Personagem jogador = {
        .rect = { 100, 300, 40, 60 },
        .cor = BLUE,
        .vida = 100,
        .vida_max = 100,
        .velocidade = 4.0f,
        .esta_atacando = 0,
        .tempo_ataque = 0.0f
    };

    // Chefão (Quadrado Vermelho Grande)
    Personagem chefao = {
        .rect = { 600, 260, 80, 100 },
        .cor = RED,
        .vida = 300,
        .vida_max = 300,
        .velocidade = 1.5f,
        .esta_atacando = 0,
        .tempo_ataque = 0.0f
    };

    int jogo_acabou = 0;
    int vitoria = 0;

    // Loop Principal do Jogo
    while (!WindowShouldClose()) {
        // -------------------------------------------------------------
        // 1. ATUALIZAÇÃO / LÓGICA DO JOGO
        // -------------------------------------------------------------
        if (!jogo_acabou) {
            // Movimentação do Jogador (A, D ou Setas)
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  jogador.rect.x -= jogador.velocidade;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) jogador.rect.x += jogador.velocidade;

            // Limites da tela para o jogador
            if (jogador.rect.x < 0) jogador.rect.x = 0;
            if (jogador.rect.x > LARGURA_TELA - jogador.rect.width) jogador.rect.x = LARGURA_TELA - jogador.rect.width;

            // Ataque do Jogador (Tecla ESPAÇO)
            if (IsKeyPressed(KEY_SPACE) && !jogador.esta_atacando) {
                jogador.esta_atacando = 1;
                jogador.tempo_ataque = 0.2f; // Duração do golpe em segundos

                // Define a caixa de colisão do alcance do ataque à direita do jogador
                jogador.area_ataque = (Rectangle){
                    jogador.rect.x + jogador.rect.width,
                    jogador.rect.y + 10,
                    50,
                    40
                };

                // Verifica se o ataque atingiu o Chefão
                if (CheckCollisionRecs(jogador.area_ataque, chefao.rect)) {
                    chefao.vida -= 15;
                    if (chefao.vida <= 0) {
                        chefao.vida = 0;
                        jogo_acabou = 1;
                        vitoria = 1;
                    }
                }
            }

            // Timer do ataque do jogador
            if (jogador.esta_atacando) {
                jogador.tempo_ataque -= GetFrameTime();
                if (jogador.tempo_ataque <= 0) {
                    jogador.esta_atacando = 0;
                }
            }

            // IA do Chefão: Persegue o jogador
            if (chefao.rect.x > jogador.rect.x + jogador.rect.width + 10) {
                chefao.rect.x -= chefao.velocidade;
            } else if (chefao.rect.x < jogador.rect.x - chefao.rect.width - 10) {
                chefao.rect.x += chefao.velocidade;
            } else {
                // Se estiver perto, o chefão ataca periodicamente
                if (!chefao.esta_atacando && GetRandomValue(1, 40) == 1) {
                    chefao.esta_atacando = 1;
                    chefao.tempo_ataque = 0.3f;

                    // O chefão causa dano direto se estiver colidindo com a área de alcance
                    jogador.vida -= 20;
                    if (jogador.vida <= 0) {
                        jogador.vida = 0;
                        jogo_acabou = 1;
                        vitoria = 0;
                    }
                }
            }

            // Timer do ataque do chefão
            if (chefao.esta_atacando) {
                chefao.tempo_ataque -= GetFrameTime();
                if (chefao.tempo_ataque <= 0) {
                    chefao.esta_atacando = 0;
                }
            }
        }

        // -------------------------------------------------------------
        // 2. DESENHO / RENDERIZAÇÃO
        // -------------------------------------------------------------
        BeginDrawing();
            ClearBackground(RAYWHITE);

            // Desenha o Chão
            DrawRectangle(0, 360, LARGURA_TELA, 90, DARKGRAY);

            if (!jogo_acabou) {
                // Desenha o Jogador
                DrawRectangleRec(jogador.rect, jogador.cor);
                
                // Animação visual do ataque do Jogador (Caixa amarela)
                if (jogador.esta_atacando) {
                    DrawRectangleRec(jogador.area_ataque, YELLOW);
                }

                // Desenha o Chefão (Fica alaranjado ao atacar)
                DrawRectangleRec(chefao.rect, chefao.esta_atacando ? ORANGE : chefao.cor);

                // --- BARRAS DE VIDA ---
                // Barra de vida do Jogador
                DrawText("JOGADOR", 20, 20, 20, BLACK);
                DrawRectangle(20, 45, 200, 20, LIGHTGRAY);
                DrawRectangle(20, 45, (int)(200.0f * ((float)jogador.vida / jogador.vida_max)), 20, GREEN);
                DrawRectangleLines(20, 45, 200, 20, BLACK);

                // Barra de vida do Chefão
                DrawText("CHEFÃO", LARGURA_TELA - 220, 20, 20, BLACK);
                DrawRectangle(LARGURA_TELA - 220, 45, 200, 20, LIGHTGRAY);
                DrawRectangle(LARGURA_TELA - 220, 45, (int)(200.0f * ((float)chefao.vida / chefao.vida_max)), 20, RED);
                DrawRectangleLines(LARGURA_TELA - 220, 45, 200, 20, BLACK);

                // Instruções na tela
                DrawText("Controles: [A]/[D] Mover | [ESPAÇO] Atacar", 240, 410, 18, WHITE);
            } else {
                // Tela de Fim de Jogo
                if (vitoria) {
                    DrawText("VITÓRIA! O CHEFÃO FOI DERROTADO!", 130, 180, 30, GREEN);
                } else {
                    DrawText("GAME OVER! VOCÊ FOI DERROTADO!", 140, 180, 30, RED);
                }
                DrawText("Pressione [ESC] para sair.", 280, 240, 20, DARKGRAY);
            }

        EndDrawing();
    }

    // Fechamento
    CloseWindow();
    return 0;
}