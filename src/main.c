#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "player.h"
#include "boss.h"
#include "combat.h"
#include "config.h"

#define ARQUIVO_PLACAR "placar.txt"
#define MAXIMO_REGISTROS_PLACAR 10
#define MENU_COMECAR 0
#define MENU_PLACAR 1
#define MENU_SAIR 2
#define MENU_TOTAL 3

typedef enum {
    TELA_MENU,
    TELA_PLACAR,
    TELA_TRANSICAO,
    TELA_COMBATE,
    TELA_FINAL,
    TELA_GAME_OVER
} Tela;

typedef enum {
    BOSS_GUARDANDO,
    BOSS_PREPARANDO,
    BOSS_ATACANDO,
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

typedef struct {
    Rectangle hurtbox_jogador;
    Rectangle hitbox_soco_jogador;
    Rectangle hurtbox_boss;
    Rectangle hitbox_guarda_boss;
    Rectangle hitbox_ataque_boss;
} HitboxesCombate;

typedef struct {
    char jogador[32];
    int pontuacao;
    int chefes_derrotados;
    int golpes_acertados;
    int esquivas_perfeitas;
    int golpes_recebidos;
    char resultado[32];
} RegistroPlacar;

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

static float calcular_intervalo_ataque(const Boss *boss) {
    float intervalo = 2.8f - (float)boss->velocidade * 0.15f;
    if (intervalo < 1.4f) {
        intervalo = 1.4f;
    }
    return intervalo;
}

static void desenhar_texto_centralizado(const char *texto, int y, int tamanho, Color cor) {
    int largura = MeasureText(texto, tamanho);
    DrawText(texto, (LARGURA_TELA - largura) / 2, y, tamanho, cor);
}

/*
 * Remove espaços extras nas extremidades de uma string lida do arquivo.
 */
static void limpar_espacos(char *texto) {
    int inicio = 0;
    int fim = (int)strlen(texto) - 1;

    while (texto[inicio] == ' ' || texto[inicio] == '\t') {
        inicio++;
    }
    while (fim >= inicio && (texto[fim] == ' ' || texto[fim] == '\t' || texto[fim] == '\r' || texto[fim] == '\n')) {
        fim--;
    }

    int len = fim - inicio + 1;
    if (len > 0) {
        memmove(texto, texto + inicio, (size_t)len);
        texto[len] = '\0';
    } else {
        texto[0] = '\0';
    }
}

/*
 * Carrega os registros existentes de placar.txt para manipulação em memória.
 */
static int carregar_placar_txt(const char *caminho, RegistroPlacar lista[], int max_itens) {
    FILE *arquivo = fopen(caminho, "r");
    if (!arquivo) {
        return 0;
    }

    char linha[256];
    int total = 0;

    while (fgets(linha, sizeof(linha), arquivo) && total < max_itens) {
        int pos = 0;
        RegistroPlacar item = {0};

        if (sscanf(
                linha,
                " %d | %31[^|] | %d | %d/%*d | %d | %d | %d | %31[^\r\n]",
                &pos,
                item.jogador,
                &item.pontuacao,
                &item.chefes_derrotados,
                &item.golpes_acertados,
                &item.esquivas_perfeitas,
                &item.golpes_recebidos,
                item.resultado
            ) == 8) {
            limpar_espacos(item.jogador);
            limpar_espacos(item.resultado);
            lista[total++] = item;
        }
    }

    fclose(arquivo);
    return total;
}

static void ordenar_placar(RegistroPlacar lista[], int total) {
    for (int i = 0; i < total - 1; i++) {
        for (int j = 0; j < total - i - 1; j++) {
            int trocar = 0;
            if (lista[j].pontuacao < lista[j + 1].pontuacao) {
                trocar = 1;
            } else if (lista[j].pontuacao == lista[j + 1].pontuacao &&
                       lista[j].chefes_derrotados < lista[j + 1].chefes_derrotados) {
                trocar = 1;
            }

            if (trocar) {
                RegistroPlacar temp = lista[j];
                lista[j] = lista[j + 1];
                lista[j + 1] = temp;
            }
        }
    }
}

/*
 * Imprime o placar manipulado e ordenado no arquivo placar.txt.
 */
static void gravar_placar_txt(const char *caminho, const RegistroPlacar lista[], int total) {
    FILE *arquivo = fopen(caminho, "w");
    if (!arquivo) {
        return;
    }

    fprintf(arquivo, "====================================================================================\n");
    fprintf(arquivo, "                        BEYOND THE FALLEN - PLACAR OFICIAL                          \n");
    fprintf(arquivo, "====================================================================================\n");
    fprintf(arquivo, "POS | JOGADOR        | PONTOS | CHEFES | ACERTOS | ESQUIVAS | SOFRIDOS | RESULTADO  \n");
    fprintf(arquivo, "------------------------------------------------------------------------------------\n");

    for (int i = 0; i < total; i++) {
        fprintf(
            arquivo,
            "%3d | %-14.14s | %6d |   %d/4  | %7d | %8d | %8d | %-11.11s\n",
            i + 1,
            lista[i].jogador,
            lista[i].pontuacao,
            lista[i].chefes_derrotados,
            lista[i].golpes_acertados,
            lista[i].esquivas_perfeitas,
            lista[i].golpes_recebidos,
            lista[i].resultado
        );
    }

    fprintf(arquivo, "====================================================================================\n");
    fclose(arquivo);
}

/*
 * Atualiza a entrada da partida atual no ranking, ordena e grava em placar.txt.
 * Retorna a posição atual do jogador no ranking (1..MAXIMO_REGISTROS_PLACAR).
 */
static int sincronizar_placar_txt(
    const char *caminho,
    const RegistroPlacar *partida_atual,
    int *indice_salvo
) {
    RegistroPlacar lista[MAXIMO_REGISTROS_PLACAR + 1];
    int total = carregar_placar_txt(caminho, lista, MAXIMO_REGISTROS_PLACAR);

    if (*indice_salvo >= 0 && *indice_salvo < total) {
        lista[*indice_salvo] = *partida_atual;
    } else if (total < MAXIMO_REGISTROS_PLACAR) {
        *indice_salvo = total;
        lista[total++] = *partida_atual;
    } else {
        lista[total] = *partida_atual;
        total++;
    }

    ordenar_placar(lista, total);
    if (total > MAXIMO_REGISTROS_PLACAR) {
        total = MAXIMO_REGISTROS_PLACAR;
    }

    int posicao = 1;
    *indice_salvo = -1;
    for (int i = 0; i < total; i++) {
        if (strcmp(lista[i].jogador, partida_atual->jogador) == 0 &&
            lista[i].pontuacao == partida_atual->pontuacao &&
            lista[i].chefes_derrotados == partida_atual->chefes_derrotados &&
            lista[i].golpes_acertados == partida_atual->golpes_acertados &&
            strcmp(lista[i].resultado, partida_atual->resultado) == 0) {
            *indice_salvo = i;
            posicao = i + 1;
            break;
        }
    }

    gravar_placar_txt(caminho, lista, total);
    return posicao;
}

/*
 * Calcula todas as hitboxes e hurtboxes do combate com base nas posições,
 * porte físico do chefe atual e estados dos lutadores na arena.
 */
static HitboxesCombate calcular_hitboxes_combate(
    int centro_x,
    int jogador_topo_y,
    int boss_topo_y,
    int numero_boss,
    int jogador_esquivando,
    int direcao_visual_esquiva,
    int jogador_atacando,
    int soco_esquerdo,
    int soco_direito,
    EstadoBoss estado_boss,
    DirecaoAtaque direcao_ataque
) {
    HitboxesCombate hb = {0};

    int dx_jogador = jogador_esquivando ? (direcao_visual_esquiva * 55) : 0;
    int dy_jogador = jogador_esquivando ? 10 : 0;
    int jx = centro_x + dx_jogador;
    int jy = jogador_topo_y + dy_jogador;

    // Hurtbox do corpo/cabeça do jogador.
    hb.hurtbox_jogador = (Rectangle){
        (float)(jx - 44),
        (float)(jy + 22),
        88.0f,
        175.0f
    };

    // Hitbox do soco do jogador quando ativo.
    if (jogador_atacando) {
        if (soco_esquerdo) {
            hb.hitbox_soco_jogador = (Rectangle){
                (float)(jx - 34),
                (float)(jy - 48),
                36.0f,
                56.0f
            };
        } else if (soco_direito) {
            hb.hitbox_soco_jogador = (Rectangle){
                (float)(jx - 2),
                (float)(jy - 48),
                36.0f,
                56.0f
            };
        }
    }

    // Hurtbox do chefe ajustada ao porte de cada personagem.
    float meia_largura_boss = 44.0f;
    float altura_boss = 112.0f;

    if (numero_boss == 1) {
        meia_largura_boss = 54.0f;
    } else if (numero_boss == 2) {
        meia_largura_boss = 30.0f;
    } else if (numero_boss == 4) {
        meia_largura_boss = 58.0f;
        altura_boss = 145.0f;
    }

    hb.hurtbox_boss = (Rectangle){
        (float)centro_x - meia_largura_boss,
        (float)(boss_topo_y + 2),
        meia_largura_boss * 2.0f,
        altura_boss
    };

    // Hitbox da guarda do chefe (bloqueia socos frontais quando em guarda).
    if (estado_boss == BOSS_GUARDANDO) {
        hb.hitbox_guarda_boss = (Rectangle){
            (float)(centro_x - 36),
            (float)(boss_topo_y + 26),
            72.0f,
            36.0f
        };
    }

    // Hitbox do ataque do chefe (projeta-se para o lado do golpe até a altura do jogador).
    if (estado_boss == BOSS_ATACANDO) {
        if (direcao_ataque == ATAQUE_ESQUERDA) {
            hb.hitbox_ataque_boss = (Rectangle){
                (float)(centro_x - 95),
                (float)(boss_topo_y + 125),
                92.0f,
                95.0f
            };
        } else {
            hb.hitbox_ataque_boss = (Rectangle){
                (float)(centro_x + 3),
                (float)(boss_topo_y + 125),
                92.0f,
                95.0f
            };
        }
    }

    return hb;
}

/*
 * Desenha o jogador em primeiro plano, visto por trás.
 * O deslocamento horizontal representa a esquiva e desloca a hurtbox.
 */
static void desenhar_jogador_de_costas(
    int centro_x,
    int topo_y,
    int atacando,
    int soco_esquerdo,
    int defendendo,
    int esquivando,
    int direcao_esquiva
) {
    int deslocamento_x = 0;
    int deslocamento_y = 0;

    if (esquivando) {
        deslocamento_x = direcao_esquiva * 55;
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
    } else if (atacando && soco_esquerdo) {
        // Soco esquerdo projetado até a altura do rosto do chefe.
        DrawLineEx(
            (Vector2){(float)(x - 25), (float)(y + 122)},
            (Vector2){(float)(x - 16), (float)(y - 22)},
            23,
            roupa
        );
        DrawLineEx(
            (Vector2){(float)(x + 24), (float)(y + 123)},
            (Vector2){(float)(x + 53), (float)(y + 84)},
            21,
            roupa
        );
        DrawCircle(x - 16, y - 26, 19, luva);
        DrawCircle(x + 57, y + 80, 16, luva);
    } else if (atacando) {
        // Soco direito projetado até a altura do rosto do chefe.
        DrawLineEx(
            (Vector2){(float)(x + 25), (float)(y + 122)},
            (Vector2){(float)(x + 16), (float)(y - 22)},
            23,
            roupa
        );
        DrawLineEx(
            (Vector2){(float)(x - 24), (float)(y + 123)},
            (Vector2){(float)(x - 53), (float)(y + 84)},
            21,
            roupa
        );
        DrawCircle(x + 16, y - 26, 19, luva);
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
 * Desenho frontal do jogador usado nos cards das telas de transição.
 */
static void desenhar_jogador_card(int centro_x, int topo_y) {
    int x = centro_x;
    int y = topo_y;

    Color roupa = (Color){25, 75, 145, 255};
    Color detalhe = (Color){60, 130, 220, 255};
    Color pele = (Color){218, 178, 132, 255};
    Color luva = (Color){190, 35, 35, 255};
    Color calca = (Color){30, 40, 58, 255};

    // Pernas e botas.
    DrawRectangle(x - 24, y + 102, 18, 44, calca);
    DrawRectangle(x + 6, y + 102, 18, 44, calca);
    DrawRectangle(x - 28, y + 140, 23, 10, (Color){35, 31, 29, 255});
    DrawRectangle(x + 5, y + 140, 23, 10, (Color){35, 31, 29, 255});

    // Tronco.
    DrawRectangle(x - 36, y + 42, 72, 65, roupa);
    DrawRectangle(x - 36, y + 88, 72, 11, detalhe);

    // Cabeça e rosto.
    DrawCircle(x, y + 23, 24, pele);
    DrawRectangle(x - 24, y + 4, 48, 12, (Color){65, 43, 31, 255});
    DrawCircle(x - 8, y + 23, 2, BLACK);
    DrawCircle(x + 8, y + 23, 2, BLACK);
    DrawLine(x - 6, y + 33, x + 6, y + 33, (Color){95, 45, 38, 255});

    // Guarda de boxe.
    DrawLineEx((Vector2){(float)(x - 30), (float)(y + 60)}, (Vector2){(float)(x - 19), (float)(y + 40)}, 14, roupa);
    DrawLineEx((Vector2){(float)(x + 30), (float)(y + 60)}, (Vector2){(float)(x + 19), (float)(y + 40)}, 14, roupa);
    DrawCircle(x - 19, y + 38, 12, luva);
    DrawCircle(x + 19, y + 38, 12, luva);
}

static void desenhar_braco(int x1, int y1, int x2, int y2, int largura, Color cor) {
    DrawLineEx((Vector2){(float)x1, (float)y1}, (Vector2){(float)x2, (float)y2}, (float)largura, cor);
}

static void desenhar_estrela(int cx, int cy, int raio, Color cor) {
    Vector2 ponta[5];
    int i;

    for (i = 0; i < 5; i++) {
        float angulo = -PI / 2.0f + (float)i * 2.0f * PI / 5.0f;
        ponta[i].x = (float)cx + cosf(angulo) * (float)raio;
        ponta[i].y = (float)cy + sinf(angulo) * (float)raio;
    }

    for (i = 0; i < 5; i++) {
        DrawLineEx(ponta[i], ponta[(i + 2) % 5], 2, cor);
    }
}

static Color cor_uniforme(int numero_boss, EstadoBoss estado) {
    Color cor;
    int r;

    switch (numero_boss) {
        case 1: cor = (Color){96, 104, 112, 255}; break;   // uniforme de preso (Bobrovsky)
        case 2: cor = (Color){120, 84, 56, 255}; break;    // casaco velho (Vladimir)
        case 3: cor = (Color){70, 86, 62, 255}; break;     // uniforme de guarda (Ivan)
        default: cor = (Color){112, 74, 46, 255}; break;   // pelo da ursa (Anastasia)
    }

    if (estado == BOSS_PREPARANDO) {
        r = cor.r + 70;
        cor.r = (unsigned char)(r > 255 ? 255 : r);
        cor.g = (unsigned char)(cor.g + 10 > 255 ? 255 : cor.g + 10);
        cor.b = (unsigned char)(cor.b / 2);
    } else if (estado == BOSS_ATACANDO) {
        r = cor.r + 105;
        cor.r = (unsigned char)(r > 255 ? 255 : r);
        cor.g = (unsigned char)(cor.g / 2);
        cor.b = (unsigned char)(cor.b / 2);
    } else if (estado == BOSS_RECUPERANDO) {
        cor.r = (unsigned char)(cor.r * 7 / 10);
        cor.g = (unsigned char)(cor.g * 7 / 10);
        cor.b = (unsigned char)(cor.b * 7 / 10);
    }

    return cor;
}

static unsigned char clarear_canal(unsigned char v) {
    int r = v + 45;
    return (unsigned char)(r > 255 ? 255 : r);
}

static void desenhar_garras(int px, int py) {
    Color garra = (Color){235, 230, 215, 255};
    int k;

    for (k = -1; k <= 1; k++) {
        DrawLineEx(
            (Vector2){(float)(px + k * 7), (float)(py + 7)},
            (Vector2){(float)(px + k * 9), (float)(py + 21)},
            3, garra
        );
    }
}

/*
 * Skin da Anastasia (Chefe 4): ursa de pé vista de frente.
 */
static void desenhar_urso_de_frente(
    int centro_x,
    int topo_y,
    Color pelo,
    EstadoBoss estado_boss,
    int lado_ataque
) {
    int x = centro_x;
    int y = topo_y;

    Color escuro = (Color){
        (unsigned char)(pelo.r * 0.7f),
        (unsigned char)(pelo.g * 0.7f),
        (unsigned char)(pelo.b * 0.7f),
        255
    };
    Color claro = (Color){
        clarear_canal(pelo.r), clarear_canal(pelo.g), clarear_canal(pelo.b), 255
    };
    Color focinho = (Color){201, 165, 125, 255};
    Color almofada = (Color){70, 40, 40, 255};

    // Patas traseiras.
    DrawEllipse(x - 30, y + 158, 26, 15, escuro);
    DrawEllipse(x + 30, y + 158, 26, 15, escuro);
    DrawEllipse(x - 30, y + 162, 13, 7, almofada);
    DrawEllipse(x + 30, y + 162, 13, 7, almofada);

    // Corpo e barriga.
    DrawEllipse(x, y + 105, 58, 64, pelo);
    DrawEllipse(x, y + 114, 35, 44, claro);

    // Braços e patas dianteiras conforme o estado.
    if (estado_boss == BOSS_GUARDANDO) {
        DrawLineEx((Vector2){(float)(x - 48), (float)(y + 80)},
                   (Vector2){(float)(x - 26), (float)(y + 54)}, 26, pelo);
        DrawLineEx((Vector2){(float)(x + 48), (float)(y + 80)},
                   (Vector2){(float)(x + 26), (float)(y + 54)}, 26, pelo);
        DrawCircle(x - 24, y + 50, 16, escuro);
        DrawCircle(x + 24, y + 50, 16, escuro);
        desenhar_garras(x - 24, y + 50);
        desenhar_garras(x + 24, y + 50);
    } else if (estado_boss == BOSS_PREPARANDO) {
        DrawLineEx((Vector2){(float)(x + lado_ataque * 46), (float)(y + 76)},
                   (Vector2){(float)(x + lado_ataque * 92), (float)(y + 52)}, 26, pelo);
        DrawCircle(x + lado_ataque * 97, y + 49, 17, escuro);
        desenhar_garras(x + lado_ataque * 97, y + 49);

        DrawLineEx((Vector2){(float)(x - lado_ataque * 46), (float)(y + 80)},
                   (Vector2){(float)(x - lado_ataque * 60), (float)(y + 120)}, 24, pelo);
        DrawCircle(x - lado_ataque * 62, y + 125, 15, escuro);
        desenhar_garras(x - lado_ataque * 62, y + 125);
    } else if (estado_boss == BOSS_ATACANDO) {
        DrawLineEx((Vector2){(float)(x + lado_ataque * 44), (float)(y + 78)},
                   (Vector2){(float)(x + lado_ataque * 52), (float)(y + 172)}, 28, pelo);
        DrawCircle(x + lado_ataque * 52, y + 182, 22, escuro);
        desenhar_garras(x + lado_ataque * 52, y + 182);

        DrawLineEx((Vector2){(float)(x - lado_ataque * 46), (float)(y + 80)},
                   (Vector2){(float)(x - lado_ataque * 60), (float)(y + 120)}, 24, pelo);
        DrawCircle(x - lado_ataque * 62, y + 125, 15, escuro);
        desenhar_garras(x - lado_ataque * 62, y + 125);
    } else {
        DrawLineEx((Vector2){(float)(x - 50), (float)(y + 78)},
                   (Vector2){(float)(x - 67), (float)(y + 124)}, 25, pelo);
        DrawLineEx((Vector2){(float)(x + 50), (float)(y + 78)},
                   (Vector2){(float)(x + 67), (float)(y + 124)}, 25, pelo);
        DrawCircle(x - 69, y + 129, 15, escuro);
        DrawCircle(x + 69, y + 129, 15, escuro);
        desenhar_garras(x - 69, y + 129);
        desenhar_garras(x + 69, y + 129);
    }

    // Orelhas, cabeça e focinho.
    DrawCircle(x - 25, y + 15, 12, pelo);
    DrawCircle(x + 25, y + 15, 12, pelo);
    DrawCircle(x - 25, y + 15, 6, almofada);
    DrawCircle(x + 25, y + 15, 6, almofada);

    DrawCircle(x, y + 40, 32, pelo);
    DrawEllipse(x, y + 52, 16, 12, focinho);
    DrawEllipse(x, y + 47, 6, 4, BLACK);
    DrawLineEx((Vector2){(float)x, (float)(y + 51)},
               (Vector2){(float)x, (float)(y + 58)}, 2, BLACK);
    DrawLineEx((Vector2){(float)(x - 7), (float)(y + 60)},
               (Vector2){(float)x, (float)(y + 58)}, 2, BLACK);
    DrawLineEx((Vector2){(float)(x + 7), (float)(y + 60)},
               (Vector2){(float)x, (float)(y + 58)}, 2, BLACK);

    // Olhos com sobrancelhas franzidas.
    DrawCircle(x - 12, y + 35, 3, BLACK);
    DrawCircle(x + 12, y + 35, 3, BLACK);
    DrawLineEx((Vector2){(float)(x - 19), (float)(y + 27)},
               (Vector2){(float)(x - 7), (float)(y + 31)}, 3, escuro);
    DrawLineEx((Vector2){(float)(x + 19), (float)(y + 27)},
               (Vector2){(float)(x + 7), (float)(y + 31)}, 3, escuro);
}

/*
 * Desenha o chefe atual com visual exclusivo:
 * 1 = Bobrovsky (preso corpulento, uniforme listrado "214")
 * 2 = Vladimir (revolucionário magro, casaco remendado e faixa vermelha)
 * 3 = Ivan (guarda da prisão, ombreiras douradas, cinto e chapéu ushanka)
 * 4 = Anastasia (ursa gigante)
 */
static void desenhar_boss_de_frente(
    int centro_x,
    int topo_y,
    int numero_boss,
    EstadoBoss estado_boss,
    int lado_ataque
) {
    Color roupa = cor_uniforme(numero_boss, estado_boss);

    if (numero_boss == 4) {
        desenhar_urso_de_frente(centro_x, topo_y - 12, roupa, estado_boss, lado_ataque);
        return;
    }

    int x = centro_x;
    int y = topo_y;
    int i;

    Color pele = (Color){210, 172, 135, 255};
    Color luva = (Color){150, 40, 40, 255};
    Color calca = (Color){45, 42, 42, 255};
    Color preto = (Color){25, 24, 24, 255};
    Color ouro = (Color){200, 165, 50, 255};
    Color vermelho = (Color){175, 30, 30, 255};
    Color cabelo = (Color){60, 40, 30, 255};
    Color manga = roupa;

    // Porte físico de cada chefe.
    int meia_largura = 44;
    int alto_tronco = 70;
    int largura_perna = 20;
    int largura_braco = 18;
    int meia_cabeca = 26;
    int raio_luva = 13;

    if (numero_boss == 1) {
        meia_largura = 54;
        alto_tronco = 72;
        largura_perna = 25;
        largura_braco = 22;
        meia_cabeca = 28;
        raio_luva = 15;
        manga = pele;   // Bobrovsky luta de mangas arregaçadas
    } else if (numero_boss == 2) {
        meia_largura = 28;
        alto_tronco = 66;
        largura_perna = 12;
        largura_braco = 10;
        meia_cabeca = 22;
        raio_luva = 11;
    }

    int ombro = meia_largura - 8;

    // Pernas e botas.
    DrawRectangle(x - 3 - largura_perna, y + 104, largura_perna, 48, calca);
    DrawRectangle(x + 3, y + 104, largura_perna, 48, calca);
    DrawRectangle(x - 5 - largura_perna, y + 142, largura_perna + 4, 10, preto);
    DrawRectangle(x + 1, y + 142, largura_perna + 4, 10, preto);

    // Tronco.
    DrawRectangle(x - meia_largura, y + 40, meia_largura * 2, alto_tronco, roupa);

    if (numero_boss == 1) {
        // Listras de uniforme de preso e o número 214 no peito.
        Color listra = (Color){roupa.r / 2, roupa.g / 2, roupa.b / 2, 255};
        for (i = 0; i < 4; i++) {
            DrawRectangle(x - meia_largura, y + 52 + i * 15, meia_largura * 2, 6, listra);
        }
        DrawRectangle(x + 10, y + 56, 30, 16, (Color){225, 220, 200, 255});
        DrawText("214", x + 14, y + 59, 12, preto);
    } else if (numero_boss == 2) {
        // Casaco remendado com estrela vermelha.
        DrawRectangle(x - 1, y + 40, 2, alto_tronco, preto);
        DrawRectangle(x - 19, y + 76, 15, 14, (Color){90, 72, 58, 255});
        DrawRectangleLines(x - 19, y + 76, 15, 14, preto);
        desenhar_estrela(x + 11, y + 60, 8, vermelho);
    } else {
        // Uniforme de guarda: ombreiras douradas, botões e cinto.
        DrawRectangle(x - meia_largura, y + 40, 20, 7, ouro);
        DrawRectangle(x + meia_largura - 20, y + 40, 20, 7, ouro);
        DrawRectangle(x - 2, y + 56, 4, 4, ouro);
        DrawRectangle(x - 2, y + 72, 4, 4, ouro);
        DrawRectangle(x - meia_largura, y + 92, meia_largura * 2, 9, preto);
        DrawRectangle(x - 6, y + 92, 12, 9, ouro);
    }

    // Cabeça e traços faciais.
    DrawRectangle(x - meia_cabeca, y + 2, meia_cabeca * 2, 40, pele);
    DrawRectangle(x - meia_cabeca - 4, y + 16, 4, 12, pele);
    DrawRectangle(x + meia_cabeca, y + 16, 4, 12, pele);
    DrawRectangle(x - 2, y + 22, 4, 8, (Color){180, 140, 108, 255});
    DrawRectangle(x - meia_cabeca / 2 - 2, y + 18, 5, 5, preto);
    DrawRectangle(x + meia_cabeca / 2 - 3, y + 18, 5, 5, preto);
    DrawRectangle(x - 6, y + 34, 12, 3, (Color){110, 50, 45, 255});

    if (numero_boss != 2) {
        DrawRectangle(x - 15, y + 13, 12, 3, preto);
        DrawRectangle(x + 3, y + 13, 12, 3, preto);
    }

    if (numero_boss == 1) {
        // Cabeça raspada e barba por fazer.
        DrawRectangle(x - meia_cabeca, y + 38, meia_cabeca * 2, 4, (Color){150, 118, 92, 255});
        DrawRectangle(x - 12, y + 30, 24, 4, (Color){150, 118, 92, 255});
    } else if (numero_boss == 2) {
        // Cabelo bagunçado, faixa vermelha na testa e cavanhaque.
        DrawRectangle(x - meia_cabeca, y + 2, meia_cabeca * 2, 7, cabelo);
        DrawRectangle(x - meia_cabeca - 4, y + 6, 6, 14, cabelo);
        DrawRectangle(x + meia_cabeca - 2, y + 6, 6, 14, cabelo);
        DrawRectangle(x - meia_cabeca - 2, y + 11, meia_cabeca * 2 + 4, 6, vermelho);
        DrawRectangle(x + meia_cabeca, y + 15, 5, 14, vermelho);
        DrawRectangle(x - 5, y + 38, 10, 7, cabelo);
    } else {
        // Chapéu ushanka com estrela vermelha e bigode.
        Color pelo_chapeu = (Color){70, 58, 50, 255};
        DrawRectangle(x - meia_cabeca - 5, y - 8, meia_cabeca * 2 + 10, 20, pelo_chapeu);
        DrawRectangle(x - meia_cabeca - 5, y + 10, 10, 22, pelo_chapeu);
        DrawRectangle(x + meia_cabeca - 5, y + 10, 10, 22, pelo_chapeu);
        desenhar_estrela(x, y + 2, 7, vermelho);
        DrawRectangle(x - 11, y + 29, 22, 5, preto);
    }

    // Braços e luvas conforme o estado do combate.
    if (estado_boss == BOSS_GUARDANDO) {
        desenhar_braco(x - ombro, y + 61, x - 18, y + 39, largura_braco + 1, manga);
        desenhar_braco(x + ombro, y + 61, x + 18, y + 39, largura_braco + 1, manga);
        DrawCircle(x - 18, y + 38, raio_luva, luva);
        DrawCircle(x + 18, y + 38, raio_luva, luva);
    } else if (estado_boss == BOSS_PREPARANDO) {
        desenhar_braco(x + lado_ataque * (ombro - 3), y + 59,
                       x + lado_ataque * (ombro + 32), y + 68, largura_braco, manga);
        DrawCircle(x + lado_ataque * (ombro + 38), y + 68, raio_luva + 1, luva);
        desenhar_braco(x - lado_ataque * (ombro - 2), y + 62,
                       x - lado_ataque * (ombro + 8), y + 82, largura_braco - 1, manga);
        DrawCircle(x - lado_ataque * (ombro + 11), y + 86, raio_luva, luva);
    } else if (estado_boss == BOSS_ATACANDO) {
        desenhar_braco(x + lado_ataque * (ombro - 2), y + 62,
                       x + lado_ataque * 48, y + 158, largura_braco + 2, manga);
        DrawCircle(x + lado_ataque * 48, y + 168, raio_luva + 8, luva);
        desenhar_braco(x - lado_ataque * (ombro - 2), y + 62,
                       x - lado_ataque * (ombro + 8), y + 82, largura_braco - 1, manga);
        DrawCircle(x - lado_ataque * (ombro + 11), y + 86, raio_luva, luva);
    } else {
        desenhar_braco(x - ombro, y + 61, x - ombro - 14, y + 84, largura_braco, manga);
        desenhar_braco(x + ombro, y + 61, x + ombro + 14, y + 84, largura_braco, manga);
        DrawCircle(x - ombro - 17, y + 88, raio_luva, luva);
        DrawCircle(x + ombro + 17, y + 88, raio_luva, luva);
    }

    if (numero_boss == 2) {
        DrawRectangle(x - ombro - 6, y + 56, 12, 8, vermelho);
    }
}

static void desenhar_arena(void) {
    Color parede = (Color){35, 38, 44, 255};
    Color piso_fundo = (Color){77, 72, 67, 255};
    Color piso_frente = (Color){52, 49, 47, 255};
    Color corda = (Color){195, 190, 180, 255};

    ClearBackground((Color){22, 24, 29, 255});

    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, parede);
    DrawRectangleGradientV(245, 0, 470, 275, (Color){80, 76, 68, 255}, parede);

    DrawTriangle((Vector2){170, 190}, (Vector2){790, 190}, (Vector2){LARGURA_TELA, 430}, piso_fundo);
    DrawTriangle((Vector2){170, 190}, (Vector2){LARGURA_TELA, 430}, (Vector2){0, 430}, piso_frente);

    DrawLineEx((Vector2){170, 190}, (Vector2){0, 430}, 5, corda);
    DrawLineEx((Vector2){790, 190}, (Vector2){LARGURA_TELA, 430}, 5, corda);
    DrawLineEx((Vector2){170, 190}, (Vector2){790, 190}, 5, corda);
    DrawLineEx((Vector2){65, 365}, (Vector2){895, 365}, 3, (Color){125, 120, 112, 255});

    DrawLine(185, 245, 775, 245, (Color){91, 85, 78, 255});
    DrawLine(125, 310, 835, 310, (Color){86, 80, 74, 255});

    DrawLineEx((Vector2){110, 162}, (Vector2){850, 162}, 5, corda);
    DrawLineEx((Vector2){68, 197}, (Vector2){892, 197}, 4, (Color){155, 150, 143, 255});
    DrawLineEx((Vector2){27, 235}, (Vector2){933, 235}, 4, (Color){130, 126, 120, 255});

    DrawRectangle(45, 145, 13, 137, (Color){105, 35, 35, 255});
    DrawRectangle(902, 145, 13, 137, (Color){105, 35, 35, 255});

    DrawRectangleGradientV(0, 405, LARGURA_TELA, 135, (Color){20, 21, 24, 0}, (Color){20, 21, 24, 230});
}

static void preparar_lutadores_para_luta(Player *jogador, Boss *boss, int numero_boss) {
    iniciar_boss(boss, numero_boss);
    jogador->hp = PLAYER_VIDA;
    jogador->dano = PLAYER_DANO_BASE;
    jogador->defendendo = 0;
    jogador->esquivando = 0;
    jogador->cooldown_defesa = 0;
    jogador->cooldown_esquiva = 0;
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
    float *tempo_esquiva,
    int *quantidade_ataques,
    int *soco_esquerdo,
    int *soco_direito,
    int *soco_jogador_conectou,
    int *golpe_boss_conectou,
    int *esquiva_perfeita,
    int *resultado
) {
    preparar_lutadores_para_luta(jogador, boss, numero_boss);

    *estado_boss = BOSS_GUARDANDO;
    *tempo_estado = 0;
    *tempo_soco = 0;
    *tempo_recuperacao_soco = 0;
    *tempo_contra_ataque = 0;
    *tempo_esquiva = 0;
    *quantidade_ataques = 0;
    *soco_esquerdo = 0;
    *soco_direito = 0;
    *soco_jogador_conectou = 0;
    *golpe_boss_conectou = 0;
    *esquiva_perfeita = 0;
    *resultado = COMBATE_EM_ANDAMENTO;
}

static void desenhar_menu(int opcao) {
    Color normal = (Color){190, 195, 205, 255};
    Color destaque = (Color){255, 205, 55, 255};
    const char *rotulos[MENU_TOTAL] = {"COMEÇAR", "PLACAR", "SAIR"};

    desenhar_arena();
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){0, 0, 0, 170});

    desenhar_texto_centralizado("BEYOND THE FALLEN", 110, 64, RAYWHITE);
    desenhar_texto_centralizado("Sobreviva ao gulag. Derrube os chefes.", 195, 22, normal);

    for (int i = 0; i < MENU_TOTAL; i++) {
        desenhar_texto_centralizado(
            i == opcao ? TextFormat("> %s <", rotulos[i]) : rotulos[i],
            265 + i * 55,
            34,
            i == opcao ? destaque : normal
        );
    }

    desenhar_texto_centralizado("W/S ou SETAS: escolher    ENTER: confirmar", 460, 18, normal);
    desenhar_texto_centralizado(
        "Projeto de PIF - CESAR School", 500, 16, (Color){120, 125, 135, 255}
    );
}

