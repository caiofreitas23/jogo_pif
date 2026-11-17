#include "raylib.h"
#include "player.h"
#include "boss.h"
#include "combat.h"
#include "config.h"

typedef enum {
    TELA_COMBATE,
    TELA_FINAL
} Tela;

typedef enum {
    BOSS_GUARDANDO,
    BOSS_PREPARANDO,
    BOSS_RECUPERANDO
} EstadoBoss;

typedef enum {
    ATAQUE_ESQUERDA,
    ATAQUE_DIREITA
} DirecaoAtaque;

typedef enum {
    ESQUIVA_NENHUMA,
    ESQUIVA_ESQUERDA,
    ESQUIVA_DIREITA
} DirecaoEsquiva;

static void iniciar_boss(Boss *boss, int numero_boss) {
    switch (numero_boss) {
        case 1:
            boss_iniciar_bobrovsky(boss);
            break;
        case 2:
            boss_iniciar_vladimir(boss);
            break;
        case 3:
            boss_iniciar_ivan(boss);
            break;
        case 4:
            boss_iniciar_anastasia(boss);
            break;
        default:
            boss_iniciar_bobrovsky(boss);
            break;
    }
}

static int obter_vida_maxima_boss(int numero_boss) {
    switch (numero_boss) {
        case 1: return BOBROVSKY_VIDA;
        case 2: return VLADIMIR_VIDA;
        case 3: return IVAN_VIDA;
        case 4: return ANASTASIA_VIDA;
        default: return BOBROVSKY_VIDA;
    }
}

/*
 * Desenha o jogador em primeiro plano, visto por trás.
 * O deslocamento horizontal representa a esquiva.
 */
static void desenhar_jogador_de_costas(
    int centro_x,
    int topo_y,
    int atacando,
    int defendendo,
    int esquivando,
    int direcao_esquiva
) {
    int deslocamento_x = 0;
    int deslocamento_y = 0;

    if (esquivando) {
        deslocamento_x = direcao_esquiva * 35;
        deslocamento_y = 10;
    }

    int x = centro_x + deslocamento_x;
    int y = topo_y + deslocamento_y;

    Color roupa = (Color){25, 75, 145, 255};
    Color roupa_sombra = (Color){17, 52, 105, 255};
    Color pele = (Color){218, 178, 132, 255};
    Color luva = (Color){190, 35, 35, 255};
    Color calca = (Color){30, 40, 58, 255};

    // Pernas e botas.
    DrawRectangle(x - 52, y + 185, 38, 75, calca);
    DrawRectangle(x + 14, y + 185, 38, 75, calca);
    DrawRectangle(x - 60, y + 245, 48, 19, (Color){35, 31, 29, 255});
    DrawRectangle(x + 12, y + 245, 48, 19, (Color){35, 31, 29, 255});

    // Tronco largo para reforçar a perspectiva de câmera atrás do jogador.
    DrawEllipse(x, y + 137, 76, 83, roupa);
    DrawRectangle(x - 63, y + 98, 126, 75, roupa);

    // Costura/detalhe nas costas.
    DrawRectangle(x - 6, y + 105, 12, 92, roupa_sombra);
    DrawRectangle(x - 43, y + 139, 86, 8, roupa_sombra);

    // Cabeça vista por trás.
    DrawCircle(x, y + 58, 39, pele);
    DrawRectangle(x - 39, y + 42, 78, 27, (Color){65, 43, 31, 255});
    DrawRectangle(x - 31, y + 25, 62, 20, (Color){65, 43, 31, 255});

    if (defendendo) {
        // Guarda alta: braços levantados na frente do corpo.
        DrawLineEx(
            (Vector2){(float)(x - 53), (float)(y + 115)},
            (Vector2){(float)(x - 35), (float)(y + 64)},
            22,
            roupa
        );
        DrawLineEx(
            (Vector2){(float)(x + 53), (float)(y + 115)},
            (Vector2){(float)(x + 35), (float)(y + 64)},
            22,
            roupa
        );
        DrawCircle(x - 35, y + 61, 17, luva);
        DrawCircle(x + 35, y + 61, 17, luva);
    } else if (atacando) {
        // Soco direcionado ao adversário, acima da silhueta do ombro.
        DrawLineEx(
            (Vector2){(float)(x + 25), (float)(y + 122)},
            (Vector2){(float)(x + 8), (float)(y + 38)},
            23,
            roupa
        );
        DrawLineEx(
            (Vector2){(float)(x - 24), (float)(y + 123)},
            (Vector2){(float)(x - 53), (float)(y + 84)},
            21,
            roupa
        );
        DrawCircle(x + 8, y + 35, 18, luva);
        DrawCircle(x - 57, y + 80, 16, luva);
    } else {
        // Guarda neutra, com luvas visíveis nas laterais.
        DrawLineEx(
            (Vector2){(float)(x - 53), (float)(y + 115)},
            (Vector2){(float)(x - 70), (float)(y + 165)},
            21,
            roupa
        );
        DrawLineEx(
            (Vector2){(float)(x + 53), (float)(y + 115)},
            (Vector2){(float)(x + 70), (float)(y + 165)},
            21,
            roupa
        );
        DrawCircle(x - 72, y + 169, 17, luva);
        DrawCircle(x + 72, y + 169, 17, luva);
    }
}

