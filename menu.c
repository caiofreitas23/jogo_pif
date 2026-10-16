#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define LARGURA_TELA 900
#define ALTURA_TELA 500
#define CHAO_Y 400
#define GRAVIDADE 1200.0f
#define FORCA_PULO -550.0f
#define MAX_PARTICULAS 50
#define LARGURA_MAPA 3000

typedef enum {
    ESTADO_MENU_INICIAL,
    ESTADO_SELECAO_MODO,
    ESTADO_CARREGAR,
    ESTADO_CREDITOS,
    ESTADO_EXPLORACAO, // Novo estado: Andar pelo mapa top-down
    ESTADO_JOGANDO,
    ESTADO_PAUSADO,
    ESTADO_FIM_DE_JOGO
} EstadoJogo;

typedef enum {
    MODO_HISTORIA,
    MODO_ARCADE
} ModoJogo;

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
    int direcao; 
    bool esta_atacando;
    float tempo_ataque;
    Rectangle area_ataque;
    float tempo_dano;
} Personagem;

// Áudio
Sound som_ataque;
Sound som_dano;
Sound som_pulo;
Sound som_clique;

Particula particulas[MAX_PARTICULAS] = { 0 };

// Posição do jogador no Mapa Top-Down
Vector2 pos_jogador_mapa = { 100, 250 };

// --- GERADORES SINTÉTICOS DE SOM ---
Sound GerarSomAtaque(void) {
    Wave wave = { 0 };
    wave.frameCount = 44100 * 0.1f;
    wave.sampleRate = 44100;
    wave.sampleSize = 16;
    wave.channels = 1;
    short *data = (short *)RL_MALLOC(wave.frameCount * sizeof(short));
    for (int i = 0; i < wave.frameCount; i++) {
        float t = (float)i / wave.sampleRate;
        float freq = 400.0f - (t * 2500.0f);
        data[i] = (short)(sinf(2.0f * PI * freq * t) * (1.0f - t/0.1f) * 8000.0f);
    }
    wave.data = data;
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    SetSoundVolume(snd, 0.3f);
    return snd;
}

Sound GerarSomDano(void) {
    Wave wave = { 0 };
    wave.frameCount = 44100 * 0.15f;
    wave.sampleRate = 44100;
    wave.sampleSize = 16;
    wave.channels = 1;
    short *data = (short *)RL_MALLOC(wave.frameCount * sizeof(short));
    for (int i = 0; i < wave.frameCount; i++) {
        float t = (float)i / wave.sampleRate;
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        data[i] = (short)(noise * (1.0f - t/0.15f) * 10000.0f);
    }
    wave.data = data;
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    SetSoundVolume(snd, 0.4f);
    return snd;
}

Sound GerarSomPulo(void) {
    Wave wave = { 0 };
    wave.frameCount = 44100 * 0.12f;
    wave.sampleRate = 44100;
    wave.sampleSize = 16;
    wave.channels = 1;
    short *data = (short *)RL_MALLOC(wave.frameCount * sizeof(short));
    for (int i = 0; i < wave.frameCount; i++) {
        float t = (float)i / wave.sampleRate;
        float freq = 150.0f + (t * 1200.0f);
        data[i] = (short)(sinf(2.0f * PI * freq * t) * (1.0f - t/0.12f) * 6000.0f);
    }
    wave.data = data;
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    SetSoundVolume(snd, 0.25f);
    return snd;
}

Sound GerarSomClique(void) {
    Wave wave = { 0 };
    wave.frameCount = 44100 * 0.05f;
    wave.sampleRate = 44100;
    wave.sampleSize = 16;
    wave.channels = 1;
    short *data = (short *)RL_MALLOC(wave.frameCount * sizeof(short));
    for (int i = 0; i < wave.frameCount; i++) {
        float t = (float)i / wave.sampleRate;
        data[i] = (short)(sinf(2.0f * PI * 800.0f * t) * (1.0f - t/0.05f) * 5000.0f);
    }
    wave.data = data;
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    SetSoundVolume(snd, 0.3f);
    return snd;
}

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

