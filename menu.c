#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

#define LARGURA_TELA 900
#define ALTURA_TELA 500
#define CHAO_Y 400
#define GRAVIDADE 1200.0f
#define FORCA_PULO -550.0f
#define MAX_PARTICULAS 50

typedef struct {
    Vector2 posicao;
    Vector2 velocidade;
    Color cor;
    bool ativa;
    float vida_util;
} Particula;

typedef struct {
    Rectangle rect;
    Vector2 velocidade;
    Color cor;
    int vida;
    int vida_max;
    float velocidade_mov;
    bool esta_no_chao;
    
    // Direção: 1 = Direita, -1 = Esquerda
    int direcao; 

    // Ataque
    bool esta_atacando;
    float tempo_ataque;
    Rectangle area_ataque;
    
    // Efeitos visuais
    float tempo_dano;
} Personagem;

// Sistema de partículas para impactos
Particula particulas[MAX_PARTICULAS] = { 0 };

void GerarParticulas(Vector2 pos, Color cor, int quantidade) {
    for (int i = 0; i < quantidade; i++) {
        for (int j = 0; j < MAX_PARTICULAS; j++) {
            if (!particulas[j].ativa) {
                particulas[j].posicao = pos;
                particulas[j].velocidade = (Vector2){ (float)GetRandomValue(-200, 200), (float)GetRandomValue(-300, 100) };
                particulas[j].cor = cor;
                particulas[j].ativa = true;
                particulas[j].vida_util = 0.4f;
                break;
            }
        }
    }
}

void AtualizarParticulas(float dt) {
    for (int i = 0; i < MAX_PARTICULAS; i++) {
        if (particulas[i].ativa) {
            particulas[i].posicao.x += particulas[i].velocidade.x * dt;
            particulas[i].posicao.y += particulas[i].velocidade.y * dt;
            particulas[i].velocidade.y += GRAVIDADE * 0.5f * dt;
            particulas[i].vida_util -= dt;
            if (particulas[i].vida_util <= 0) particulas[i].ativa = false;
        }
    }
}

void DesenharParticulas(void) {
    for (int i = 0; i < MAX_PARTICULAS; i++) {
        if (particulas[i].ativa) {
            DrawRectangleV(particulas[i].posicao, (Vector2){ 5, 5 }, particulas[i].cor);
        }
    }
}