/*
 * Desenha o chefe menor e ao fundo, de frente para a câmera.
 */
static void desenhar_boss_de_frente(
    int centro_x,
    int topo_y,
    Color roupa,
    Color pele,
    int atacando,
    int defendendo
) {
    int x = centro_x;
    int y = topo_y;

    Color luva = (Color){190, 35, 35, 255};
    Color sombra = (Color){80, 35, 35, 255};
    Color calca = (Color){48, 43, 42, 255};

    // Pernas.
    DrawRectangle(x - 25, y + 104, 19, 48, calca);
    DrawRectangle(x + 6, y + 104, 19, 48, calca);

    // Tronco.
    DrawRectangle(x - 38, y + 42, 76, 69, roupa);
    DrawRectangle(x - 38, y + 91, 76, 12, sombra);

    // Cabeça e rosto.
    DrawCircle(x, y + 23, 25, pele);
    DrawRectangle(x - 26, y + 12, 52, 9, (Color){60, 43, 34, 255});
    DrawCircle(x - 9, y + 23, 2, BLACK);
    DrawCircle(x + 9, y + 23, 2, BLACK);
    DrawLine(x - 7, y + 34, x + 7, y + 34, (Color){95, 45, 38, 255});

    if (defendendo) {
        // Guarda do chefe.
        DrawLineEx(
            (Vector2){(float)(x - 30), (float)(y + 61)},
            (Vector2){(float)(x - 18), (float)(y + 39)},
            15,
            roupa
        );
        DrawLineEx(
            (Vector2){(float)(x + 30), (float)(y + 61)},
            (Vector2){(float)(x + 18), (float)(y + 39)},
            15,
            roupa
        );
        DrawCircle(x - 18, y + 38, 12, luva);
        DrawCircle(x + 18, y + 38, 12, luva);
    } else if (atacando) {
        // Braço avançado na preparação do golpe.
        DrawLineEx(
            (Vector2){(float)(x + 27), (float)(y + 59)},
            (Vector2){(float)(x + 62), (float)(y + 68)},
            16,
            pele
        );
        DrawCircle(x + 68, y + 68, 13, luva);
        DrawLineEx(
            (Vector2){(float)(x - 28), (float)(y + 62)},
            (Vector2){(float)(x - 38), (float)(y + 82)},
            14,
            pele
        );
        DrawCircle(x - 41, y + 86, 12, luva);
    } else {
        // Guarda neutra.
        DrawLineEx(
            (Vector2){(float)(x - 30), (float)(y + 61)},
            (Vector2){(float)(x - 44), (float)(y + 84)},
            14,
            pele
        );
        DrawLineEx(
            (Vector2){(float)(x + 30), (float)(y + 61)},
            (Vector2){(float)(x + 44), (float)(y + 84)},
            14,
            pele
        );
        DrawCircle(x - 47, y + 88, 12, luva);
        DrawCircle(x + 47, y + 88, 12, luva);
    }
}