/*
 * Tela de Placar / Ranking lida diretamente do arquivo placar.txt.
 */
static void desenhar_placar_ranking(const RegistroPlacar lista[], int total) {
    Color cinza = (Color){190, 195, 205, 255};
    Color dourado = (Color){255, 205, 55, 255};

    desenhar_arena();
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){0, 0, 0, 210});

    desenhar_texto_centralizado("PLACAR OFICIAL (TOP 10)", 28, 42, RAYWHITE);

    DrawText("POS", 70, 95, 18, cinza);
    DrawText("JOGADOR", 130, 95, 18, cinza);
    DrawText("PONTOS", 330, 95, 18, cinza);
    DrawText("CHEFES", 440, 95, 18, cinza);
    DrawText("ACERTOS", 545, 95, 18, cinza);
    DrawText("ESQUIVAS", 660, 95, 18, cinza);
    DrawText("STATUS", 785, 95, 18, cinza);
    DrawLine(60, 120, 900, 120, (Color){110, 115, 125, 255});

    if (total == 0) {
        desenhar_texto_centralizado("Nenhum recorde registrado em placar.txt ainda.", 255, 24, cinza);
    } else {
        for (int i = 0; i < total; i++) {
            int y = 132 + i * 31;
            Color cor = (i == 0) ? dourado : RAYWHITE;

            if (i == 0) {
                DrawRectangle(60, y - 4, 840, 28, (Color){255, 205, 55, 35});
            }

            DrawText(TextFormat("#%d", i + 1), 70, y, 20, cor);
            DrawText(lista[i].jogador, 130, y, 20, cor);
            DrawText(TextFormat("%d", lista[i].pontuacao), 330, y, 20, cor);
            DrawText(TextFormat("%d/4", lista[i].chefes_derrotados), 452, y, 20, cor);
            DrawText(TextFormat("%d", lista[i].golpes_acertados), 565, y, 20, cor);
            DrawText(TextFormat("%d", lista[i].esquivas_perfeitas), 685, y, 20, cor);
            DrawText(lista[i].resultado, 785, y, 18, cor);
        }
    }

    desenhar_texto_centralizado("ENTER ou ESC: VOLTAR AO MENU", 485, 22, dourado);
}