bool DesenharBotao(Rectangle rect, const char *texto, Color cor_base, Color cor_hover) {
    Vector2 mouse_pos = GetMousePosition();
    bool esta_sobre = CheckCollisionPointRec(mouse_pos, rect);
    Color cor = esta_sobre ? cor_hover : cor_base;

    DrawRectangleRounded(rect, 0.25f, 4, cor);
    DrawRectangleRoundedLines(rect, 0.25f, 4, 2, RAYWHITE);

    int tamanho_fonte = 20;
    int largura_texto = MeasureText(texto, tamanho_fonte);
    DrawText(texto, (int)(rect.x + rect.width/2 - largura_texto/2), (int)(rect.y + rect.height/2 - tamanho_fonte/2), tamanho_fonte, RAYWHITE);

    if (esta_sobre && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySound(som_clique);
        return true;
    }
    return false;
}

void SalvarJogo(Personagem *jog, Personagem *boss, int fase, ModoJogo modo, Vector2 pos_mapa) {
    FILE *arquivo = fopen("savegame.txt", "w");
    if (arquivo != NULL) {
        fprintf(arquivo, "%d %d %d %d %f %f\n", jog->vida, boss->vida, fase, (int)modo, pos_mapa.x, pos_mapa.y);
        fclose(arquivo);
    }
}

bool CarregarJogo(Personagem *jog, Personagem *boss, int *fase, ModoJogo *modo, Vector2 *pos_mapa) {
    FILE *arquivo = fopen("savegame.txt", "r");
    if (arquivo != NULL) {
        int m;
        if (fscanf(arquivo, "%d %d %d %d %f %f", &jog->vida, &boss->vida, fase, &m, &pos_mapa->x, &pos_mapa->y) == 6) {
            *modo = (ModoJogo)m;
            fclose(arquivo);
            return true;
        }
        fclose(arquivo);
    }
    return false;
}

void ReiniciarJogo(Personagem *jog, Personagem *boss, int nivel) {
    *jog = (Personagem){
        .rect = { 150, CHAO_Y - 70, 45, 70 },
        .velocidade = { 0, 0 },
        .cor = (Color){ 41, 128, 185, 255 },
        .vida = 100,
        .vida_max = 100,
        .velocidade_mov = 320.0f,
        .esta_no_chao = true,
        .direcao = 1
    };

    int vida_boss = 300 + (nivel * 50);
    *boss = (Personagem){
        .rect = { 650, CHAO_Y - 110, 85, 110 },
        .velocidade = { 0, 0 },
        .cor = (Color){ 192, 57, 43, 255 },
        .vida = vida_boss,
        .vida_max = vida_boss,
        .velocidade_mov = 150.0f + (nivel * 15.0f),
        .esta_no_chao = true,
        .direcao = -1
    };

    for (int i = 0; i < MAX_PARTICULAS; i++) particulas[i].ativa = false;
}