static void desenhar_arena(void) {
    Color parede = (Color){35, 38, 44, 255};
    Color piso_fundo = (Color){77, 72, 67, 255};
    Color piso_frente = (Color){52, 49, 47, 255};
    Color corda = (Color){195, 190, 180, 255};

    ClearBackground((Color){22, 24, 29, 255});

    // Fundo escuro da arena.
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, parede);

    // Luzes e detalhe de fundo.
    DrawRectangleGradientV(
        245, 0, 470, 275,
        (Color){80, 76, 68, 255},
        parede
    );

    // Piso em perspectiva: estreito ao fundo e largo na frente.
    DrawTriangle(
        (Vector2){170, 190},
        (Vector2){790, 190},
        (Vector2){LARGURA_TELA, 430},
        piso_fundo
    );
    DrawTriangle(
        (Vector2){170, 190},
        (Vector2){LARGURA_TELA, 430},
        (Vector2){0, 430},
        piso_frente
    );

    // Linhas do piso convergem para o fundo do ringue.
    DrawLineEx((Vector2){170, 190}, (Vector2){0, 430}, 5, corda);
    DrawLineEx((Vector2){790, 190}, (Vector2){LARGURA_TELA, 430}, 5, corda);
    DrawLineEx((Vector2){170, 190}, (Vector2){790, 190}, 5, corda);
    DrawLineEx((Vector2){65, 365}, (Vector2){895, 365}, 3, (Color){125, 120, 112, 255});

    // Marcas simples no piso.
    DrawLine(185, 245, 775, 245, (Color){91, 85, 78, 255});
    DrawLine(125, 310, 835, 310, (Color){86, 80, 74, 255});

    // Cordas traseiras do ringue.
    DrawLineEx((Vector2){110, 162}, (Vector2){850, 162}, 5, corda);
    DrawLineEx((Vector2){68, 197}, (Vector2){892, 197}, 4, (Color){155, 150, 143, 255});
    DrawLineEx((Vector2){27, 235}, (Vector2){933, 235}, 4, (Color){130, 126, 120, 255});

    // Postes laterais.
    DrawRectangle(45, 145, 13, 137, (Color){105, 35, 35, 255});
    DrawRectangle(902, 145, 13, 137, (Color){105, 35, 35, 255});

    // Vinheta inferior para dar profundidade e separar a área de controles.
    DrawRectangleGradientV(
        0, 405, LARGURA_TELA, 135,
        (Color){20, 21, 24, 0},
        (Color){20, 21, 24, 230}
    );
}

static void reiniciar_combate(
    Player *jogador,
    Boss *boss,
    int numero_boss,
    EstadoBoss *estado_boss,
    float *tempo_estado,
    float *tempo_soco,
    float *tempo_recuperacao_soco,
    float *tempo_contra_ataque,
    int *quantidade_ataques,
    int *soco_esquerdo,
    int *soco_direito,
    int *esquiva_perfeita,
    int *resultado
) {
    iniciar_boss(boss, numero_boss);

    jogador->hp = PLAYER_VIDA;
    jogador->dano = PLAYER_DANO_BASE;
    jogador->defendendo = 0;
    jogador->esquivando = 0;
    jogador->cooldown_defesa = 0;
    jogador->cooldown_esquiva = 0;

    *estado_boss = BOSS_GUARDANDO;
    *tempo_estado = 0;
    *tempo_soco = 0;
    *tempo_recuperacao_soco = 0;
    *tempo_contra_ataque = 0;
    *quantidade_ataques = 0;
    *soco_esquerdo = 0;
    *soco_direito = 0;
    *esquiva_perfeita = 0;
    *resultado = COMBATE_EM_ANDAMENTO;
}

