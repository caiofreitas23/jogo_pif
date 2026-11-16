
#include "raylib.h"
#include "player.h"
#include "boss.h"
#include "combat.h"
#include "config.h"

typedef enum {
    TELA_SELECAO,
    TELA_COMBATE,
    TELA_FINAL
} Tela;

typedef enum {
    BOSS_GUARDANDO,
    BOSS_AVISANDO,
    BOSS_RECUPERANDO,
    BOSS_ATORDOADO
} EstadoBoss;

void iniciar_boss(Boss *b, int escolha) {
    switch (escolha) {
        case 1:
            boss_iniciar_bobrovsky(b);
            break;

        case 2:
            boss_iniciar_vladimir(b);
            break;

        case 3:
            boss_iniciar_ivan(b);
            break;

        case 4:
            boss_iniciar_anastasia(b);
            break;
    }
}

int main(void) {

    Player jogador;
    Boss boss;

    player_iniciar(&jogador);

    InitWindow(LARGURA_TELA, ALTURA_TELA, "Beyond the Fallen");

    SetTargetFPS(60);

    Tela tela = TELA_SELECAO;
    EstadoBoss estado_boss = BOSS_GUARDANDO;

    int escolha = 0;
    int resultado = COMBATE_EM_ANDAMENTO;
    int quantidade_ataques = 0;

    float tempo_estado = 0;
    float tempo_soco = 0;

    int soco_esquerdo = 0;
    int soco_direito = 0;

    float intervalo_ataque = 2.0f;
    float duracao_aviso = 0.9f;
    float duracao_recuperacao = 1.5f;

    while (!WindowShouldClose()) {

        float tempo = GetFrameTime();

        if (tela == TELA_SELECAO) {

            if (IsKeyPressed(KEY_ONE)) escolha = 1;
            if (IsKeyPressed(KEY_TWO)) escolha = 2;
            if (IsKeyPressed(KEY_THREE)) escolha = 3;
            if (IsKeyPressed(KEY_FOUR)) escolha = 4;

            if (IsKeyPressed(KEY_ENTER) && escolha != 0) {

                iniciar_boss(&boss, escolha);

                jogador.hp = PLAYER_VIDA;
                jogador.dano = PLAYER_DANO_BASE;
                jogador.defendendo = 0;
                jogador.esquivando = 0;
                jogador.cooldown_defesa = 0;
                jogador.cooldown_esquiva = 0;

                estado_boss = BOSS_GUARDANDO;
                tempo_estado = 0;
                tempo_soco = 0;
                quantidade_ataques = 0;

                soco_esquerdo = 0;
                soco_direito = 0;

                resultado = COMBATE_EM_ANDAMENTO;

                intervalo_ataque = 2.8f - boss.velocidade * 0.15f;

                if (intervalo_ataque < 1.4f) {
                    intervalo_ataque = 1.4f;
                }

                tela = TELA_COMBATE;
            }
        }

        // TELA COMBATE

        else if (tela == TELA_COMBATE) {

            combat_atualizar(&jogador, tempo);

            tempo_estado += tempo;

            // TECLAS DE ATAQUE JOGADOR

            if (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_K)) {

                soco_esquerdo = IsKeyPressed(KEY_J);
                soco_direito = IsKeyPressed(KEY_K);
                tempo_soco = 0.2f;

                // CONTRA-ATAQUE
                if (estado_boss == BOSS_AVISANDO ||
                    estado_boss == BOSS_RECUPERANDO ||
                    estado_boss == BOSS_ATORDOADO) {

                    combat_atacar(&jogador, &boss);

                    // SE ACERTAR DURANTE O AVISO, INTERROMPE O ATAQUE
                    if (estado_boss == BOSS_AVISANDO) {
                        estado_boss = BOSS_ATORDOADO;
                        tempo_estado = 0;
                    }
                }
            }

            // DEFESA E ESQUIVA

            if (IsKeyPressed(KEY_S)) {
                combat_defender(&jogador);
            }

            if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_D)) {
                combat_esquivar(&jogador);
            }

            // TEMPO DOS SOCOS

            if (tempo_soco > 0) {
                tempo_soco -= tempo;

                if (tempo_soco <= 0) {
                    soco_esquerdo = 0;
                    soco_direito = 0;
                }
            }

            // COMPORTAMENTO DO CHEFE

            switch (estado_boss) {

                case BOSS_GUARDANDO:

                    if (tempo_estado >= intervalo_ataque) {
                        estado_boss = BOSS_AVISANDO;
                        tempo_estado = 0;
                    }

                    break;

                case BOSS_AVISANDO:

                    if (tempo_estado >= duracao_aviso) {

                        quantidade_ataques++;

                        // Anastasia realiza um especial a cada terceiro ataque.
                        if (escolha == 4 && quantidade_ataques % 3 == 0) {
                            int dano_original = boss.dano;

                            boss.dano = ANASTASIA_DANO_ESPECIAL;
                            combat_boss_atacar(&boss, &jogador);
                            boss.dano = dano_original;
                        } else {
                            combat_boss_atacar(&boss, &jogador);
                        }

                        estado_boss = BOSS_RECUPERANDO;
                        tempo_estado = 0;
                    }

                    break;

                case BOSS_RECUPERANDO:

                    if (tempo_estado >= duracao_recuperacao) {
                        estado_boss = BOSS_GUARDANDO;
                        tempo_estado = 0;
                    }

                    break;

                case BOSS_ATORDOADO:

                    if (tempo_estado >= 1.5f) {
                        estado_boss = BOSS_GUARDANDO;
                        tempo_estado = 0;
                    }

                    break;
            }

            resultado = combat_verificar_fim(&jogador, &boss);

            if (resultado != COMBATE_EM_ANDAMENTO) {
                tela = TELA_FINAL;
            }
        }

        // REINICIAR OU VOLTAR A SELECAO

        else if (tela == TELA_FINAL) {

            if (IsKeyPressed(KEY_R)) {

                iniciar_boss(&boss, escolha);

                jogador.hp = PLAYER_VIDA;
                jogador.defendendo = 0;
                jogador.esquivando = 0;
                jogador.cooldown_defesa = 0;
                jogador.cooldown_esquiva = 0;

                estado_boss = BOSS_GUARDANDO;
                tempo_estado = 0;
                tempo_soco = 0;
                quantidade_ataques = 0;

                resultado = COMBATE_EM_ANDAMENTO;
                tela = TELA_COMBATE;
            }

            if (IsKeyPressed(KEY_BACKSPACE)) {
                escolha = 0;
                tela = TELA_SELECAO;
            }
        }

        BeginDrawing();

        ClearBackground((Color){25, 27, 32, 255});

        if (tela == TELA_SELECAO) {

            DrawText("BEYOND THE FALLEN", 285, 55, 35, WHITE);
            DrawText("SELECIONE SEU OPONENTE", 310, 125, 25, YELLOW);

            DrawText("1 - BOBROVSKY", 180, 220, 25, WHITE);
            DrawText("150 HP | Dano: 15", 210, 255, 18, LIGHTGRAY);

            DrawText("2 - VLADIMIR", 550, 220, 25, WHITE);
            DrawText("250 HP | Dano: 20", 580, 255, 18, LIGHTGRAY);

            DrawText("3 - IVAN", 180, 330, 25, WHITE);
            DrawText("400 HP | Dano: 40", 210, 365, 18, LIGHTGRAY);

            DrawText("4 - ANASTASIA", 550, 330, 25, WHITE);
            DrawText("700 HP | Dano: 50", 580, 365, 18, LIGHTGRAY);

            DrawText("APERTE 1, 2, 3 OU 4 E DEPOIS ENTER",
                     230, 460, 20, YELLOW);

            if (escolha != 0) {
                DrawText(TextFormat("Selecionado: %s",
                         escolha == 1 ? "Bobrovsky" :
                         escolha == 2 ? "Vladimir" :
                         escolha == 3 ? "Ivan" : "Anastasia"),
                         350, 420, 20, GREEN);
            }
        }

        else if (tela == TELA_COMBATE) {

            // ARENA
            DrawRectangle(0, 390, 960, 150, (Color){65, 60, 55, 255});
            DrawRectangle(0, 390, 960, 8, GRAY);

            DrawRectangle(50, 110, 860, 280, (Color){40, 43, 48, 255});
            DrawRectangleLines(50, 110, 860, 280, GRAY);

            DrawText("BEYOND THE FALLEN", 350, 20, 25, WHITE);

            // VIDA JOGADOR
            DrawText(jogador.nome, 100, 100, 22, YELLOW);

            DrawText(TextFormat("HP: %d", jogador.hp),
                     100, 130, 18, WHITE);

            DrawRectangle(100, 158, 250, 18, DARKGRAY);

            DrawRectangle(100, 158,
                (int)(250.0f * jogador.hp / PLAYER_VIDA),
                18, GREEN);

            // VIDA CHEFE
            DrawText(boss.nome, 690, 100, 22, RED);

            DrawText(TextFormat("HP: %d", boss.hp),
                     690, 130, 18, WHITE);

            DrawRectangle(690, 158, 200, 18, DARKGRAY);

            int largura_barra = (int)(200.0f * boss.hp /
                (escolha == 1 ? BOBROVSKY_VIDA :
                 escolha == 2 ? VLADIMIR_VIDA :
                 escolha == 3 ? IVAN_VIDA : ANASTASIA_VIDA));

            DrawRectangle(690, 158, largura_barra, 18, RED);

            // JOGADOR
            DrawRectangle(190, 275, 65, 105, BLUE);
            DrawCircle(222, 255, 28, BEIGE);

            if (soco_esquerdo) {
                DrawCircle(275, 300, 20, YELLOW);
            }

            if (soco_direito) {
                DrawCircle(275, 340, 20, YELLOW);
            }

            // CHEFE
            Color cor_boss = (Color){150, 40, 40, 255};

            if (estado_boss == BOSS_AVISANDO) {
                cor_boss = ORANGE;
            }

            if (estado_boss == BOSS_RECUPERANDO) {
                cor_boss = GRAY;
            }

            if (estado_boss == BOSS_ATORDOADO) {
                cor_boss = YELLOW;
            }

            DrawRectangle(690, 275, 65, 105, cor_boss);
            DrawCircle(722, 255, 28, BEIGE);

            // MENSAGENS
            if (estado_boss == BOSS_GUARDANDO) {
                DrawText("O CHEFE ESTA OBSERVANDO...",
                         320, 205, 20, LIGHTGRAY);
            }

            if (estado_boss == BOSS_AVISANDO) {
                DrawText("CUIDADO! O CHEFE VAI ATACAR!",
                         290, 205, 22, ORANGE);
            }

            if (estado_boss == BOSS_RECUPERANDO) {
                DrawText("CHEFE VULNERAVEL! ATAQUE!",
                         300, 205, 22, GREEN);
            }

            if (estado_boss == BOSS_ATORDOADO) {
                DrawText("CHEFE ATORDOADO!",
                         350, 205, 22, YELLOW);
            }

            // ESTADO DO JOGADOR
            if (jogador.defendendo) {
                DrawText("DEFENDENDO", 100, 190, 18, SKYBLUE);
            }

            if (jogador.esquivando) {
                DrawText("ESQUIVANDO", 100, 190, 18, SKYBLUE);
            }

            DrawText(TextFormat("Defesa: %.1f s",
                     jogador.cooldown_defesa),
                     100, 435, 17, WHITE);

            DrawText(TextFormat("Esquiva: %.1f s",
                     jogador.cooldown_esquiva),
                     100, 460, 17, WHITE);

            DrawText("J/K: SOCAR | S: DEFENDER | A/D: ESQUIVAR",
                     270, 495, 18, WHITE);
        }

        else if (tela == TELA_FINAL) {

            if (resultado == COMBATE_VITORIA) {
                DrawText("VITORIA!", 370, 170, 55, GREEN);
                DrawText("Voce derrotou o chefe!", 315, 245, 25, WHITE);
            }

            else if (resultado == COMBATE_DERROTA) {
                DrawText("DERROTA!", 370, 170, 55, RED);
                DrawText("Voce foi derrotado!", 335, 245, 25, WHITE);
            }

            else {
                DrawText("EMPATE!", 385, 170, 55, YELLOW);
            }

            DrawText("R - TENTAR NOVAMENTE",
                     320, 330, 22, WHITE);

            DrawText("BACKSPACE - VOLTAR A SELECAO",
                     275, 375, 20, LIGHTGRAY);
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