/*
 * Tela de transição exibida antes da 1ª luta (10s) e entre cada luta (5s).
 * Mostra o desenho exclusivo de cada lutador da vez e todos os seus status.
 */
static void desenhar_transicao(
    float restante,
    float duracao_total,
    const Player *jogador,
    const Boss *boss,
    int numero_boss,
    int continues,
    int pontuacao_atual
) {
    Color cinza = (Color){190, 195, 205, 255};
    Color dourado = (Color){255, 205, 55, 255};
    Color painel_fundo = (Color){24, 27, 34, 245};
    Color borda_jogador = (Color){65, 145, 235, 255};
    Color borda_boss = (Color){220, 65, 65, 255};

    float decorrido = duracao_total - restante;
    float alfa = 1.0f;

    if (decorrido < 0.6f) alfa = decorrido / 0.6f;
    if (restante < 0.6f && (restante / 0.6f) < alfa) alfa = restante / 0.6f;
    if (alfa < 0.0f) alfa = 0.0f;
    if (alfa > 1.0f) alfa = 1.0f;

    int segundos = (int)restante;
    if (restante > (float)segundos) segundos++;

    ClearBackground((Color){12, 13, 16, 255});

    desenhar_texto_centralizado(
        TextFormat("COMBATE %d DE 4 - PREPARE-SE", numero_boss),
        16, 28, Fade(RAYWHITE, alfa)
    );
    desenhar_texto_centralizado(
        TextFormat("A LUTA COMEÇA EM %d SEGUNDO%s", segundos, segundos == 1 ? "" : "S"),
        50, 20, Fade(dourado, alfa)
    );

    int largura_barra = 420;
    int barra_x = (LARGURA_TELA - largura_barra) / 2;
    DrawRectangle(barra_x, 76, largura_barra, 8, Fade(DARKGRAY, alfa));
    DrawRectangle(
        barra_x, 76,
        (int)(largura_barra * decorrido / duracao_total),
        8,
        Fade(dourado, alfa)
    );

    // Painel esquerdo: JOGADOR.
    DrawRectangle(40, 96, 380, 376, Fade(painel_fundo, alfa));
    DrawRectangleLinesEx((Rectangle){40, 96, 380, 376}, 2, Fade(borda_jogador, alfa));
    DrawText(jogador->nome, 60, 106, 22, Fade(YELLOW, alfa));
    DrawText("DESAFIANTE", 305, 110, 14, Fade(borda_jogador, alfa));

    desenhar_jogador_card(230, 136);
    DrawLine(58, 294, 402, 294, Fade((Color){60, 66, 80, 255}, alfa));

    DrawText(
        TextFormat("Vida: %d / %d (Reviver: %d)", jogador->hp, PLAYER_VIDA, PLAYER_VIDA_REVIVER),
        60, 304, 17, Fade(RAYWHITE, alfa)
    );
    DrawText(
        TextFormat("Dano Base: %d", jogador->dano),
        60, 328, 17, Fade(RAYWHITE, alfa)
    );
    DrawText(
        TextFormat("Dano Contra-Ataque: %d", PLAYER_DANO_BASE * 2),
        60, 352, 17, Fade(dourado, alfa)
    );
    DrawText(
        TextFormat("Cooldown Esquiva: %.1fs | Defesa: 50%%", (float)COOLDOWN_ESQUIVA),
        60, 376, 17, Fade(cinza, alfa)
    );
    DrawText(
        TextFormat("Continues Restantes: %d", continues),
        60, 400, 17, Fade(cinza, alfa)
    );
    DrawText(
        TextFormat("Placar Acumulado: %d pts", pontuacao_atual),
        60, 424, 17, Fade(GREEN, alfa)
    );

    // Emblema central VS.
    DrawText("VS", 452, 255, 42, Fade(dourado, alfa));

    // Painel direito: CHEFE DA VEZ (com skin específica de numero_boss).
    int vida_max_boss = obter_vida_maxima_boss(numero_boss);
    float intervalo_boss = calcular_intervalo_ataque(boss);

    DrawRectangle(540, 96, 380, 376, Fade(painel_fundo, alfa));
    DrawRectangleLinesEx((Rectangle){540, 96, 380, 376}, 2, Fade(borda_boss, alfa));
    DrawText(boss->nome, 560, 106, 22, Fade(RED, alfa));
    DrawText(TextFormat("CHEFE #%d", numero_boss), 825, 110, 14, Fade(borda_boss, alfa));

    desenhar_boss_de_frente(
        730,
        135,
        numero_boss,
        BOSS_GUARDANDO,
        -1
    );
    DrawLine(558, 294, 902, 294, Fade((Color){60, 66, 80, 255}, alfa));

    DrawText(
        TextFormat("Vida: %d / %d", boss->hp, vida_max_boss),
        560, 304, 17, Fade(RAYWHITE, alfa)
    );
    DrawText(
        TextFormat("Dano Base: %d", boss->dano),
        560, 328, 17, Fade(RAYWHITE, alfa)
    );
    if (numero_boss == 4) {
        DrawText(
            TextFormat("Dano Especial (3o golpe): %d", ANASTASIA_DANO_ESPECIAL),
            560, 352, 17, Fade(ORANGE, alfa)
        );
    } else {
        DrawText("Dano Especial: Nenhum", 560, 352, 17, Fade(cinza, alfa));
    }
    DrawText(
        TextFormat("Velocidade: %.1f", (float)boss->velocidade),
        560, 376, 17, Fade(RAYWHITE, alfa)
    );
    DrawText(
        TextFormat("Intervalo entre Ataques: %.2fs", intervalo_boss),
        560, 400, 17, Fade(cinza, alfa)
    );
    DrawText(
        TextFormat("Nivel de Ameaça: %d / 4", numero_boss),
        560, 424, 17, Fade(borda_boss, alfa)
    );

    desenhar_texto_centralizado(
        "A / D : Esquivar   |   S : Defender   |   J / K : Soco",
        492, 18, Fade(cinza, alfa)
    );
}