int main(void) {
    Player jogador;
    Boss boss;

    player_iniciar(&jogador);

    InitWindow(LARGURA_TELA, ALTURA_TELA, "Beyond the Fallen");
    SetTargetFPS(60);

    Tela tela = TELA_COMBATE;
    EstadoBoss estado_boss = BOSS_GUARDANDO;

    DirecaoAtaque direcao_ataque = ATAQUE_ESQUERDA;
    DirecaoEsquiva direcao_esquiva = ESQUIVA_NENHUMA;

    int numero_boss = 1;
    int resultado = COMBATE_EM_ANDAMENTO;
    int quantidade_ataques = 0;
    int esquiva_perfeita = 0;

    float tempo_estado = 0;
    float tempo_soco = 0;
    float tempo_recuperacao_soco = 0;
    float tempo_contra_ataque = 0;
    float intervalo_ataque = 2.0f;

    const float duracao_preparacao = 0.9f;
    const float duracao_recuperacao = 1.5f;

    int soco_esquerdo = 0;
    int soco_direito = 0;

    reiniciar_combate(
        &jogador,
        &boss,
        numero_boss,
        &estado_boss,
        &tempo_estado,
        &tempo_soco,
        &tempo_recuperacao_soco,
        &tempo_contra_ataque,
        &quantidade_ataques,
        &soco_esquerdo,
        &soco_direito,
        &esquiva_perfeita,
        &resultado
    );

    intervalo_ataque = 2.8f - boss.velocidade * 0.15f;
    if (intervalo_ataque < 1.4f) {
        intervalo_ataque = 1.4f;
    }

    while (!WindowShouldClose()) {
        float tempo = GetFrameTime();

        if (tela == TELA_COMBATE) {
            combat_atualizar(&jogador, tempo);

            if (tempo_recuperacao_soco > 0) {
                tempo_recuperacao_soco -= tempo;
                if (tempo_recuperacao_soco < 0) {
                    tempo_recuperacao_soco = 0;
                }
            }

            if (tempo_contra_ataque > 0) {
                tempo_contra_ataque -= tempo;
                if (tempo_contra_ataque < 0) {
                    tempo_contra_ataque = 0;
                }
            }

            if (tempo_soco > 0) {
                tempo_soco -= tempo;
                if (tempo_soco <= 0) {
                    tempo_soco = 0;
                    soco_esquerdo = 0;
                    soco_direito = 0;
                }
            }

            // Socos acertam quando o chefe está se recuperando.
            if ((IsKeyPressed(KEY_J) || IsKeyPressed(KEY_K)) &&
                tempo_recuperacao_soco <= 0 &&
                jogador.hp > 0 &&
                boss.hp > 0 &&
                estado_boss == BOSS_RECUPERANDO) {

                soco_esquerdo = IsKeyPressed(KEY_J);
                soco_direito = IsKeyPressed(KEY_K);
                tempo_soco = 0.2f;
                tempo_recuperacao_soco = 0.35f;

                int dano_original = jogador.dano;

                if (tempo_contra_ataque > 0 && esquiva_perfeita) {
                    jogador.dano = PLAYER_DANO_BASE * 2;
                    tempo_contra_ataque = 0;
                    esquiva_perfeita = 0;
                }

                combat_atacar(&jogador, &boss);
                jogador.dano = dano_original;
            }

            if (IsKeyPressed(KEY_S)) {
                combat_defender(&jogador);
            }

            if (IsKeyPressed(KEY_A) &&
                jogador.cooldown_esquiva <= 0) {
                combat_esquivar(&jogador);
                direcao_esquiva = ESQUIVA_ESQUERDA;
            }

            if (IsKeyPressed(KEY_D) &&
                jogador.cooldown_esquiva <= 0) {
                combat_esquivar(&jogador);
                direcao_esquiva = ESQUIVA_DIREITA;
            }

            tempo_estado += tempo;

            switch (estado_boss) {
                case BOSS_GUARDANDO:
                    if (tempo_estado >= intervalo_ataque) {
                        estado_boss = BOSS_PREPARANDO;
                        tempo_estado = 0;
                        direcao_esquiva = ESQUIVA_NENHUMA;
                        direcao_ataque =
                            GetRandomValue(0, 1) == 0
                                ? ATAQUE_ESQUERDA
                                : ATAQUE_DIREITA;
                    }
                    break;

                case BOSS_PREPARANDO:
                    if (tempo_estado >= duracao_preparacao) {
                        quantidade_ataques++;

                        int esquivou_corretamente =
                            (direcao_ataque == ATAQUE_ESQUERDA &&
                             direcao_esquiva == ESQUIVA_DIREITA) ||
                            (direcao_ataque == ATAQUE_DIREITA &&
                             direcao_esquiva == ESQUIVA_ESQUERDA);

                        if (esquivou_corretamente) {
                            jogador.esquivando = 0;
                            jogador.cooldown_esquiva = COOLDOWN_ESQUIVA;
                            esquiva_perfeita = 1;
                            tempo_contra_ataque = 0.8f;
                        } else {
                            jogador.esquivando = 0;

                            int dano_original = boss.dano;

                            if (numero_boss == 4 &&
                                quantidade_ataques % 3 == 0) {
                                boss.dano = ANASTASIA_DANO_ESPECIAL;
                            }

                            combat_boss_atacar(&boss, &jogador);
                            boss.dano = dano_original;

                            esquiva_perfeita = 0;
                            tempo_contra_ataque = 0;
                        }

                        direcao_esquiva = ESQUIVA_NENHUMA;
                        estado_boss = BOSS_RECUPERANDO;
                        tempo_estado = 0;
                    }
                    break;

                case BOSS_RECUPERANDO:
                    if (tempo_estado >= duracao_recuperacao) {
                        estado_boss = BOSS_GUARDANDO;
                        tempo_estado = 0;
                        esquiva_perfeita = 0;
                        tempo_contra_ataque = 0;
                    }
                    break;
            }

            resultado = combat_verificar_fim(&jogador, &boss);
            if (resultado != COMBATE_EM_ANDAMENTO) {
                tela = TELA_FINAL;
            }
        } else if (tela == TELA_FINAL) {
            if (resultado == COMBATE_VITORIA &&
                numero_boss < 4 &&
                IsKeyPressed(KEY_ENTER)) {

                numero_boss++;

                reiniciar_combate(
                    &jogador,
                    &boss,
                    numero_boss,
                    &estado_boss,
                    &tempo_estado,
                    &tempo_soco,
                    &tempo_recuperacao_soco,
                    &tempo_contra_ataque,
                    &quantidade_ataques,
                    &soco_esquerdo,
                    &soco_direito,
                    &esquiva_perfeita,
                    &resultado
                );

                direcao_esquiva = ESQUIVA_NENHUMA;

                intervalo_ataque = 2.8f - boss.velocidade * 0.15f;
                if (intervalo_ataque < 1.4f) {
                    intervalo_ataque = 1.4f;
                }

                tela = TELA_COMBATE;
            } else if (resultado == COMBATE_DERROTA &&
                       IsKeyPressed(KEY_R)) {

                reiniciar_combate(
                    &jogador,
                    &boss,
                    numero_boss,
                    &estado_boss,
                    &tempo_estado,
                    &tempo_soco,
                    &tempo_recuperacao_soco,
                    &tempo_contra_ataque,
                    &quantidade_ataques,
                    &soco_esquerdo,
                    &soco_direito,
                    &esquiva_perfeita,
                    &resultado
                );

                direcao_esquiva = ESQUIVA_NENHUMA;
                tela = TELA_COMBATE;
            }
        }

        BeginDrawing();

        if (tela == TELA_COMBATE) {
            desenhar_arena();

            DrawText("BEYOND THE FALLEN", 326, 28, 28, RAYWHITE);

            // Vida do jogador.
            DrawText(jogador.nome, 55, 77, 22, YELLOW);
            DrawText(TextFormat("%d", jogador.hp), 55, 105, 19, RAYWHITE);
            DrawRectangle(95, 111, 260, 17, DARKGRAY);

            int hp_jogador = jogador.hp;
            if (hp_jogador < 0) hp_jogador = 0;
            if (hp_jogador > PLAYER_VIDA) hp_jogador = PLAYER_VIDA;

            DrawRectangle(
                95, 111,
                (int)(260.0f * hp_jogador / PLAYER_VIDA),
                17,
                (Color){35, 210, 75, 255}
            );

            // Vida do chefe.
            int hp_maximo_boss = obter_vida_maxima_boss(numero_boss);
            int hp_boss = boss.hp;

            if (hp_boss < 0) hp_boss = 0;
            if (hp_boss > hp_maximo_boss) hp_boss = hp_maximo_boss;

            DrawText(boss.nome, 650, 77, 22, RED);
            DrawText(TextFormat("%d", boss.hp), 650, 105, 19, RAYWHITE);
            DrawRectangle(690, 111, 220, 17, DARKGRAY);
            DrawRectangle(
                690, 111,
                (int)(220.0f * hp_boss / hp_maximo_boss),
                17,
                (Color){220, 45, 55, 255}
            );

            Color roupa_boss = (Color){155, 45, 45, 255};

            if (estado_boss == BOSS_PREPARANDO) {
                roupa_boss = (Color){205, 85, 40, 255};
            } else if (estado_boss == BOSS_RECUPERANDO) {
                roupa_boss = (Color){115, 55, 55, 255};
            }

            // Sombras no piso para dar profundidade.
            DrawEllipse(480, 386, 37, 9, (Color){15, 16, 18, 120});
            DrawEllipse(480, 505, 92, 15, (Color){15, 16, 18, 150});

            // Chefe ao fundo, de frente para o jogador.
            desenhar_boss_de_frente(
                480,
                157,
                roupa_boss,
                (Color){218, 178, 132, 255},
                estado_boss == BOSS_PREPARANDO,
                estado_boss == BOSS_PREPARANDO
            );

            // Jogador em primeiro plano, visto de costas.
            int direcao_visual_esquiva = 0;
            if (direcao_esquiva == ESQUIVA_ESQUERDA) {
                direcao_visual_esquiva = -1;
            } else if (direcao_esquiva == ESQUIVA_DIREITA) {
                direcao_visual_esquiva = 1;
            }

            desenhar_jogador_de_costas(
                480,
                270,
                tempo_soco > 0 && (soco_esquerdo || soco_direito),
                jogador.defendendo,
                jogador.esquivando,
                direcao_visual_esquiva
            );

            if (jogador.defendendo) {
                DrawEllipseLines(
                    480, 386, 91, 113,
                    (Color){80, 170, 255, 180}
                );
            }

            // Controles.
            DrawText("J/K: SOCO", 95, 475, 18, RAYWHITE);
            DrawText("S: DEFESA", 400, 475, 18, RAYWHITE);
            DrawText("A/D: ESQUIVA", 700, 475, 18, RAYWHITE);

            DrawText(
                TextFormat("ESQUIVA %.1f", jogador.cooldown_esquiva),
                95, 505, 16,
                (Color){190, 195, 205, 255}
            );
            DrawText(
                TextFormat("DEFESA %.1f", jogador.cooldown_defesa),
                400, 505, 16,
                (Color){190, 195, 205, 255}
            );
        } else if (tela == TELA_FINAL) {
            ClearBackground((Color){22, 24, 29, 255});

            if (resultado == COMBATE_VITORIA && numero_boss < 4) {
                DrawText("VITORIA", 365, 170, 52, GREEN);
                DrawText(
                    "ENTER: PRÓXIMO COMBATE",
                    285, 275, 22, RAYWHITE
                );
            } else if (resultado == COMBATE_VITORIA) {
                DrawText(
                    "CAMPEÃO",
                    360, 170, 52,
                    (Color){255, 205, 55, 255}
                );
                DrawText(
                    "VOCÊ VENCEU A CAMPANHA",
                    280, 255, 24, RAYWHITE
                );
            } else if (resultado == COMBATE_DERROTA) {
                DrawText("DERROTA", 365, 170, 52, RED);
                DrawText(
                    "R: TENTAR NOVAMENTE",
                    320, 275, 22, RAYWHITE
                );
            } else {
                DrawText("EMPATE", 380, 170, 52, YELLOW);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}