int main(void) {
    InitWindow(LARGURA_TELA, ALTURA_TELA, "Boss Fight 2D - Raylib Engine");
    InitAudioDevice();
    SetTargetFPS(60);

    som_ataque = GerarSomAtaque();
    som_dano = GerarSomDano();
    som_pulo = GerarSomPulo();
    som_clique = GerarSomClique();

    EstadoJogo estado = ESTADO_MENU_INICIAL;
    EstadoJogo estado_anterior = ESTADO_MENU_INICIAL;

    Personagem jogador, chefao;
    ModoJogo modo_atual = MODO_HISTORIA;

    int fase_atual = 1;
    float tremor_tela = 0.0f;
    bool vitoria = false;
    bool mira_travada = false;
    char mensagem_status[60] = "";
    float timer_mensagem = 0.0f;

    // Locais dos portões dos Bosses no Mapa (Modo História)
    float portoes_boss[3] = { 700.0f, 1500.0f, 2400.0f };

    ReiniciarJogo(&jogador, &chefao, fase_atual);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_P)) {
            if (estado == ESTADO_PAUSADO) {
                // Retorna exatamente para a tela onde o jogo estava antes do pause
                estado = estado_anterior;
            } else if (estado == ESTADO_JOGANDO || estado == ESTADO_EXPLORACAO) {
                // Guarda se o jogador estava na arena ou no mapa antes de pausar
                estado_anterior = estado;
                estado = ESTADO_PAUSADO;
            }
        }

        // -------------------------------------------------------------
        // LÓGICA DO MAPA TOP-DOWN (MODO HISTÓRIA)
        // -------------------------------------------------------------
        if (estado == ESTADO_EXPLORACAO) {
            float vel_mapa = 220.0f;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) pos_jogador_mapa.y -= vel_mapa * dt;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) pos_jogador_mapa.y += vel_mapa * dt;
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) pos_jogador_mapa.x -= vel_mapa * dt;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) pos_jogador_mapa.x += vel_mapa * dt;

            // Limites do caminho
            if (pos_jogador_mapa.y < 180) pos_jogador_mapa.y = 180;
            if (pos_jogador_mapa.y > 320) pos_jogador_mapa.y = 320;
            if (pos_jogador_mapa.x < 50) pos_jogador_mapa.x = 50;
            if (pos_jogador_mapa.x > LARGURA_MAPA - 100) pos_jogador_mapa.x = LARGURA_MAPA - 100;

            // Verificar se chegou ao portão da fase atual
            if (fase_atual <= 3) {
                float x_portao = portoes_boss[fase_atual - 1];
                if (pos_jogador_mapa.x >= x_portao - 20 && pos_jogador_mapa.x <= x_portao + 40) {
                    ReiniciarJogo(&jogador, &chefao, fase_atual);
                    mira_travada = false;
                    estado = ESTADO_JOGANDO;
                }
            }
        }

        // -------------------------------------------------------------
        // LÓGICA DA BATALHA CONTRA CHEFÃO
        // -------------------------------------------------------------
        if (estado == ESTADO_JOGANDO) {
            if (IsKeyPressed(KEY_Z)) {
                mira_travada = !mira_travada;
                PlaySound(som_clique);
            }

            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
                jogador.rect.x -= jogador.velocidade_mov * dt;
                if (!mira_travada) jogador.direcao = -1;
            }
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
                jogador.rect.x += jogador.velocidade_mov * dt;
                if (!mira_travada) jogador.direcao = 1;
            }

            if (mira_travada) {
                float centro_jog = jogador.rect.x + jogador.rect.width/2;
                float centro_boss = chefao.rect.x + chefao.rect.width/2;
                jogador.direcao = (centro_boss >= centro_jog) ? 1 : -1;
            }

            if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) && jogador.esta_no_chao) {
                jogador.velocidade.y = FORCA_PULO;
                jogador.esta_no_chao = false;
                PlaySound(som_pulo);
            }

            jogador.velocidade.y += GRAVIDADE * dt;
            jogador.rect.y += jogador.velocidade.y * dt;

            if (jogador.rect.y >= CHAO_Y - jogador.rect.height) {
                jogador.rect.y = CHAO_Y - jogador.rect.height;
                jogador.velocidade.y = 0;
                jogador.esta_no_chao = true;
            }

            if (jogador.rect.x < 0) jogador.rect.x = 0;
            if (jogador.rect.x > LARGURA_TELA - jogador.rect.width) jogador.rect.x = LARGURA_TELA - jogador.rect.width;

            // Ataque
            if ((IsKeyPressed(KEY_J) || IsKeyPressed(KEY_K) || IsKeyPressed(KEY_SPACE)) && !jogador.esta_atacando) {
                jogador.esta_atacando = true;
                jogador.tempo_ataque = 0.2f;
                PlaySound(som_ataque);

                float alcance = 60.0f;
                float ataque_x = (jogador.direcao == 1) ? (jogador.rect.x + jogador.rect.width) : (jogador.rect.x - alcance);
                jogador.area_ataque = (Rectangle){ ataque_x, jogador.rect.y + 15, alcance, 40 };

                if (CheckCollisionRecs(jogador.area_ataque, chefao.rect)) {
                    chefao.vida -= 20;
                    chefao.tempo_dano = 0.15f;
                    tremor_tela = 0.12f;
                    PlaySound(som_dano);

                    Vector2 ponto_impacto = { chefao.rect.x + chefao.rect.width/2, chefao.rect.y + 30 };
                    GerarParticulas(ponto_impacto, RED, 12);

                    if (chefao.vida <= 0) {
                        chefao.vida = 0;
                        vitoria = true;
                        estado = ESTADO_FIM_DE_JOGO;
                    }
                }
            }

            if (jogador.esta_atacando) {
                jogador.tempo_ataque -= dt;
                if (jogador.tempo_ataque <= 0) jogador.esta_atacando = false;
            }

            // IA do Chefão
            chefao.velocidade.y += GRAVIDADE * dt;
            chefao.rect.y += chefao.velocidade.y * dt;

            if (chefao.rect.y >= CHAO_Y - chefao.rect.height) {
                chefao.rect.y = CHAO_Y - chefao.rect.height;
                chefao.velocidade.y = 0;
                chefao.esta_no_chao = true;
            }

            chefao.direcao = (jogador.rect.x > chefao.rect.x) ? 1 : -1;
            float distancia = fabsf((chefao.rect.x + chefao.rect.width/2) - (jogador.rect.x + jogador.rect.width/2));
            float alcance_chefao = 75.0f;

            if (distancia > alcance_chefao) {
                chefao.rect.x += chefao.direcao * chefao.velocidade_mov * dt;
            } else {
                if (!chefao.esta_atacando && GetRandomValue(1, 30) == 1) {
                    chefao.esta_atacando = true;
                    chefao.tempo_ataque = 0.35f;

                    float ataque_boss_x = (chefao.direcao == 1) ? chefao.rect.x : (chefao.rect.x - alcance_chefao);
                    float largura_ataque = chefao.rect.width + alcance_chefao;
                    chefao.area_ataque = (Rectangle){ ataque_boss_x, chefao.rect.y + 5, largura_ataque, chefao.rect.height - 10 };

                    if (CheckCollisionRecs(chefao.area_ataque, jogador.rect) || CheckCollisionRecs(chefao.rect, jogador.rect)) {
                        jogador.vida -= 22;
                        jogador.tempo_dano = 0.2f;
                        tremor_tela = 0.25f;
                        PlaySound(som_dano);

                        Vector2 ponto_impacto = { jogador.rect.x + jogador.rect.width/2, jogador.rect.y + 30 };
                        GerarParticulas(ponto_impacto, GOLD, 15);

                        if (jogador.vida <= 0) {
                            jogador.vida = 0;
                            vitoria = false;
                            estado = ESTADO_FIM_DE_JOGO;
                        }
                    }
                }
            }

            if (chefao.esta_no_chao && GetRandomValue(1, 150) == 1) {
                chefao.velocidade.y = FORCA_PULO * 0.85f;
                chefao.esta_no_chao = false;
            }

            if (chefao.esta_atacando) {
                chefao.tempo_ataque -= dt;
                if (chefao.tempo_ataque <= 0) chefao.esta_atacando = false;
            }

            if (jogador.tempo_dano > 0) jogador.tempo_dano -= dt;
            if (chefao.tempo_dano > 0) chefao.tempo_dano -= dt;

            AtualizarParticulas(dt);
        }

        if (timer_mensagem > 0) timer_mensagem -= dt;

        // Câmera da Arena de Combate
        Camera2D camera = { 0 };
        camera.zoom = 1.0f;
        if (tremor_tela > 0) {
            tremor_tela -= dt;
            camera.offset = (Vector2){ (float)GetRandomValue(-6, 6), (float)GetRandomValue(-6, 6) };
        }

        // Câmera do Mapa Top-Down
        Camera2D camera_mapa = { 0 };
        camera_mapa.target = pos_jogador_mapa;
        camera_mapa.offset = (Vector2){ LARGURA_TELA/2.0f, ALTURA_TELA/2.0f };
        camera_mapa.zoom = 1.0f;

        // -------------------------------------------------------------
        // RENDERIZAÇÃO
        // -------------------------------------------------------------
        BeginDrawing();
            ClearBackground((Color){ 20, 20, 28, 255 });

            // RENDERIZAÇÃO DO MAPA TOP-DOWN (MODO HISTÓRIA)
            if (estado == ESTADO_EXPLORACAO) {
                BeginMode2D(camera_mapa);
                    // Gramado
                    DrawRectangle(0, 0, LARGURA_MAPA, ALTURA_TELA, (Color){ 34, 112, 62, 255 });

                    // Estrada Antiga de Pedra
                    DrawRectangle(0, 180, LARGURA_MAPA, 140, (Color){ 108, 122, 137, 255 });
                    DrawRectangle(0, 180, LARGURA_MAPA, 6, (Color){ 70, 80, 90, 255 });
                    DrawRectangle(0, 314, LARGURA_MAPA, 6, (Color){ 70, 80, 90, 255 });

                    // Árvores Decorativas Medievais
                    for (int x = 100; x < LARGURA_MAPA; x += 200) {
                        DrawCircle(x, 80, 35, (Color){ 20, 80, 40, 255 });
                        DrawCircle(x + 50, 420, 35, (Color){ 20, 80, 40, 255 });
                    }

                    // Desenhar os 3 Portões de Boss
                    for (int i = 0; i < 3; i++) {
                        float x_port = portoes_boss[i];
                        Color cor_portao = (i + 1 < fase_atual) ? GRAY : ((i + 1 == fase_atual) ? GOLD : DARKGRAY);
                        
                        DrawRectangle((int)x_port, 150, 30, 200, cor_portao);
                        DrawRectangleLines((int)x_port, 150, 30, 200, BLACK);

                        char label_boss[20];
                        snprintf(label_boss, sizeof(label_boss), "BOSS %d", i + 1);
                        DrawText(label_boss, (int)x_port - 10, 125, 16, RAYWHITE);
                    }

                    // Jogador no Mapa (Visto de cima)
                    DrawCircleV(pos_jogador_mapa, 16, (Color){ 41, 128, 185, 255 });
                    DrawCircleLines((int)pos_jogador_mapa.x, (int)pos_jogador_mapa.y, 16, RAYWHITE);

                EndMode2D();

                DrawText("MODO HISTÓRIA - MAPA", 30, 20, 22, GOLD);
                DrawText("Siga em frente pela estrada para enfrentar os chefões!", 30, 50, 16, LIGHTGRAY);
                DrawText("[W/A/S/D] Mover pelo mapa  |  [E] Pausar", 30, ALTURA_TELA - 35, 15, WHITE);
            }

            // RENDERIZAÇÃO DA ARENA DE COMBATE 2D
            if (estado == ESTADO_JOGANDO || estado == ESTADO_PAUSADO || estado == ESTADO_FIM_DE_JOGO) {
                BeginMode2D(camera);
                    DrawRectangle(0, CHAO_Y, LARGURA_TELA, ALTURA_TELA - CHAO_Y, (Color){ 45, 52, 54, 255 });
                    DrawRectangle(0, CHAO_Y, LARGURA_TELA, 8, (Color){ 0, 184, 148, 255 });

                    DrawEllipse((int)(jogador.rect.x + jogador.rect.width/2), CHAO_Y + 5, 25, 6, (Color){ 0, 0, 0, 80 });
                    DrawEllipse((int)(chefao.rect.x + chefao.rect.width/2), CHAO_Y + 5, 45, 10, (Color){ 0, 0, 0, 80 });

                    // Jogador
                    Color cor_jog = (jogador.tempo_dano > 0) ? WHITE : jogador.cor;
                    DrawRectangleRounded(jogador.rect, 0.2f, 4, cor_jog);
                    float olho_x = (jogador.direcao == 1) ? (jogador.rect.x + jogador.rect.width - 12) : (jogador.rect.x + 4);
                    DrawRectangle((int)olho_x, (int)(jogador.rect.y + 12), 8, 8, RAYWHITE);

                    if (jogador.esta_atacando) {
                        DrawRectangleRounded(jogador.area_ataque, 0.4f, 4, (Color){ 241, 196, 15, 200 });
                    }

                    // Chefão
                    Color cor_boss = (chefao.tempo_dano > 0) ? WHITE : ((chefao.esta_atacando) ? ORANGE : chefao.cor);
                    DrawRectangleRounded(chefao.rect, 0.15f, 4, cor_boss);
                    float olho_boss_x = (chefao.direcao == 1) ? (chefao.rect.x + chefao.rect.width - 18) : (chefao.rect.x + 8);
                    DrawRectangle((int)olho_boss_x, (int)(chefao.rect.y + 18), 10, 10, YELLOW);

                    if (chefao.esta_atacando) {
                        DrawRectangleRounded(chefao.area_ataque, 0.3f, 4, (Color){ 230, 126, 34, 180 });
                    }

                    // Indicador de Lock-On sobre a cabeça do Chefão
                    if (mira_travada) {
                        Vector2 centro_boss = { chefao.rect.x + chefao.rect.width/2, chefao.rect.y - 15 };
                        DrawTriangle((Vector2){ centro_boss.x - 10, centro_boss.y - 12 }, (Vector2){ centro_boss.x + 10, centro_boss.y - 12 }, (Vector2){ centro_boss.x, centro_boss.y }, RED);
                    }

                    DesenharParticulas();
                EndMode2D();

                // UI Barras
                DrawText("HERÓI", 30, 20, 18, RAYWHITE);
                DrawRectangle(30, 42, 250, 22, (Color){ 20, 20, 20, 200 });
                DrawRectangle(30, 42, (int)(250.0f * ((float)jogador.vida / jogador.vida_max)), 22, (Color){ 46, 204, 113, 255 });
                DrawRectangleLines(30, 42, 250, 22, RAYWHITE);

                char txt_boss[30];
                snprintf(txt_boss, sizeof(txt_boss), "CHEFÃO (FASE %d)", fase_atual);
                DrawText(txt_boss, LARGURA_TELA - 280, 20, 18, RAYWHITE);
                DrawRectangle(LARGURA_TELA - 280, 42, 250, 22, (Color){ 20, 20, 20, 200 });
                DrawRectangle(LARGURA_TELA - 280, 42, (int)(250.0f * ((float)chefao.vida / chefao.vida_max)), 22, (Color){ 231, 76, 60, 255 });
                DrawRectangleLines(LARGURA_TELA - 280, 42, 250, 22, RAYWHITE);

                if (mira_travada) {
                    DrawText("[Z] MIRA: TRAVADA", 30, 72, 14, GREEN);
                } else {
                    DrawText("[Z] MIRA: LIVRE", 30, 72, 14, GRAY);
                }

                DrawText("[E] Pausar  |  [A/D] Mover  |  [W] Pular  |  [J] Atacar  |  [Z] Lock-On", 130, ALTURA_TELA - 30, 15, LIGHTGRAY);
            }

            // MENU INICIAL
            if (estado == ESTADO_MENU_INICIAL) {
                DrawText("Beyond the Fallen", LARGURA_TELA/2 - MeasureText("Beyond the Fallen", 42)/2, 80, 42, GOLD);
                DrawText("Projeto de Algoritmos & Programação", LARGURA_TELA/2 - MeasureText("Projeto de Algoritmos & Programação", 16)/2, 130, 16, GRAY);

                if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 125, 180, 250, 45 }, "NOVO JOGO", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) {
                    estado = ESTADO_SELECAO_MODO;
                }
                if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 125, 240, 250, 45 }, "CARREGAR JOGO", (Color){ 39, 174, 96, 255 }, (Color){ 46, 204, 113, 255 })) {
                    estado = ESTADO_CARREGAR;
                }
                if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 125, 300, 250, 45 }, "CRÉDITOS", (Color){ 142, 68, 173, 255 }, (Color){ 155, 89, 182, 255 })) {
                    estado = ESTADO_CREDITOS;
                }
                if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 125, 360, 250, 45 }, "SAIR", (Color){ 192, 57, 43, 255 }, (Color){ 231, 76, 60, 255 })) {
                    break;
                }
            }

            // SELEÇÃO DE MODO
            if (estado == ESTADO_SELECAO_MODO) {
                DrawText("SELEÇÃO DE MODO", LARGURA_TELA/2 - MeasureText("SELEÇÃO DE MODO", 32)/2, 60, 32, RAYWHITE);

                if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 140, 130, 280, 50 }, "MODO HISTÓRIA", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) {
                    modo_atual = MODO_HISTORIA;
                    fase_atual = 1;
                    pos_jogador_mapa = (Vector2){ 100, 250 };
                    estado = ESTADO_EXPLORACAO;
                }
                DrawText("Derrote chefões progressivos até zerar!", LARGURA_TELA/2 - MeasureText("Derrote chefões progressivos até zerar!", 14)/2, 188, 14, GRAY);

                if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 140, 250, 280, 50 }, "MODO ARCADE", (Color){ 211, 84, 0, 255 }, (Color){ 230, 126, 34, 255 })) {
                    modo_atual = MODO_ARCADE;
                    fase_atual = 1;
                    ReiniciarJogo(&jogador, &chefao, fase_atual);
                    mira_travada = false;
                    estado = ESTADO_JOGANDO;
                }
                DrawText("Enfrente ondas infinitas com dificuldade crescente!", LARGURA_TELA/2 - MeasureText("Enfrente ondas infinitas com dificuldade crescente!", 14)/2, 308, 14, GRAY);

                if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 80, 370, 160, 40 }, "VOLTAR", (Color){ 127, 140, 141, 255 }, (Color){ 149, 165, 166, 255 })) {
                    estado = ESTADO_MENU_INICIAL;
                }
            }

            // CARREGAR JOGO
            if (estado == ESTADO_CARREGAR) {
                DrawText("CARREGAR PROGRESSO", LARGURA_TELA/2 - MeasureText("CARREGAR PROGRESSO", 32)/2, 80, 32, RAYWHITE);

                if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 130, 200, 260, 50 }, "CARREGAR SAVE 1", (Color){ 39, 174, 96, 255 }, (Color){ 46, 204, 113, 255 })) {
                    if (CarregarJogo(&jogador, &chefao, &fase_atual, &modo_atual, &pos_jogador_mapa)) {
                        mira_travada = false;
                        estado = (modo_atual == MODO_HISTORIA) ? ESTADO_EXPLORACAO : ESTADO_JOGANDO;
                    } else {
                        snprintf(mensagem_status, sizeof(mensagem_status), "Nenhum arquivo 'savegame.txt' encontrado!");
                        timer_mensagem = 2.5f;
                    }
                }

                if (timer_mensagem > 0) {
                    DrawText(mensagem_status, LARGURA_TELA/2 - MeasureText(mensagem_status, 16)/2, 270, 16, RED);
                }

                if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 80, 350, 160, 40 }, "VOLTAR", (Color){ 127, 140, 141, 255 }, (Color){ 149, 165, 166, 255 })) {
                    estado = ESTADO_MENU_INICIAL;
                }
            }

            // CRÉDITOS
            if (estado == ESTADO_CREDITOS) {
                DrawText("CRÉDITOS DO JOGO", LARGURA_TELA/2 - MeasureText("CRÉDITOS DO JOGO", 32)/2, 60, 32, GOLD);

                DrawText("Desenvolvimento Geral & Código em C:", LARGURA_TELA/2 - MeasureText("Desenvolvimento Geral & Código em C:", 18)/2, 130, 18, RAYWHITE);
                DrawText("Caio Freitas de Andrade Medeiros", LARGURA_TELA/2 - MeasureText("Caio Freitas de Andrade Medeiros", 16)/2, 155, 16, LIGHTGRAY);
                DrawText("Gabriel Furtado Corrêa Miller", LARGURA_TELA/2 - MeasureText("Gabriel Furtado Corrêa Miller", 16)/2, 175, 16, LIGHTGRAY);
                DrawText("João Pedro Guedes Alcoforado C Leão", LARGURA_TELA/2 - MeasureText("João Pedro Guedes Alcoforado C Leão", 16)/2, 195, 16, LIGHTGRAY);

                DrawText("Engine Gráfica & Áudio:", LARGURA_TELA/2 - MeasureText("Engine Gráfica & Áudio:", 18)/2, 240, 18, RAYWHITE);
                DrawText("Raylib Framework (C99)", LARGURA_TELA/2 - MeasureText("Raylib Framework (C99)", 16)/2, 265, 16, LIGHTGRAY);

                DrawText("Instituição:", LARGURA_TELA/2 - MeasureText("Instituição:", 18)/2, 310, 18, RAYWHITE);
                DrawText("Trabalho Universitário de C", LARGURA_TELA/2 - MeasureText("Trabalho Universitário de C", 16)/2, 335, 16, LIGHTGRAY);

                if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 80, 390, 160, 40 }, "VOLTAR", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) {
                    estado = ESTADO_MENU_INICIAL;
                }
            }

            // MENU DE PAUSE
            if (estado == ESTADO_PAUSADO) {
                DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){ 0, 0, 0, 180 });
                DrawText("PAUSADO", LARGURA_TELA/2 - MeasureText("PAUSADO", 36)/2, 100, 36, RAYWHITE);

                // Voltar para o estado correto (Arena do Arcade, Arena da História ou Mapa)
                if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 100, 180, 200, 45 }, "CONTINUAR", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) {
                    estado = estado_anterior;
                }

                // O botão SALVAR JOGO aparece apenas se estiver no MODO HISTÓRIA
                if (modo_atual == MODO_HISTORIA) {
                    if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 100, 240, 200, 45 }, "SALVAR JOGO", (Color){ 39, 174, 96, 255 }, (Color){ 46, 204, 113, 255 })) {
                        SalvarJogo(&jogador, &chefao, fase_atual, modo_atual, pos_jogador_mapa);
                        snprintf(mensagem_status, sizeof(mensagem_status), "Progresso salvo com sucesso!");
                        timer_mensagem = 2.0f;
                    }
                    if (timer_mensagem > 0) {
                        DrawText(mensagem_status, LARGURA_TELA/2 - MeasureText(mensagem_status, 16)/2, 292, 16, GREEN);
                    }
                }

                float pos_y_sair = (modo_atual == MODO_HISTORIA) ? 320.0f : 250.0f;
                if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 100, (int)pos_y_sair, 200, 45 }, "MENU INICIAL", (Color){ 192, 57, 43, 255 }, (Color){ 231, 76, 60, 255 })) {
                    estado = ESTADO_MENU_INICIAL;
                }
            }

            // TELA DE FIM DE JOGO / VITÓRIA DE FASE
            if (estado == ESTADO_FIM_DE_JOGO) {
                DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){ 0, 0, 0, 200 });

                if (vitoria) {
                    DrawText("FASE CONCLUÍDA!", LARGURA_TELA/2 - MeasureText("FASE CONCLUÍDA!", 36)/2, 110, 36, GREEN);

                    if (modo_atual == MODO_HISTORIA && fase_atual >= 3) {
                        DrawText("PARABÉNS! Você zerou o Modo História!", LARGURA_TELA/2 - MeasureText("PARABÉNS! Você zerou o Modo História!", 20)/2, 165, 20, GOLD);
                        if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 110, 230, 220, 45 }, "MENU INICIAL", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) {
                            estado = ESTADO_MENU_INICIAL;
                        }
                    } else if (modo_atual == MODO_HISTORIA) {
                        if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 110, 210, 220, 45 }, "CONTINUAR VIAGEM", (Color){ 39, 174, 96, 255 }, (Color){ 46, 204, 113, 255 })) {
                            // Avança para a próxima fase no mapa top-down
                            pos_jogador_mapa.x = portoes_boss[fase_atual - 1] + 60.0f;
                            fase_atual++;
                            estado = ESTADO_EXPLORACAO;
                        }
                    } else { // Modo Arcade
                        if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 110, 200, 220, 45 }, "PRÓXIMA ONDA", (Color){ 39, 174, 96, 255 }, (Color){ 46, 204, 113, 255 })) {
                            fase_atual++;
                            ReiniciarJogo(&jogador, &chefao, fase_atual);
                            mira_travada = false;
                            estado = ESTADO_JOGANDO;
                        }
                        if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 110, 260, 220, 45 }, "MENU INICIAL", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) {
                            estado = ESTADO_MENU_INICIAL;
                        }
                    }
                } else {
                    DrawText("VOCÊ FOI DERROTADO!", LARGURA_TELA/2 - MeasureText("VOCÊ FOI DERROTADO!", 36)/2, 110, 36, RED);

                    if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 110, 200, 220, 45 }, "TENTAR NOVAMENTE", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) {
                        ReiniciarJogo(&jogador, &chefao, fase_atual);
                        mira_travada = false;
                        estado = ESTADO_JOGANDO;
                    }
                    if (DesenharBotao((Rectangle){ LARGURA_TELA/2 - 110, 260, 220, 45 }, "MENU INICIAL", (Color){ 192, 57, 43, 255 }, (Color){ 231, 76, 60, 255 })) {
                        estado = ESTADO_MENU_INICIAL;
                    }
                }
            }

        EndDrawing();
    }

    UnloadSound(som_ataque);
    UnloadSound(som_dano);
    UnloadSound(som_pulo);
    UnloadSound(som_clique);
    CloseAudioDevice();

    CloseWindow();
    return 0;
}