static void desenhar_game_over(const RegistroPlacar *placar, int posicao_ranking) {
    ClearBackground((Color){22, 24, 29, 255});

    desenhar_texto_centralizado("GAME OVER", 95, 64, RED);
    desenhar_texto_centralizado("Você perdeu todos os continues.", 175, 24, RAYWHITE);

    desenhar_texto_centralizado(
        TextFormat(
            "Pontuação Final: %d pts  |  Ranking (%s): #%d",
            placar->pontuacao,
            ARQUIVO_PLACAR,
            posicao_ranking
        ),
        235, 22, YELLOW
    );
    desenhar_texto_centralizado(
        TextFormat(
            "Chefes: %d/4  |  Acertos: %d  |  Esquivas: %d  |  Golpes Sofridos: %d",
            placar->chefes_derrotados,
            placar->golpes_acertados,
            placar->esquivas_perfeitas,
            placar->golpes_recebidos
        ),
        275, 19, (Color){190, 195, 205, 255}
    );
    desenhar_texto_centralizado(
        "Placar salvo e atualizado em placar.txt",
        315, 18, GREEN
    );

    desenhar_texto_centralizado(
        "ENTER: VOLTAR AO MENU (O JOGO RECOMEÇA)", 390, 22, RAYWHITE
    );
}