int main(void) {
    InitWindow(LARGURA_TELA, ALTURA_TELA, "Boss Fight 2D - Raylib Engine");
    SetTargetFPS(60);

    // Jogador (Guerreiro Azul)
    Personagem jogador = {
        .rect = { 150, CHAO_Y - 70, 45, 70 },
        .velocidade = { 0, 0 },
        .cor = (Color){ 41, 128, 185, 255 },
        .vida = 100,
        .vida_max = 100,
        .velocidade_mov = 320.0f,
        .esta_no_chao = true,
        .direcao = 1,
        .esta_atacando = false,
        .tempo_ataque = 0.0f,
        .tempo_dano = 0.0f
    };

    // Chefão (Titã Vermelho)
    Personagem chefao = {
        .rect = { 650, CHAO_Y - 110, 85, 110 },
        .velocidade = { 0, 0 },
        .cor = (Color){ 192, 57, 43, 255 },
        .vida = 350,
        .vida_max = 350,
        .velocidade_mov = 160.0f,
        .esta_no_chao = true,
        .direcao = -1,
        .esta_atacando = false,
        .tempo_ataque = 0.0f,
        .tempo_dano = 0.0f
    };

    float tremor_tela = 0.0f;
    bool jogo_acabou = false;
    bool vitoria = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // -------------------------------------------------------------
        // 1. LÓGICA DO JOGO
        // -------------------------------------------------------------
        if (!jogo_acabou) {
            // --- CONTROLES DO JOGADOR ---
            // Movimento Horizontal
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
                jogador.rect.x -= jogador.velocidade_mov * dt;
                jogador.direcao = -1;
            }
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
                jogador.rect.x += jogador.velocidade_mov * dt;
                jogador.direcao = 1;
            }

            // Pulo
            if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) && jogador.esta_no_chao) {
                jogador.velocidade.y = FORCA_PULO;
                jogador.esta_no_chao = false;
            }

            // Aplicar Gravidade no Jogador
            jogador.velocidade.y += GRAVIDADE * dt;
            jogador.rect.y += jogador.velocidade.y * dt;

            // Colisão com o Chão
            if (jogador.rect.y >= CHAO_Y - jogador.rect.height) {
                jogador.rect.y = CHAO_Y - jogador.rect.height;
                jogador.velocidade.y = 0;
                jogador.esta_no_chao = true;
            }

            // Limites da Tela
            if (jogador.rect.x < 0) jogador.rect.x = 0;
            if (jogador.rect.x > LARGURA_TELA - jogador.rect.width) jogador.rect.x = LARGURA_TELA - jogador.rect.width;

            // Ataque do Jogador (Teclas J, K ou ESPAÇO)
            if ((IsKeyPressed(KEY_J) || IsKeyPressed(KEY_K) || IsKeyPressed(KEY_SPACE)) && !jogador.esta_atacando) {
                jogador.esta_atacando = true;
                jogador.tempo_ataque = 0.2f;

                // Caixa de ataque gerada na direção que o jogador está olhando
                float alcance = 60.0f;
                float ataque_x = (jogador.direcao == 1) ? (jogador.rect.x + jogador.rect.width) : (jogador.rect.x - alcance);
                
                jogador.area_ataque = (Rectangle){ ataque_x, jogador.rect.y + 15, alcance, 40 };

                // Dano no Chefão
                if (CheckCollisionRecs(jogador.area_ataque, chefao.rect)) {
                    chefao.vida -= 18;
                    chefao.tempo_dano = 0.15f;
                    tremor_tela = 0.12f;
                    
                    Vector2 ponto_impacto = { chefao.rect.x + chefao.rect.width/2, chefao.rect.y + 30 };
                    GerarParticulas(ponto_impacto, RED, 12);

                    if (chefao.vida <= 0) {
                        chefao.vida = 0;
                        jogo_acabou = true;
                        vitoria = true;
                    }
                }
            }

            // Timers de Ataque
            if (jogador.esta_atacando) {
                jogador.tempo_ataque -= dt;
                if (jogador.tempo_ataque <= 0) jogador.esta_atacando = false;
            }

            // --- IA DO CHEFÃO ---
            // Gravidade do Chefão
            chefao.velocidade.y += GRAVIDADE * dt;
            chefao.rect.y += chefao.velocidade.y * dt;

            if (chefao.rect.y >= CHAO_Y - chefao.rect.height) {
                chefao.rect.y = CHAO_Y - chefao.rect.height;
                chefao.velocidade.y = 0;
                chefao.esta_no_chao = true;
            }

            // Direção da IA
            chefao.direcao = (jogador.rect.x > chefao.rect.x) ? 1 : -1;

            // Perseguição
            float distancia = fabsf((chefao.rect.x + chefao.rect.width/2) - (jogador.rect.x + jogador.rect.width/2));
            if (distancia > 70.0f) {
                chefao.rect.x += chefao.direcao * chefao.velocidade_mov * dt;
            } else {
                // Chefão ataca quando está perto
                if (!chefao.esta_atacando && GetRandomValue(1, 30) == 1) {
                    chefao.esta_atacando = true;
                    chefao.tempo_ataque = 0.35f;

                    // Ataque causa dano se atingir o jogador
                    if (CheckCollisionRecs(chefao.rect, jogador.rect)) {
                        jogador.vida -= 22;
                        jogador.tempo_dano = 0.2f;
                        tremor_tela = 0.25f;

                        Vector2 ponto_impacto = { jogador.rect.x + jogador.rect.width/2, jogador.rect.y + 30 };
                        GerarParticulas(ponto_impacto, GOLD, 15);

                        if (jogador.vida <= 0) {
                            jogador.vida = 0;
                            jogo_acabou = true;
                            vitoria = false;
                        }
                    }
                }
            }

            // Pulo aleatório do chefão para surpreender
            if (chefao.esta_no_chao && GetRandomValue(1, 150) == 1) {
                chefao.velocidade.y = FORCA_PULO * 0.85f;
                chefao.esta_no_chao = false;
            }

            if (chefao.esta_atacando) {
                chefao.tempo_ataque -= dt;
                if (chefao.tempo_ataque <= 0) chefao.esta_atacando = false;
            }

            // Atualiza timers de cor ao sofrer dano
            if (jogador.tempo_dano > 0) jogador.tempo_dano -= dt;
            if (chefao.tempo_dano > 0) chefao.tempo_dano -= dt;

            AtualizarParticulas(dt);
        }

        // Tremor de tela
        Camera2D camera = { 0 };
        camera.zoom = 1.0f;
        if (tremor_tela > 0) {
            tremor_tela -= dt;
            camera.offset = (Vector2){ (float)GetRandomValue(-6, 6), (float)GetRandomValue(-6, 6) };
        }

        // -------------------------------------------------------------
        // 2. RENDERIZAÇÃO
        // -------------------------------------------------------------
        BeginDrawing();
            ClearBackground((Color){ 30, 30, 40, 255 }); // Fundo escuro elegante

            BeginMode2D(camera);

                // Desenhar Cenário (Arena)
                DrawRectangle(0, CHAO_Y, LARGURA_TELA, ALTURA_TELA - CHAO_Y, (Color){ 45, 52, 54, 255 });
                DrawRectangle(0, CHAO_Y, LARGURA_TELA, 8, (Color){ 0, 184, 148, 255 }); // Borda brilhante no chão

                // Sombras no Chão
                DrawEllipse((int)(jogador.rect.x + jogador.rect.width/2), CHAO_Y + 5, 25, 6, (Color){ 0, 0, 0, 80 });
                DrawEllipse((int)(chefao.rect.x + chefao.rect.width/2), CHAO_Y + 5, 45, 10, (Color){ 0, 0, 0, 80 });

                if (!jogo_acabou) {
                    // --- DESENHAR JOGADOR ---
                    Color cor_jogador = (jogador.tempo_dano > 0) ? WHITE : jogador.cor;
                    DrawRectangleRounded(jogador.rect, 0.2f, 4, cor_jogador);
                    
                    // Rosto/Olho do Jogador (Indicando a direção)
                    float olho_x = (jogador.direcao == 1) ? (jogador.rect.x + jogador.rect.width - 12) : (jogador.rect.x + 4);
                    DrawRectangle((int)olho_x, (int)(jogador.rect.y + 12), 8, 8, RAYWHITE);

                    // Efeito de Ataque do Jogador (Efeito de corte/espada)
                    if (jogador.esta_atacando) {
                        Color cor_corte = (Color){ 241, 196, 15, 200 };
                        DrawRectangleRounded(jogador.area_ataque, 0.4f, 4, cor_corte);
                    }

                    // --- DESENHAR CHEFÃO ---
                    Color cor_chefao = (chefao.tempo_dano > 0) ? WHITE : ((chefao.esta_atacando) ? ORANGE : chefao.cor);
                    DrawRectangleRounded(chefao.rect, 0.15f, 4, cor_chefao);

                    // Olhos do Chefão
                    float olho_chefao_x = (chefao.direcao == 1) ? (chefao.rect.x + chefao.rect.width - 18) : (chefao.rect.x + 8);
                    DrawRectangle((int)olho_chefao_x, (int)(chefao.rect.y + 18), 10, 10, YELLOW);

                    DesenharParticulas();
                }

            EndMode2D();

            // --- INTERFACE DE USUÁRIO (HUD) ---
            if (!jogo_acabou) {
                // UI Jogador
                DrawText("HERÓI", 30, 20, 18, RAYWHITE);
                DrawRectangle(30, 42, 250, 22, (Color){ 20, 20, 20, 200 });
                DrawRectangle(30, 42, (int)(250.0f * ((float)jogador.vida / jogador.vida_max)), 22, (Color){ 46, 204, 113, 255 });
                DrawRectangleLines(30, 42, 250, 22, RAYWHITE);

                // UI Chefão
                DrawText("TITÃ (CHEFÃO)", LARGURA_TELA - 280, 20, 18, RAYWHITE);
                DrawRectangle(LARGURA_TELA - 280, 42, 250, 22, (Color){ 20, 20, 20, 200 });
                DrawRectangle(LARGURA_TELA - 280, 42, (int)(250.0f * ((float)chefao.vida / chefao.vida_max)), 22, (Color){ 231, 76, 60, 255 });
                DrawRectangleLines(LARGURA_TELA - 280, 42, 250, 22, RAYWHITE);

                // Rodapé de Controles
                DrawText("[A/D] Mover  |  [W] Pular  |  [J / ESPAÇO] Atacar", 230, ALTURA_TELA - 30, 16, LIGHTGRAY);
            } else {
                // Tela Final
                if (vitoria) {
                    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){ 0, 0, 0, 180 });
                    DrawText("VITÓRIA ÉPICA!", LARGURA_TELA/2 - MeasureText("VITÓRIA ÉPICA!", 40)/2, 180, 40, GREEN);
                    DrawText("O Chefão foi totalmente derrotado.", LARGURA_TELA/2 - MeasureText("O Chefão foi totalmente derrotado.", 20)/2, 240, 20, RAYWHITE);
                } else {
                    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){ 0, 0, 0, 180 });
                    DrawText("VOCÊ MORREU!", LARGURA_TELA/2 - MeasureText("VOCÊ MORREU!", 40)/2, 180, 40, RED);
                    DrawText("Treine mais e tente novamente.", LARGURA_TELA/2 - MeasureText("Treine mais e tente novamente.", 20)/2, 240, 20, RAYWHITE);
                }
                DrawText("Pressione [ESC] para encerrar", LARGURA_TELA/2 - MeasureText("Pressione [ESC] para encerrar", 18)/2, 310, 18, GRAY);
            }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