int main(void) {
    Player jogador;
    Boss boss = {0};

    player_iniciar(&jogador);

    InitWindow(LARGURA_TELA, ALTURA_TELA, "Beyond the Fallen");
    SetTargetFPS(60);

    Tela tela = TELA_MENU;
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
    float tempo_esquiva = 0;
    float intervalo_ataque = 2.0f;

    const float duracao_preparacao = 0.9f;
    const float duracao_ataque_boss = 0.25f;
    const float duracao_recuperacao = 1.5f;
    const float duracao_esquiva = 0.75f;

    int soco_esquerdo = 0;
    int soco_direito = 0;
    int soco_jogador_conectou = 0;
    int golpe_boss_conectou = 0;
    int mostrar_hitboxes = 0;

    // Feedback visual de colisão.
    char texto_feedback[40] = "";
    Color cor_feedback = RAYWHITE;
    float tempo_feedback = 0.0f;

    // Menu, transição e sistema de continues.
    int opcao_menu = MENU_COMECAR;
    int sair_do_jogo = 0;
    int iniciar_combate = 0;
    int revivido = 0;
    int continues = CONTINUES_INICIAIS;
    float tempo_transicao = 10.0f;
    float duracao_transicao_atual = 10.0f;

    // Placar da partida atual e lista lida para exibição no menu.
    RegistroPlacar placar_atual = {0};
    RegistroPlacar lista_ranking[MAXIMO_REGISTROS_PLACAR] = {0};
    int total_ranking = 0;
    int indice_placar_salvo = -1;
    int posicao_ranking = 1;

    while (!WindowShouldClose() && !sair_do_jogo) {
        float tempo = GetFrameTime();

        if (IsKeyPressed(KEY_F1)) {
            mostrar_hitboxes = !mostrar_hitboxes;
        }

        if (tempo_feedback > 0) {
            tempo_feedback -= tempo;
            if (tempo_feedback < 0) {
                tempo_feedback = 0;
            }
        }

        if (tela == TELA_MENU) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                opcao_menu = (opcao_menu + MENU_TOTAL - 1) % MENU_TOTAL;
            }

            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                opcao_menu = (opcao_menu + 1) % MENU_TOTAL;
            }

            if (IsKeyPressed(KEY_ENTER)) {
                if (opcao_menu == MENU_COMECAR) {
                    // Novo jogo: recomeça do primeiro chefe com transição de 10 segundos.
                    numero_boss = 1;
                    continues = CONTINUES_INICIAIS;
                    revivido = 0;

                    preparar_lutadores_para_luta(&jogador, &boss, numero_boss);
                    intervalo_ataque = calcular_intervalo_ataque(&boss);

                    memset(&placar_atual, 0, sizeof(placar_atual));
                    snprintf(placar_atual.jogador, sizeof(placar_atual.jogador), "%.31s", jogador.nome);
                    snprintf(placar_atual.resultado, sizeof(placar_atual.resultado), "EM CURSO");
                    indice_placar_salvo = -1;
                    posicao_ranking = sincronizar_placar_txt(ARQUIVO_PLACAR, &placar_atual, &indice_placar_salvo);

                    duracao_transicao_atual = 10.0f;
                    tempo_transicao = duracao_transicao_atual;
                    tela = TELA_TRANSICAO;
                } else if (opcao_menu == MENU_PLACAR) {
                    total_ranking = carregar_placar_txt(ARQUIVO_PLACAR, lista_ranking, MAXIMO_REGISTROS_PLACAR);
                    ordenar_placar(lista_ranking, total_ranking);
                    tela = TELA_PLACAR;
                } else {
                    sair_do_jogo = 1;
                }
            }
        } else if (tela == TELA_PLACAR) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
                tela = TELA_MENU;
            }
        } else if (tela == TELA_TRANSICAO) {
            tempo_transicao -= tempo;

            if (tempo_transicao <= 0) {
                tempo_transicao = 0;
                iniciar_combate = 1;
            }
        } else if (tela == TELA_COMBATE) {
            combat_atualizar(&jogador, tempo);

            if (tempo_esquiva > 0) {
                tempo_esquiva -= tempo;
                jogador.esquivando = 1;
                if (tempo_esquiva <= 0) {
                    tempo_esquiva = 0;
                    jogador.esquivando = 0;
                    direcao_esquiva = ESQUIVA_NENHUMA;
                }
            }

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
                    soco_jogador_conectou = 0;
                }
            }

            // Dispara o soco (cria a hitbox ofensiva do jogador).
            if ((IsKeyPressed(KEY_J) || IsKeyPressed(KEY_K)) &&
                tempo_recuperacao_soco <= 0 &&
                jogador.hp > 0 &&
                boss.hp > 0) {

                soco_esquerdo = IsKeyPressed(KEY_J);
                soco_direito = IsKeyPressed(KEY_K);
                soco_jogador_conectou = 0;
                tempo_soco = 0.22f;
                tempo_recuperacao_soco = 0.35f;
            }

            if (IsKeyPressed(KEY_S)) {
                combat_defender(&jogador);
            }

            if (IsKeyPressed(KEY_A) &&
                jogador.cooldown_esquiva <= 0) {
                combat_esquivar(&jogador);
                direcao_esquiva = ESQUIVA_ESQUERDA;
                tempo_esquiva = duracao_esquiva;
            }

            if (IsKeyPressed(KEY_D) &&
                jogador.cooldown_esquiva <= 0) {
                combat_esquivar(&jogador);
                direcao_esquiva = ESQUIVA_DIREITA;
                tempo_esquiva = duracao_esquiva;
            }

            tempo_estado += tempo;

            switch (estado_boss) {
                case BOSS_GUARDANDO:
                    if (tempo_estado >= intervalo_ataque) {
                        estado_boss = BOSS_PREPARANDO;
                        tempo_estado = 0;
                        direcao_ataque =
                            GetRandomValue(0, 1) == 0
                                ? ATAQUE_ESQUERDA
                                : ATAQUE_DIREITA;
                    }
                    break;

                case BOSS_PREPARANDO:
                    if (tempo_estado >= duracao_preparacao) {
                        quantidade_ataques++;
                        golpe_boss_conectou = 0;
                        estado_boss = BOSS_ATACANDO;
                        tempo_estado = 0;
                    }
                    break;

                case BOSS_ATACANDO:
                    if (tempo_estado >= duracao_ataque_boss) {
                        if (!golpe_boss_conectou && jogador.esquivando) {
                            esquiva_perfeita = 1;
                            tempo_contra_ataque = 0.8f;
                            placar_atual.esquivas_perfeitas++;
                            placar_atual.pontuacao += 75;

                            snprintf(texto_feedback, sizeof(texto_feedback), "ESQUIVA PERFEITA! +75");
                            cor_feedback = SKYBLUE;
                            tempo_feedback = 0.75f;
                        }

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

            // Calcula hitboxes/hurtboxes atuais e resolve contato por colisão.
            int direcao_visual_esquiva = 0;
            if (direcao_esquiva == ESQUIVA_ESQUERDA) {
                direcao_visual_esquiva = -1;
            } else if (direcao_esquiva == ESQUIVA_DIREITA) {
                direcao_visual_esquiva = 1;
            }

            HitboxesCombate hitboxes = calcular_hitboxes_combate(
                480,
                270,
                157,
                numero_boss,
                jogador.esquivando,
                direcao_visual_esquiva,
                tempo_soco > 0 && (soco_esquerdo || soco_direito),
                soco_esquerdo,
                soco_direito,
                estado_boss,
                direcao_ataque
            );

            // 1. Colisão do soco do jogador com a guarda ou com o corpo do chefe.
            if (tempo_soco > 0 && !soco_jogador_conectou && jogador.hp > 0 && boss.hp > 0) {
                if (estado_boss == BOSS_GUARDANDO &&
                    CheckCollisionRecs(hitboxes.hitbox_soco_jogador, hitboxes.hitbox_guarda_boss)) {
                    soco_jogador_conectou = 1;
                    snprintf(texto_feedback, sizeof(texto_feedback), "BLOQUEADO PELA GUARDA!");
                    cor_feedback = LIGHTGRAY;
                    tempo_feedback = 0.5f;
                } else if (CheckCollisionRecs(hitboxes.hitbox_soco_jogador, hitboxes.hurtbox_boss)) {
                    soco_jogador_conectou = 1;

                    int dano_original = jogador.dano;
                    int foi_contra_ataque = 0;

                    if (tempo_contra_ataque > 0 && esquiva_perfeita) {
                        jogador.dano = PLAYER_DANO_BASE * 2;
                        tempo_contra_ataque = 0;
                        esquiva_perfeita = 0;
                        foi_contra_ataque = 1;
                    }

                    combat_atacar(&jogador, &boss);
                    jogador.dano = dano_original;

                    placar_atual.golpes_acertados++;
                    placar_atual.pontuacao += foi_contra_ataque ? 150 : 50;

                    snprintf(
                        texto_feedback,
                        sizeof(texto_feedback),
                        foi_contra_ataque ? "CONTRA-ATAQUE! +150" : "IMPACTO! +50"
                    );
                    cor_feedback = foi_contra_ataque ? GOLD : GREEN;
                    tempo_feedback = 0.6f;
                }
            }

            // 2. Colisão do golpe do chefe com a hurtbox do jogador.
            if (estado_boss == BOSS_ATACANDO && !golpe_boss_conectou && boss.hp > 0 && jogador.hp > 0) {
                if (CheckCollisionRecs(hitboxes.hitbox_ataque_boss, hitboxes.hurtbox_jogador)) {
                    golpe_boss_conectou = 1;

                    int dano_original = boss.dano;
                    if (numero_boss == 4 && quantidade_ataques % 3 == 0) {
                        boss.dano = ANASTASIA_DANO_ESPECIAL;
                    }

                    int hp_antes = jogador.hp;
                    combat_boss_atacar(&boss, &jogador);
                    boss.dano = dano_original;

                    esquiva_perfeita = 0;
                    tempo_contra_ataque = 0;

                    placar_atual.golpes_recebidos++;
                    placar_atual.pontuacao -= 40;
                    if (placar_atual.pontuacao < 0) {
                        placar_atual.pontuacao = 0;
                    }

                    snprintf(
                        texto_feedback,
                        sizeof(texto_feedback),
                        jogador.defendendo ? "DEFENDIDO! (-%d HP)" : "GOLPE SOFRIDO! (-%d HP)",
                        hp_antes - jogador.hp
                    );
                    cor_feedback = jogador.defendendo ? SKYBLUE : RED;
                    tempo_feedback = 0.75f;
                }
            }

            resultado = combat_verificar_fim(&jogador, &boss);

            if (resultado == COMBATE_EMPATE) {
                resultado = COMBATE_DERROTA;
            }

            if (resultado != COMBATE_EM_ANDAMENTO) {
                tela = TELA_FINAL;

                if (resultado == COMBATE_VITORIA) {
                    if (placar_atual.chefes_derrotados < numero_boss) {
                        placar_atual.chefes_derrotados = numero_boss;
                        placar_atual.pontuacao += 300 * numero_boss + jogador.hp * 2;
                    }

                    if (numero_boss == 4) {
                        placar_atual.pontuacao += continues * 200;
                        snprintf(placar_atual.resultado, sizeof(placar_atual.resultado), "CAMPEAO");
                    } else {
                        snprintf(placar_atual.resultado, sizeof(placar_atual.resultado), "CHEFE %d OK", numero_boss);
                    }

                    posicao_ranking = sincronizar_placar_txt(
                        ARQUIVO_PLACAR,
                        &placar_atual,
                        &indice_placar_salvo
                    );
                } else if (resultado == COMBATE_DERROTA && revivido) {
                    continues--;
                    placar_atual.pontuacao -= 100;
                    if (placar_atual.pontuacao < 0) {
                        placar_atual.pontuacao = 0;
                    }

                    if (continues <= 0) {
                        continues = 0;
                        snprintf(placar_atual.resultado, sizeof(placar_atual.resultado), "GAME OVER");
                        posicao_ranking = sincronizar_placar_txt(
                            ARQUIVO_PLACAR,
                            &placar_atual,
                            &indice_placar_salvo
                        );
                        tela = TELA_GAME_OVER;
                    } else {
                        snprintf(placar_atual.resultado, sizeof(placar_atual.resultado), "CONTINUE");
                        posicao_ranking = sincronizar_placar_txt(
                            ARQUIVO_PLACAR,
                            &placar_atual,
                            &indice_placar_salvo
                        );
                    }
                }
            }
        } else if (tela == TELA_FINAL) {
            if (resultado == COMBATE_VITORIA &&
                numero_boss < 4 &&
                IsKeyPressed(KEY_ENTER)) {

                numero_boss++;
                revivido = 0;
                preparar_lutadores_para_luta(&jogador, &boss, numero_boss);
                intervalo_ataque = calcular_intervalo_ataque(&boss);

                duracao_transicao_atual = 5.0f;
                tempo_transicao = duracao_transicao_atual;
                tela = TELA_TRANSICAO;
            } else if (resultado == COMBATE_VITORIA &&
                       numero_boss == 4 &&
                       IsKeyPressed(KEY_ENTER)) {

                numero_boss = 1;
                continues = CONTINUES_INICIAIS;
                revivido = 0;
                opcao_menu = MENU_COMECAR;
                tela = TELA_MENU;
            } else if (resultado == COMBATE_DERROTA &&
                       !revivido &&
                       IsKeyPressed(KEY_ENTER)) {

                jogador.hp = PLAYER_VIDA_REVIVER;
                jogador.defendendo = 0;
                jogador.esquivando = 0;
                jogador.cooldown_defesa = 0;
                jogador.cooldown_esquiva = 0;

                direcao_esquiva = ESQUIVA_NENHUMA;
                tempo_esquiva = 0;
                revivido = 1;
                resultado = COMBATE_EM_ANDAMENTO;
                tela = TELA_COMBATE;
            } else if (resultado == COMBATE_DERROTA &&
                       revivido &&
                       IsKeyPressed(KEY_ENTER)) {

                iniciar_combate = 1;
            }
        } else if (tela == TELA_GAME_OVER) {
            if (IsKeyPressed(KEY_ENTER)) {
                numero_boss = 1;
                continues = CONTINUES_INICIAIS;
                revivido = 0;
                opcao_menu = MENU_COMECAR;
                tela = TELA_MENU;
            }
        }

        // Começa (ou recomeça do zero) a luta do chefe atual.
        if (iniciar_combate) {
            iniciar_combate = 0;

            reiniciar_combate(
                &jogador,
                &boss,
                numero_boss,
                &estado_boss,
                &tempo_estado,
                &tempo_soco,
                &tempo_recuperacao_soco,
                &tempo_contra_ataque,
                &tempo_esquiva,
                &quantidade_ataques,
                &soco_esquerdo,
                &soco_direito,
                &soco_jogador_conectou,
                &golpe_boss_conectou,
                &esquiva_perfeita,
                &resultado
            );

            revivido = 0;
            direcao_esquiva = ESQUIVA_NENHUMA;
            intervalo_ataque = calcular_intervalo_ataque(&boss);
            tela = TELA_COMBATE;
        }

        BeginDrawing();

        if (tela == TELA_MENU) {
            desenhar_menu(opcao_menu);
        } else if (tela == TELA_PLACAR) {
            desenhar_placar_ranking(lista_ranking, total_ranking);
        } else if (tela == TELA_TRANSICAO) {
            desenhar_transicao(
                tempo_transicao,
                duracao_transicao_atual,
                &jogador,
                &boss,
                numero_boss,
                continues,
                placar_atual.pontuacao
            );
        } else if (tela == TELA_COMBATE) {
            desenhar_arena();

            DrawText("BEYOND THE FALLEN", 326, 22, 28, RAYWHITE);
            desenhar_texto_centralizado(
                TextFormat("PLACAR: %d PTS", placar_atual.pontuacao),
                54, 18, GOLD
            );

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

            DrawText(
                TextFormat(
                    "CONTINUES: %d   REVIVER: %s",
                    continues,
                    revivido ? "USADO" : "DISPONÍVEL"
                ),
                95, 135, 16,
                (Color){190, 195, 205, 255}
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

            // Sombras no piso proporcionais ao porte do chefe.
            int largura_sombra = (numero_boss == 4) ? 62 : (numero_boss == 1 ? 52 : (numero_boss == 2 ? 32 : 42));
            DrawEllipse(480, 310, largura_sombra, 9, (Color){15, 16, 18, 120});
            DrawEllipse(480, 505, 92, 15, (Color){15, 16, 18, 150});

            // Chefe ao fundo com sua skin própria.
            desenhar_boss_de_frente(
                480,
                157,
                numero_boss,
                estado_boss,
                direcao_ataque == ATAQUE_ESQUERDA ? -1 : 1
            );

            // Jogador em primeiro plano, visto de costas.
            int direcao_visual_esquiva = 0;
            if (direcao_esquiva == ESQUIVA_ESQUERDA) {
                direcao_visual_esquiva = -1;
            } else if (direcao_esquiva == ESQUIVA_DIREITA) {
                direcao_visual_esquiva = 1;
            }

            int jogador_atacando = tempo_soco > 0 && (soco_esquerdo || soco_direito);

            desenhar_jogador_de_costas(
                480,
                270,
                jogador_atacando,
                soco_esquerdo,
                jogador.defendendo,
                jogador.esquivando,
                direcao_visual_esquiva
            );

            if (jogador.defendendo) {
                int dx_escudo = jogador.esquivando ? (direcao_visual_esquiva * 55) : 0;
                DrawEllipseLines(
                    480 + dx_escudo, 386, 91, 113,
                    (Color){80, 170, 255, 180}
                );
            }

            // Visualização opcional das hitboxes/hurtboxes (tecla F1).
            if (mostrar_hitboxes) {
                HitboxesCombate hb = calcular_hitboxes_combate(
                    480,
                    270,
                    157,
                    numero_boss,
                    jogador.esquivando,
                    direcao_visual_esquiva,
                    jogador_atacando,
                    soco_esquerdo,
                    soco_direito,
                    estado_boss,
                    direcao_ataque
                );

                DrawRectangleLinesEx(hb.hurtbox_jogador, 2, LIME);
                DrawRectangleLinesEx(hb.hurtbox_boss, 2, YELLOW);

                if (jogador_atacando) {
                    DrawRectangleLinesEx(hb.hitbox_soco_jogador, 2, SKYBLUE);
                }
                if (estado_boss == BOSS_GUARDANDO) {
                    DrawRectangleLinesEx(hb.hitbox_guarda_boss, 2, LIGHTGRAY);
                }
                if (estado_boss == BOSS_ATACANDO) {
                    DrawRectangleLinesEx(hb.hitbox_ataque_boss, 2, RED);
                }
            }

            if (tempo_feedback > 0) {
                desenhar_texto_centralizado(texto_feedback, 165, 22, cor_feedback);
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
            DrawText(
                "F1: HITBOXES",
                700, 505, 16,
                (Color){140, 145, 155, 255}
            );
        } else if (tela == TELA_FINAL) {
            ClearBackground((Color){22, 24, 29, 255});

            if (resultado == COMBATE_VITORIA && numero_boss < 4) {
                desenhar_texto_centralizado("VITÓRIA!", 110, 52, GREEN);
                desenhar_texto_centralizado(
                    TextFormat(
                        "Placar Atual: %d pts  |  Ranking (%s): #%d",
                        placar_atual.pontuacao,
                        ARQUIVO_PLACAR,
                        posicao_ranking
                    ),
                    195, 22, YELLOW
                );
                desenhar_texto_centralizado(
                    "Progresso salvo em placar.txt",
                    232, 18, (Color){190, 195, 205, 255}
                );
                desenhar_texto_centralizado(
                    "ENTER: PRÓXIMO COMBATE",
                    310, 24, RAYWHITE
                );
            } else if (resultado == COMBATE_VITORIA) {
                desenhar_texto_centralizado(
                    "CAMPEÃO",
                    95, 52,
                    (Color){255, 205, 55, 255}
                );
                desenhar_texto_centralizado(
                    "VOCÊ VENCEU A CAMPANHA!",
                    165, 24, RAYWHITE
                );
                desenhar_texto_centralizado(
                    TextFormat(
                        "Pontuação Final: %d pts  |  Ranking (%s): #%d",
                        placar_atual.pontuacao,
                        ARQUIVO_PLACAR,
                        posicao_ranking
                    ),
                    220, 22, YELLOW
                );
                desenhar_texto_centralizado(
                    TextFormat(
                        "Acertos: %d  |  Esquivas: %d  |  Golpes Sofridos: %d",
                        placar_atual.golpes_acertados,
                        placar_atual.esquivas_perfeitas,
                        placar_atual.golpes_recebidos
                    ),
                    258, 19, (Color){190, 195, 205, 255}
                );
                desenhar_texto_centralizado(
                    "Placar final registrado em placar.txt",
                    295, 18, GREEN
                );
                desenhar_texto_centralizado(
                    "ENTER: VOLTAR AO MENU",
                    365, 22, RAYWHITE
                );
            } else if (!revivido) {
                desenhar_texto_centralizado("VOCÊ CAIU!", 150, 52, RED);
                desenhar_texto_centralizado(
                    "Você pode se levantar uma vez nesta luta.", 235, 22, RAYWHITE
                );
                desenhar_texto_centralizado(
                    TextFormat("ENTER: REVIVER COM %d DE VIDA", PLAYER_VIDA_REVIVER),
                    285, 24, YELLOW
                );
                desenhar_texto_centralizado(
                    "A vida do chefe e o andamento da luta continuam.", 330, 20,
                    (Color){190, 195, 205, 255}
                );
            } else {
                desenhar_texto_centralizado("DERROTA", 135, 52, RED);
                desenhar_texto_centralizado(
                    TextFormat("CONTINUES RESTANTES: %d", continues),
                    215, 24, RAYWHITE
                );
                desenhar_texto_centralizado(
                    TextFormat("Placar atualizado em placar.txt: %d pts", placar_atual.pontuacao),
                    255, 18, (Color){190, 195, 205, 255}
                );
                desenhar_texto_centralizado(
                    "ENTER: RECOMEÇAR A LUTA DO ZERO", 315, 24, YELLOW
                );
            }
        } else if (tela == TELA_GAME_OVER) {
            desenhar_game_over(&placar_atual, posicao_ranking);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
