#include "raylib.h"
#include "rlgl.h"
#include <ctype.h>
#include <math.h>
#include <string.h>
#include "player.h"
#include "boss.h"
#include "combat.h"
#include "config.h"
#include "placar.h"
#include "estatisticas.h"

typedef enum {
    TELA_MENU,
    TELA_TRANSICAO,
    TELA_COMBATE,
    TELA_FINAL,
    TELA_GAME_OVER,
    TELA_NOME,
    TELA_PLACAR
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

// Sequência de nocaute: queda -> juiz entra -> contagem -> levanta (ou fim).
typedef enum {
    KO_NENHUM,
    KO_QUEDA,
    KO_JUIZ_ENTRA,
    KO_CONTAGEM,
    KO_LEVANTA,
    KO_FIM
} FaseKO;

typedef enum {
    KO_ALVO_JOGADOR,
    KO_ALVO_BOSS
} AlvoKO;

// Opções do menu inicial.
#define MENU_COMECAR 0
#define MENU_PLACAR 1
#define MENU_SAIR 2
#define MENU_TOTAL 3

// Tudo que a tela de nome e a tela de placar precisam.
typedef struct {
    Placar placar;
    int destaque;            // linha do recorde recém-inserido (-1 = nenhuma)
    int pontos_fora;         // pontuação que não entrou no placar (-1 = nenhuma)
    int opcao;               // 0 = VOLTAR, 1 = APAGAR RECORDES
    int confirmar_apagar;    // 1 = próximo clique/ENTER apaga de verdade
    char nome[PLACAR_TAM_NOME + 1];
    int tam_nome;
} EstadoPlacar;

// Texto de "documento" mostrado embaixo do nome de cada chefe.
static const char *PAPEIS_CHEFES[NUM_CHEFES] = {
    "PRESO 214 - TRAIÇÃO À PÁTRIA",
    "PRESO - REVOLUCIONÁRIO",
    "GUARDA DA PRISÃO",
    "DESAFIO FINAL"
};

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

static void desenhar_texto_centralizado(const char *texto, int y, int tamanho, Color cor) {
    int largura = MeasureText(texto, tamanho);
    DrawText(texto, (LARGURA_TELA - largura) / 2, y, tamanho, cor);
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

// Braço como uma linha grossa (evita repetir o cast em todo lugar).
static void desenhar_braco(int x1, int y1, int x2, int y2, int largura, Color cor) {
    DrawLineEx((Vector2){(float)x1, (float)y1}, (Vector2){(float)x2, (float)y2}, (float)largura, cor);
}

// Estrela de cinco pontas: liga cada ponta na ponta dois lugares à frente.
static void desenhar_estrela(int cx, int cy, int raio, Color cor) {
    Vector2 ponta[5];
    int i;

    for (i = 0; i < 5; i++) {
        float angulo = -PI / 2 + i * 2 * PI / 5;
        ponta[i].x = cx + cosf(angulo) * raio;
        ponta[i].y = cy + sinf(angulo) * raio;
    }

    for (i = 0; i < 5; i++) {
        DrawLineEx(ponta[i], ponta[(i + 2) % 5], 2, cor);
    }
}

// Cor da roupa de cada chefe. Quando ele prepara o golpe fica avermelhado,
// quando está se recuperando fica mais escuro.
static Color cor_uniforme(int numero_boss, EstadoBoss estado) {
    Color cor;
    int r;

    switch (numero_boss) {
        case 1: cor = (Color){96, 104, 112, 255}; break;   // uniforme de preso
        case 2: cor = (Color){120, 84, 56, 255}; break;    // casaco velho
        case 3: cor = (Color){70, 86, 62, 255}; break;     // uniforme de guarda
        default: cor = (Color){112, 74, 46, 255}; break;   // pelo da ursa
    }

    if (estado == BOSS_PREPARANDO) {
        r = cor.r + 70;
        cor.r = r > 255 ? 255 : r;
        cor.g = cor.g + 10;
        cor.b = cor.b / 2;
    } else if (estado == BOSS_RECUPERANDO) {
        cor.r = cor.r * 7 / 10;
        cor.g = cor.g * 7 / 10;
        cor.b = cor.b * 7 / 10;
    }

    return cor;
}

/*
 * Desenha os chefes 1, 2 e 3 de frente, ao fundo. Estilo chapado, com blocos
 * de cor, inspirado em Papers, Please.
 * 1 = Bobrovsky (preso, corpulento), 2 = Vladimir (preso revolucionário, magro),
 * 3 = Ivan (guarda da prisão, forte e lento).
 */
static void desenhar_boss_de_frente(
    int centro_x,
    int topo_y,
    int numero_boss,
    Color roupa,
    int atacando,
    int defendendo,
    int lado_ataque
) {
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

    // Tamanho do corpo de cada um.
    int meia_largura = 44;    // metade da largura do tronco
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
        manga = pele;   // braços de fora
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
        // Listras de uniforme de preso e o número no peito.
        Color listra = (Color){roupa.r / 2, roupa.g / 2, roupa.b / 2, 255};

        for (i = 0; i < 4; i++) {
            DrawRectangle(x - meia_largura, y + 52 + i * 15, meia_largura * 2, 6, listra);
        }

        DrawRectangle(x + 10, y + 56, 30, 16, (Color){225, 220, 200, 255});
        DrawText("214", x + 14, y + 59, 12, preto);
    } else if (numero_boss == 2) {
        // Casaco remendado, com botões e uma estrela vermelha.
        DrawRectangle(x - 1, y + 40, 2, alto_tronco, preto);
        DrawRectangle(x - 19, y + 76, 15, 14, (Color){90, 72, 58, 255});
        DrawRectangleLines(x - 19, y + 76, 15, 14, preto);
        desenhar_estrela(x + 11, y + 60, 8, vermelho);
    } else {
        // Uniforme de guarda: ombreiras, botões e cinto.
        DrawRectangle(x - meia_largura, y + 40, 20, 7, ouro);
        DrawRectangle(x + meia_largura - 20, y + 40, 20, 7, ouro);
        DrawRectangle(x - 2, y + 56, 4, 4, ouro);
        DrawRectangle(x - 2, y + 72, 4, 4, ouro);
        DrawRectangle(x - meia_largura, y + 92, meia_largura * 2, 9, preto);
        DrawRectangle(x - 6, y + 92, 12, 9, ouro);
    }

    // Cabeça quadrada, olhos de um pixel só.
    DrawRectangle(x - meia_cabeca, y + 2, meia_cabeca * 2, 40, pele);
    DrawRectangle(x - meia_cabeca - 4, y + 16, 4, 12, pele);
    DrawRectangle(x + meia_cabeca, y + 16, 4, 12, pele);
    DrawRectangle(x - 2, y + 22, 4, 8, (Color){180, 140, 108, 255});
    DrawRectangle(x - meia_cabeca / 2 - 2, y + 18, 5, 5, preto);
    DrawRectangle(x + meia_cabeca / 2 - 3, y + 18, 5, 5, preto);
    DrawRectangle(x - 6, y + 34, 12, 3, (Color){110, 50, 45, 255});

    if (numero_boss != 2) {
        // Sobrancelhas pesadas.
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
        // Chapéu ushanka com estrela e bigode.
        Color pelo_chapeu = (Color){70, 58, 50, 255};

        DrawRectangle(x - meia_cabeca - 5, y - 8, meia_cabeca * 2 + 10, 20, pelo_chapeu);
        DrawRectangle(x - meia_cabeca - 5, y + 10, 10, 22, pelo_chapeu);
        DrawRectangle(x + meia_cabeca - 5, y + 10, 10, 22, pelo_chapeu);
        desenhar_estrela(x, y + 2, 7, vermelho);
        DrawRectangle(x - 11, y + 29, 22, 5, preto);
    }

    // Braços e luvas, em três poses: defesa, golpe e guarda neutra.
    if (defendendo) {
        desenhar_braco(x - ombro, y + 61, x - 18, y + 39, largura_braco + 1, manga);
        desenhar_braco(x + ombro, y + 61, x + 18, y + 39, largura_braco + 1, manga);
        DrawCircle(x - 18, y + 38, raio_luva, luva);
        DrawCircle(x + 18, y + 38, raio_luva, luva);
    } else if (atacando) {
        // lado_ataque: -1 = golpe pela esquerda da tela, +1 = pela direita.
        desenhar_braco(x + lado_ataque * (ombro - 3), y + 59,
                       x + lado_ataque * (ombro + 32), y + 68, largura_braco, manga);
        DrawCircle(x + lado_ataque * (ombro + 38), y + 68, raio_luva + 1, luva);
        desenhar_braco(x - lado_ataque * (ombro - 2), y + 62,
                       x - lado_ataque * (ombro + 8), y + 82, largura_braco - 1, manga);
        DrawCircle(x - lado_ataque * (ombro + 11), y + 86, raio_luva, luva);
    } else {
        desenhar_braco(x - ombro, y + 61, x - ombro - 14, y + 84, largura_braco, manga);
        desenhar_braco(x + ombro, y + 61, x + ombro + 14, y + 84, largura_braco, manga);
        DrawCircle(x - ombro - 17, y + 88, raio_luva, luva);
        DrawCircle(x + ombro + 17, y + 88, raio_luva, luva);
    }

    // Braçadeira vermelha do revolucionário, no ombro.
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


static unsigned char clarear_canal(unsigned char v) {
    int r = v + 45;
    return (unsigned char)(r > 255 ? 255 : r);
}

// Três garras apontadas para baixo, a partir do centro da pata.
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
 * Anastasia: uma ursa de pé, vista de frente, no mesmo ponto de vista dos
 * outros chefes. Mesma lógica de poses: guarda, preparando golpe e defesa.
 */
static void desenhar_urso_de_frente(
    int centro_x,
    int topo_y,
    Color pelo,
    int atacando,
    int defendendo,
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

    // Braços e patas dianteiras.
    if (defendendo) {
        DrawLineEx((Vector2){(float)(x - 48), (float)(y + 80)},
                   (Vector2){(float)(x - 26), (float)(y + 54)}, 26, pelo);
        DrawLineEx((Vector2){(float)(x + 48), (float)(y + 80)},
                   (Vector2){(float)(x + 26), (float)(y + 54)}, 26, pelo);
        DrawCircle(x - 24, y + 50, 16, escuro);
        DrawCircle(x + 24, y + 50, 16, escuro);
        desenhar_garras(x - 24, y + 50);
        desenhar_garras(x + 24, y + 50);
    } else if (atacando) {
        // lado_ataque: -1 = golpe pela esquerda da tela, +1 = pela direita.
        DrawLineEx((Vector2){(float)(x + lado_ataque * 46), (float)(y + 76)},
                   (Vector2){(float)(x + lado_ataque * 92), (float)(y + 52)}, 26, pelo);
        DrawCircle(x + lado_ataque * 97, y + 49, 17, escuro);
        desenhar_garras(x + lado_ataque * 97, y + 49);

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
 * Juiz de camisa listrada. (x, y_pe) é o ponto dos pés; "ritmo" balança as
 * pernas enquanto ele anda (0 = parado); braco_levantado marca a contagem.
 */
static void desenhar_juiz(float x, float y_pe, float escala, float ritmo, int braco_levantado) {
    Color camisa = (Color){236, 236, 236, 255};
    Color listra = (Color){22, 22, 26, 255};
    Color calca = (Color){26, 26, 32, 255};
    Color pele = (Color){218, 178, 132, 255};
    int passo = (ritmo > 0) ? (int)(sinf(ritmo * 14.0f) * 6.0f) : 0;
    int k;

    rlPushMatrix();
    rlTranslatef(x, y_pe, 0.0f);
    rlScalef(escala, escala, 1.0f);

    // Pernas e sapatos.
    DrawRectangle(-16 + passo, -60, 13, 60, calca);
    DrawRectangle(3 - passo, -60, 13, 60, calca);
    DrawRectangle(-19 + passo, -8, 19, 8, BLACK);
    DrawRectangle(1 - passo, -8, 19, 8, BLACK);

    // Tronco listrado.
    DrawRectangle(-26, -124, 52, 66, camisa);
    for (k = 0; k < 5; k++) {
        DrawRectangle(-22 + k * 10, -124, 4, 66, listra);
    }

    // Braços: o direito marca a contagem.
    DrawLineEx((Vector2){-26.0f, -116.0f}, (Vector2){-35.0f, -70.0f}, 12, camisa);
    DrawCircle(-36, -66, 7, pele);

    if (braco_levantado) {
        DrawLineEx((Vector2){26.0f, -116.0f}, (Vector2){46.0f, -170.0f}, 12, camisa);
        DrawCircle(47, -175, 8, pele);
    } else {
        DrawLineEx((Vector2){26.0f, -116.0f}, (Vector2){50.0f, -128.0f}, 12, camisa);
        DrawCircle(53, -130, 8, pele);
    }

    // Cabeça, cabelo e gravata-borboleta.
    DrawCircle(0, -142, 18, pele);
    DrawRectangle(-18, -160, 36, 9, (Color){40, 30, 24, 255});
    DrawRectangle(-9, -124, 18, 6, (Color){190, 35, 35, 255});

    rlPopMatrix();
}

// Pequenas estrelas girando sobre a cabeça de quem foi nocauteado.
static void desenhar_estrelas(float cx, float cy, float raio) {
    float t = (float)GetTime();
    int i;

    for (i = 0; i < 3; i++) {
        float a = t * 4.0f + i * 2.094f;
        Vector2 centro = {
            cx + cosf(a) * raio,
            cy - raio * 0.8f + sinf(a) * raio * 0.35f
        };
        DrawPoly(centro, 4, raio * 0.16f + 3.0f, t * 120.0f, (Color){255, 220, 60, 255});
    }
}

// Ângulo (0 a 90) da queda, conforme a fase do nocaute.
static float angulo_nocaute(FaseKO fase, float tempo_ko) {
    float u;

    switch (fase) {
        case KO_QUEDA:
            u = tempo_ko / NOCAUTE_DURACAO_QUEDA;
            if (u > 1.0f) u = 1.0f;
            return 90.0f * u * u;
        case KO_LEVANTA:
            u = tempo_ko / NOCAUTE_DURACAO_LEVANTAR;
            if (u > 1.0f) u = 1.0f;
            return 90.0f * (1.0f - u);
        case KO_NENHUM:
            return 0.0f;
        default:
            return 90.0f;
    }
}

// Juiz e contagem acima da cabeça dele (jogador: tamanho grande; chefe: pequeno ao fundo).
static void desenhar_juiz_nocaute(AlvoKO alvo, FaseKO fase, float tempo_ko) {
    int alvo_boss = (alvo == KO_ALVO_BOSS);
    float escala = alvo_boss ? 0.55f : 1.0f;
    float x_final = alvo_boss ? 600.0f : 660.0f;
    float y_pe = alvo_boss ? 318.0f : 515.0f;
    float x = x_final;
    float ritmo = 0.0f;
    int braco = 0;

    if (fase == KO_QUEDA || fase == KO_NENHUM) {
        return;
    }

    if (fase == KO_JUIZ_ENTRA) {
        float u = tempo_ko / NOCAUTE_DURACAO_JUIZ;
        float inicio = (float)LARGURA_TELA + 60.0f;

        if (u > 1.0f) u = 1.0f;
        u = 1.0f - (1.0f - u) * (1.0f - u);
        x = inicio + (x_final - inicio) * u;
        ritmo = (float)GetTime();
    }

    if (fase == KO_CONTAGEM) {
        braco = (tempo_ko - (int)tempo_ko) < 0.5f;
    }

    desenhar_juiz(x, y_pe, escala, ritmo, braco);

    if (fase == KO_CONTAGEM) {
        int limite = alvo_boss ? NOCAUTE_CONTAGEM_BOSS : NOCAUTE_CONTAGEM_JOGADOR;
        int n = (int)tempo_ko + 1;
        int tamanho = alvo_boss ? 44 : 64;
        int y_texto = (int)(y_pe - 160.0f * escala) - tamanho - 12;
        const char *texto;
        int largura;

        if (n > limite) n = limite;
        texto = TextFormat("%d", n);
        largura = MeasureText(texto, tamanho);

        DrawText(texto, (int)x - largura / 2 + 2, y_texto + 2, tamanho, BLACK);
        DrawText(texto, (int)x - largura / 2, y_texto, tamanho, (Color){255, 205, 55, 255});
    }
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


static void desenhar_menu(int opcao) {
    Color normal = (Color){190, 195, 205, 255};
    Color destaque = (Color){255, 205, 55, 255};
    const char *rotulos[MENU_TOTAL] = {"COMEÇAR", "PLACAR", "SAIR"};
    int i;

    desenhar_arena();
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){0, 0, 0, 170});

    desenhar_texto_centralizado("BEYOND THE FALLEN", 110, 64, RAYWHITE);
    desenhar_texto_centralizado("Sobreviva ao gulag. Derrube os chefes.", 195, 22, normal);

    for (i = 0; i < MENU_TOTAL; i++) {
        desenhar_texto_centralizado(
            i == opcao ? TextFormat("> %s <", rotulos[i]) : rotulos[i],
            265 + i * 55, 34, i == opcao ? destaque : normal
        );
    }

    desenhar_texto_centralizado("W/S ou SETAS: escolher    ENTER: confirmar", 460, 18, normal);
    desenhar_texto_centralizado(
        "Projeto de PIF - CESAR School", 500, 16, (Color){120, 125, 135, 255}
    );
}

// Tabela da matriz de estatísticas: uma linha por chefe.
static void desenhar_resumo(const Estatisticas *est, int y) {
    static const char *nomes[NUM_CHEFES] = {"Bobrovsky", "Vladimir", "Ivan", "Anastasia"};
    Color cinza = (Color){190, 195, 205, 255};
    Color apagado = (Color){100, 105, 115, 255};
    int i;

    DrawText("CHEFE", 150, y, 18, cinza);
    DrawText("SOCOS", 330, y, 18, cinza);
    DrawText("CONTRA-ATAQUES", 430, y, 18, cinza);
    DrawText("ESQUIVAS", 640, y, 18, cinza);
    DrawText("PONTOS", 770, y, 18, cinza);
    DrawLine(140, y + 24, 840, y + 24, apagado);

    for (i = 0; i < NUM_CHEFES; i++) {
        int linha_y = y + 32 + i * 26;
        int pontos = estat_pontos_chefe(est, i + 1);
        Color cor = (pontos > 0) ? RAYWHITE : apagado;

        DrawText(nomes[i], 150, linha_y, 20, cor);
        DrawText(TextFormat("%d", est->dados[i][STAT_SOCOS]), 330, linha_y, 20, cor);
        DrawText(TextFormat("%d", est->dados[i][STAT_CONTRA_ATAQUES]), 430, linha_y, 20, cor);
        DrawText(TextFormat("%d", est->dados[i][STAT_ESQUIVAS]), 640, linha_y, 20, cor);
        DrawText(TextFormat("%d", pontos), 770, linha_y, 20, cor);
    }
}

static void desenhar_game_over(int pontuacao, const Estatisticas *est) {
    ClearBackground((Color){22, 24, 29, 255});

    desenhar_texto_centralizado("GAME OVER", 35, 64, RED);
    desenhar_texto_centralizado("Você perdeu todos os continues.", 112, 22, RAYWHITE);
    desenhar_texto_centralizado(
        TextFormat("PONTUAÇÃO: %d", pontuacao), 150, 30, (Color){255, 205, 55, 255}
    );
    desenhar_resumo(est, 215);
    desenhar_texto_centralizado("ENTER: REGISTRAR PONTUAÇÃO", 440, 22, RAYWHITE);
}

static void abrir_placar(EstadoPlacar *ep) {
    placar_carregar(&ep->placar);
    ep->destaque = -1;
    ep->pontos_fora = -1;
    ep->opcao = 0;
    ep->confirmar_apagar = 0;
}

// Fim de partida: pede o nome se a pontuação entra no placar; senão, mostra o placar.
static Tela abrir_fim_de_partida(EstadoPlacar *ep, int pontos) {
    abrir_placar(ep);
    ep->nome[0] = '\0';
    ep->tam_nome = 0;

    if (placar_qualifica(&ep->placar, pontos)) {
        // Descarta teclas digitadas nas telas anteriores (ex.: o "G" de desistir).
        while (GetCharPressed() > 0) {
        }
        return TELA_NOME;
    }

    ep->pontos_fora = pontos;
    return TELA_PLACAR;
}

static Rectangle retangulo_botao_placar(int indice) {
    if (indice == 0) {
        return (Rectangle){190, 462, 220, 46};
    }

    return (Rectangle){430, 462, 340, 46};
}

static void desenhar_botao(Rectangle r, const char *texto, int selecionado) {
    Color dourado = (Color){255, 205, 55, 255};
    int largura = MeasureText(texto, 22);

    DrawRectangleRec(
        r, selecionado ? (Color){70, 58, 20, 235} : (Color){30, 32, 38, 235}
    );
    DrawRectangleLinesEx(r, 2, selecionado ? dourado : (Color){110, 115, 125, 255});
    DrawText(
        texto,
        (int)(r.x + (r.width - largura) / 2),
        (int)(r.y + (r.height - 22) / 2),
        22,
        selecionado ? dourado : (Color){190, 195, 205, 255}
    );
}

static void desenhar_placar(const EstadoPlacar *ep) {
    Color cinza = (Color){190, 195, 205, 255};
    Color dourado = (Color){255, 205, 55, 255};
    int i;

    desenhar_arena();
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){0, 0, 0, 205});

    desenhar_texto_centralizado("PLACAR", 24, 48, RAYWHITE);

    DrawText("POS", 140, 100, 18, cinza);
    DrawText("NOME", 230, 100, 18, cinza);
    DrawText("PONTOS", 340, 100, 18, cinza);
    DrawText("DATA E HORA", 520, 100, 18, cinza);
    DrawLine(120, 124, 840, 124, (Color){110, 115, 125, 255});

    if (ep->placar.quantidade == 0) {
        desenhar_texto_centralizado("Nenhum recorde ainda.", 250, 24, cinza);
    }

    i = 0;
    for (const NoPlacar *no = ep->placar.inicio; no != NULL; no = no->proximo, i++) {
        const Registro *r = &no->registro;
        int y = 134 + i * 28;
        Color cor = (i == ep->destaque) ? dourado : RAYWHITE;

        if (i == ep->destaque) {
            DrawRectangle(120, y - 4, 720, 28, (Color){255, 205, 55, 45});
        }

        DrawText(TextFormat("%2d.", i + 1), 140, y, 20, cor);
        DrawText(r->nome, 230, y, 20, cor);
        DrawText(TextFormat("%d", r->pontos), 340, y, 20, cor);
        DrawText(
            TextFormat("%02d/%02d/%04d  %02d:%02d",
                       r->dia, r->mes, r->ano, r->hora, r->minuto),
            520, y, 20, cor
        );
    }

    if (ep->pontos_fora >= 0) {
        desenhar_texto_centralizado(
            TextFormat("Sua pontuação (%d) não entrou no top %d.",
                       ep->pontos_fora, PLACAR_MAX_REGISTROS),
            424, 18, cinza
        );
    }

    desenhar_botao(retangulo_botao_placar(0), "VOLTAR", ep->opcao == 0);
    desenhar_botao(
        retangulo_botao_placar(1),
        ep->confirmar_apagar ? "CONFIRMAR: APAGAR TUDO" : "APAGAR RECORDES",
        ep->opcao == 1
    );

    desenhar_texto_centralizado(
        "A/D ou SETAS: escolher    ENTER ou mouse: confirmar",
        516, 14, (Color){120, 125, 135, 255}
    );
}

static void desenhar_tela_nome(const EstadoPlacar *ep, const Registro *r) {
    Color cinza = (Color){190, 195, 205, 255};
    Color dourado = (Color){255, 205, 55, 255};
    int i;
    int largura_caixa = 70;
    int espaco = 20;
    int x0 = (LARGURA_TELA - (3 * largura_caixa + 2 * espaco)) / 2;

    ClearBackground((Color){22, 24, 29, 255});

    desenhar_texto_centralizado("NOVO RECORDE!", 55, 52, dourado);
    desenhar_texto_centralizado(TextFormat("PONTUAÇÃO: %d", r->pontos), 135, 32, RAYWHITE);
    desenhar_texto_centralizado(
        TextFormat("Partida encerrada em %02d/%02d/%04d às %02d:%02d",
                   r->dia, r->mes, r->ano, r->hora, r->minuto),
        185, 20, cinza
    );
    desenhar_texto_centralizado("DIGITE 3 LETRAS PARA O PLACAR", 250, 22, RAYWHITE);

    for (i = 0; i < PLACAR_TAM_NOME; i++) {
        Rectangle caixa = {(float)(x0 + i * (largura_caixa + espaco)), 295, (float)largura_caixa, 90};
        int ativa = (i == ep->tam_nome);

        DrawRectangleRec(caixa, (Color){30, 32, 38, 255});
        DrawRectangleLinesEx(caixa, 3, ativa ? dourado : (Color){110, 115, 125, 255});

        if (i < ep->tam_nome) {
            char letra[2] = {ep->nome[i], '\0'};
            int w = MeasureText(letra, 60);
            DrawText(letra, (int)(caixa.x + (caixa.width - w) / 2), 310, 60, RAYWHITE);
        } else if (ativa && ((int)(GetTime() * 2.0) % 2 == 0)) {
            DrawRectangle((int)caixa.x + 15, 360, 40, 5, dourado);
        }
    }

    desenhar_texto_centralizado(
        ep->tam_nome == PLACAR_TAM_NOME ? "ENTER: CONFIRMAR" : "Faltam letras para confirmar",
        425, 22, ep->tam_nome == PLACAR_TAM_NOME ? dourado : cinza
    );
    desenhar_texto_centralizado("BACKSPACE: apagar a última letra", 465, 18, cinza);
}

/*
 * Transição entre o menu e a primeira luta: contagem regressiva de
 * TEMPO_TRANSICAO segundos, com fade-in no início e fade-out no final.
 */
static void desenhar_transicao(float restante) {
    Color cinza = (Color){190, 195, 205, 255};
    Color dourado = (Color){255, 205, 55, 255};

    float decorrido = TEMPO_TRANSICAO - restante;
    float alfa = 1.0f;

    if (decorrido < 1.0f) alfa = decorrido;
    if (restante < alfa) alfa = restante;
    if (alfa < 0.0f) alfa = 0.0f;
    if (alfa > 1.0f) alfa = 1.0f;

    int segundos = (int)restante;
    if (restante > (float)segundos) segundos++;

    ClearBackground((Color){12, 13, 16, 255});

    desenhar_texto_centralizado("PREPARE-SE", 60, 40, Fade(RAYWHITE, alfa));
    desenhar_texto_centralizado("A LUTA COMEÇA EM", 125, 24, Fade(cinza, alfa));
    desenhar_texto_centralizado(TextFormat("%d", segundos), 165, 110, Fade(dourado, alfa));

    // Barra de progresso da contagem.
    int largura_barra = 400;
    int barra_x = (LARGURA_TELA - largura_barra) / 2;
    DrawRectangle(barra_x, 310, largura_barra, 10, Fade(DARKGRAY, alfa));
    DrawRectangle(
        barra_x, 310,
        (int)(largura_barra * decorrido / TEMPO_TRANSICAO),
        10,
        Fade(dourado, alfa)
    );

    desenhar_texto_centralizado("A / D : ESQUIVAR para o lado OPOSTO ao braço do chefe", 365, 20, Fade(cinza, alfa));
    desenhar_texto_centralizado("S : DEFENDER (reduz o dano pela metade)", 400, 20, Fade(cinza, alfa));
    desenhar_texto_centralizado("J / K : SOCO (quando o chefe estiver se recuperando)", 435, 20, Fade(cinza, alfa));
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
    float intervalo_ataque = 2.0f;

    const float duracao_preparacao = 0.9f;
    const float duracao_recuperacao = 1.5f;

    int soco_esquerdo = 0;
    int soco_direito = 0;

    // Menu, transição e sistema de continues.
    int opcao_menu = MENU_COMECAR;
    int sair_do_jogo = 0;
    int iniciar_combate = 0;        // pede para (re)começar a luta atual
    int revivido = 0;               // jogador já usou o "reviver" nesta luta?
    int boss_reviveu = 0;           // chefe já usou o "reviver" nesta luta?
    int continues = CONTINUES_INICIAIS;
    float tempo_transicao = TEMPO_TRANSICAO;

    // Nocaute (vale para o jogador e para os chefes).
    FaseKO fase_ko = KO_NENHUM;
    AlvoKO alvo_ko = KO_ALVO_JOGADOR;
    int ko_definitivo = 0;          // 1 = não haverá contagem nem reviver
    float tempo_ko = 0;

    // Pontuação (calculada a partir da matriz de estatísticas) e placar.
    Estatisticas estatisticas;
    Estatisticas *est = &estatisticas;
    int pontuacao = 0;

    estat_zerar(est);

    Registro novo_registro = {{0}, 0, 1, 1, 2026, 0, 0};
    EstadoPlacar ep;

    memset(&ep, 0, sizeof(ep));
    placar_iniciar(&ep.placar);
    ep.destaque = -1;
    ep.pontos_fora = -1;

    while (!WindowShouldClose() && !sair_do_jogo) {
        float tempo = GetFrameTime();

        if (tela == TELA_MENU) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                opcao_menu = (opcao_menu + MENU_TOTAL - 1) % MENU_TOTAL;
            }

            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                opcao_menu = (opcao_menu + 1) % MENU_TOTAL;
            }

            if (IsKeyPressed(KEY_ENTER)) {
                if (opcao_menu == MENU_COMECAR) {
                    // Novo jogo: sempre recomeça do primeiro chefe.
                    numero_boss = 1;
                    continues = CONTINUES_INICIAIS;
                    revivido = 0;
                    boss_reviveu = 0;
                    estat_zerar(est);
                    pontuacao = 0;
                    fase_ko = KO_NENHUM;
                    tempo_transicao = TEMPO_TRANSICAO;
                    tela = TELA_TRANSICAO;
                } else if (opcao_menu == MENU_PLACAR) {
                    abrir_placar(&ep);
                    tela = TELA_PLACAR;
                } else {
                    sair_do_jogo = 1;
                }
            }
        } else if (tela == TELA_TRANSICAO) {
            tempo_transicao -= tempo;

            if (tempo_transicao <= 0) {
                tempo_transicao = 0;
                iniciar_combate = 1;
            }
        } else if (tela == TELA_COMBATE && fase_ko != KO_NENHUM) {
            // ---------- Sequência de nocaute ----------
            int encerrar_luta = 0;

            tempo_ko += tempo;

            switch (fase_ko) {
                case KO_QUEDA:
                    if (tempo_ko >= NOCAUTE_DURACAO_QUEDA) {
                        fase_ko = KO_JUIZ_ENTRA;
                        tempo_ko = 0;
                    }
                    break;

                case KO_JUIZ_ENTRA:
                    if (tempo_ko >= NOCAUTE_DURACAO_JUIZ) {
                        fase_ko = ko_definitivo ? KO_FIM : KO_CONTAGEM;
                        tempo_ko = 0;
                    }
                    break;

                case KO_CONTAGEM:
                    if (alvo_ko == KO_ALVO_JOGADOR) {
                        if (IsKeyPressed(KEY_SPACE)) {
                            // Levantou a tempo: volta com metade da vida.
                            jogador.hp = PLAYER_VIDA_REVIVER;
                            revivido = 1;
                            fase_ko = KO_LEVANTA;
                            tempo_ko = 0;
                        } else if (tempo_ko >= NOCAUTE_CONTAGEM_JOGADOR) {
                            // Contagem esgotada: luta perdida.
                            encerrar_luta = 1;
                        }
                    } else if (tempo_ko >= NOCAUTE_CONTAGEM_BOSS) {
                        // O chefe sempre se levanta quando o juiz chega a 5.
                        boss.hp = obter_vida_maxima_boss(numero_boss) / 2;
                        boss_reviveu = 1;
                        fase_ko = KO_LEVANTA;
                        tempo_ko = 0;
                    }
                    break;

                case KO_LEVANTA:
                    if (tempo_ko >= NOCAUTE_DURACAO_LEVANTAR) {
                        fase_ko = KO_NENHUM;
                        tempo_ko = 0;
                        resultado = COMBATE_EM_ANDAMENTO;

                        // Depois da intervenção do juiz, o chefe volta em guarda.
                        estado_boss = BOSS_GUARDANDO;
                        tempo_estado = 0;
                        tempo_recuperacao_soco = 0;
                        tempo_contra_ataque = 0;
                        esquiva_perfeita = 0;
                    }
                    break;

                case KO_FIM:
                    if (tempo_ko >= NOCAUTE_DURACAO_FIM) {
                        encerrar_luta = 1;
                    }
                    break;

                default:
                    break;
            }

            if (encerrar_luta) {
                fase_ko = KO_NENHUM;
                tempo_ko = 0;
                tela = TELA_FINAL;

                if (resultado == COMBATE_DERROTA) {
                    // Perdeu a luta de vez: gasta um continue.
                    if (continues > 0) {
                        continues--;
                    } else {
                        tela = TELA_GAME_OVER;
                        placar_registro_agora(&novo_registro, pontuacao);
                    }
                } else if (numero_boss == 4) {
                    // Campanha concluída.
                    placar_registro_agora(&novo_registro, pontuacao);
                }
            }
        } else if (tela == TELA_COMBATE) {
            combat_atualizar(&jogador, tempo);
            boss_update(&boss); //função da nova animação do boss

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

                int contra_ataque = (tempo_contra_ataque > 0 && esquiva_perfeita);
                int dano_original = jogador.dano;

                soco_esquerdo = IsKeyPressed(KEY_J);
                soco_direito = IsKeyPressed(KEY_K);
                tempo_soco = 0.2f;
                tempo_recuperacao_soco = 0.35f;

                if (contra_ataque) {
                    jogador.dano = PLAYER_DANO_BASE * 2;
                    tempo_contra_ataque = 0;
                    esquiva_perfeita = 0;
                }

                combat_atacar(&jogador, &boss);
                jogador.dano = dano_original;

                // Recompensa por bater.
                estat_registrar(est, numero_boss,
                                contra_ataque ? STAT_CONTRA_ATAQUES : STAT_SOCOS);
                pontuacao = estat_pontuacao(est);
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
                        jogador.esquivando = 0;
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

                            // Recompensa por desviar.
                            estat_registrar(est, numero_boss, STAT_ESQUIVAS);
                            pontuacao = estat_pontuacao(est);
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

            // Empate (ambos com 0 de vida) conta como derrota do jogador.
            if (resultado == COMBATE_EMPATE) {
                resultado = COMBATE_DERROTA;
            }

            if (resultado != COMBATE_EM_ANDAMENTO) {
                // Alguém foi nocauteado: começa a animação de queda.
                if (resultado == COMBATE_DERROTA) {
                    alvo_ko = KO_ALVO_JOGADOR;
                    ko_definitivo = revivido;
                } else {
                    alvo_ko = KO_ALVO_BOSS;
                    ko_definitivo = boss_reviveu;
                }

                fase_ko = KO_QUEDA;
                tempo_ko = 0;

                jogador.defendendo = 0;
                jogador.esquivando = 0;
                direcao_esquiva = ESQUIVA_NENHUMA;
                tempo_soco = 0;
                soco_esquerdo = 0;
                soco_direito = 0;
            }
        } else if (tela == TELA_FINAL) {
            if (resultado == COMBATE_VITORIA &&
                numero_boss < 4 &&
                IsKeyPressed(KEY_ENTER)) {

                // Próximo chefe, sempre do mais fácil para o mais difícil.
                numero_boss++;
                iniciar_combate = 1;
            } else if (resultado == COMBATE_VITORIA &&
                       numero_boss == 4 &&
                       IsKeyPressed(KEY_ENTER)) {

                // Campanha concluída: registra a pontuação.
                numero_boss = 1;
                continues = CONTINUES_INICIAIS;
                revivido = 0;
                opcao_menu = MENU_COMECAR;
                tela = abrir_fim_de_partida(&ep, novo_registro.pontos);
            } else if (resultado == COMBATE_DERROTA) {
                if (IsKeyPressed(KEY_ENTER)) {
                    // Usa o continue (já descontado): recomeça a MESMA luta do zero.
                    iniciar_combate = 1;
                } else if (IsKeyPressed(KEY_G)) {
                    // Desistiu: a partida termina agora.
                    placar_registro_agora(&novo_registro, pontuacao);
                    numero_boss = 1;
                    continues = CONTINUES_INICIAIS;
                    revivido = 0;
                    opcao_menu = MENU_COMECAR;
                    tela = abrir_fim_de_partida(&ep, novo_registro.pontos);
                }
            }
        } else if (tela == TELA_GAME_OVER) {
            if (IsKeyPressed(KEY_ENTER)) {
                // Perdeu todos os continues: registra a pontuação.
                numero_boss = 1;
                continues = CONTINUES_INICIAIS;
                revivido = 0;
                opcao_menu = MENU_COMECAR;
                tela = abrir_fim_de_partida(&ep, novo_registro.pontos);
            }
        } else if (tela == TELA_NOME) {
            int c = GetCharPressed();

            while (c > 0) {
                if (c < 128 && isalpha(c) && ep.tam_nome < PLACAR_TAM_NOME) {
                    ep.nome[ep.tam_nome++] = (char)toupper(c);
                    ep.nome[ep.tam_nome] = '\0';
                }

                c = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && ep.tam_nome > 0) {
                ep.nome[--ep.tam_nome] = '\0';
            }

            if (IsKeyPressed(KEY_ENTER) && ep.tam_nome == PLACAR_TAM_NOME) {
                Registro r = novo_registro;

                strncpy(r.nome, ep.nome, PLACAR_TAM_NOME);
                r.nome[PLACAR_TAM_NOME] = '\0';

                ep.destaque = placar_inserir(&ep.placar, &r);
                placar_salvar(&ep.placar);

                ep.opcao = 0;
                ep.confirmar_apagar = 0;
                tela = TELA_PLACAR;
            }
        } else if (tela == TELA_PLACAR) {
            int ativar = IsKeyPressed(KEY_ENTER);
            Vector2 mouse = GetMousePosition();
            Vector2 movimento = GetMouseDelta();
            int i;

            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                ep.opcao = 0;
                ep.confirmar_apagar = 0;
            }

            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                ep.opcao = 1;
            }

            for (i = 0; i < 2; i++) {
                if (CheckCollisionPointRec(mouse, retangulo_botao_placar(i))) {
                    if (movimento.x != 0 || movimento.y != 0) {
                        if (ep.opcao != i) {
                            ep.confirmar_apagar = 0;
                        }
                        ep.opcao = i;
                    }

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        if (ep.opcao != i) {
                            ep.confirmar_apagar = 0;
                        }
                        ep.opcao = i;
                        ativar = 1;
                    }
                }
            }

            if (ativar) {
                if (ep.opcao == 0) {
                    ep.confirmar_apagar = 0;
                    tela = TELA_MENU;
                } else if (!ep.confirmar_apagar) {
                    // Primeiro clique só pede confirmação.
                    ep.confirmar_apagar = 1;
                } else {
                    placar_apagar(&ep.placar);
                    ep.destaque = -1;
                    ep.pontos_fora = -1;
                    ep.confirmar_apagar = 0;
                }
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
                &quantidade_ataques,
                &soco_esquerdo,
                &soco_direito,
                &esquiva_perfeita,
                &resultado
            );

            revivido = 0;
            boss_reviveu = 0;
            fase_ko = KO_NENHUM;
            tempo_ko = 0;
            direcao_esquiva = ESQUIVA_NENHUMA;

            intervalo_ataque = 2.8f - boss.velocidade * 0.15f;
            if (intervalo_ataque < 1.4f) {
                intervalo_ataque = 1.4f;
            }

            tela = TELA_COMBATE;
        }

        BeginDrawing();

        if (tela == TELA_MENU) {
            desenhar_menu(opcao_menu);
        } else if (tela == TELA_TRANSICAO) {
            desenhar_transicao(tempo_transicao);
        } else if (tela == TELA_COMBATE) {
            int em_nocaute = (fase_ko != KO_NENHUM);
            float angulo = angulo_nocaute(fase_ko, tempo_ko);
            float ang_jogador = (em_nocaute && alvo_ko == KO_ALVO_JOGADOR) ? -angulo : 0.0f;
            float ang_boss = (em_nocaute && alvo_ko == KO_ALVO_BOSS) ? -angulo : 0.0f;

            desenhar_arena();

            DrawText("BEYOND THE FALLEN", 326, 28, 28, RAYWHITE);
            desenhar_texto_centralizado(
                TextFormat("PONTOS: %d", pontuacao), 62, 20, (Color){255, 205, 55, 255}
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

            // Continues restantes e disponibilidade do reviver.
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
            DrawText(PAPEIS_CHEFES[numero_boss - 1], 650, 58, 14, (Color){200, 190, 150, 255});
            DrawText(TextFormat("%d", boss.hp), 650, 105, 19, RAYWHITE);
            DrawRectangle(690, 111, 220, 17, DARKGRAY);
            DrawRectangle(
                690, 111,
                (int)(220.0f * hp_boss / hp_maximo_boss),
                17,
                (Color){220, 45, 55, 255}
            );
            DrawText(
                TextFormat("REVIVER: %s", boss_reviveu ? "USADO" : "DISPONÍVEL"),
                690, 135, 16,
                (Color){190, 195, 205, 255}
            );

            // Cor da roupa (ou do pelo, no caso da ursa Anastasia).
            int ursa = (numero_boss == 4);
            Color roupa_boss = cor_uniforme(numero_boss, estado_boss);

            // Chefe ao fundo, de frente para o jogador (gira ao cair).
            int topo_boss = ursa ? 140 : 157;

            // Sombras no piso para dar profundidade.
            int largura_sombra = ursa ? 66 : (numero_boss == 1 ? 56 : 42);
            DrawEllipse(480, topo_boss + (ursa ? 173 : 153), largura_sombra, 9, (Color){15, 16, 18, 120});
            DrawEllipse(480, 505, 92, 15, (Color){15, 16, 18, 150});

            int pivo_boss_y = topo_boss + (ursa ? 110 : 105);
            int boss_em_guarda = (!em_nocaute && estado_boss == BOSS_GUARDANDO);
            int boss_atacando = (!em_nocaute && estado_boss == BOSS_PREPARANDO);
            int lado_boss = (direcao_ataque == ATAQUE_ESQUERDA) ? -1 : 1;

            rlPushMatrix();
            rlTranslatef(480.0f, (float)pivo_boss_y, 0.0f);
            rlRotatef(ang_boss, 0.0f, 0.0f, 1.0f);
            rlTranslatef(-480.0f, (float)(-pivo_boss_y), 0.0f);

            if (ursa) {
                desenhar_urso_de_frente(
                    480, topo_boss, roupa_boss,
                    boss_atacando, boss_em_guarda, lado_boss
                );
            } else {
                desenhar_boss_de_frente(
                    480, topo_boss, numero_boss, roupa_boss,
                    boss_atacando, boss_em_guarda, lado_boss
                );
            }

            rlPopMatrix();

            if (em_nocaute && alvo_ko == KO_ALVO_BOSS) {
                // Cabeça do chefe caído: à esquerda do pivô.
                desenhar_estrelas(
                    480.0f - (float)(pivo_boss_y - (topo_boss + 25)),
                    (float)pivo_boss_y,
                    22.0f
                );
                desenhar_juiz_nocaute(alvo_ko, fase_ko, tempo_ko);
            }

            // Jogador em primeiro plano, visto de costas (gira ao cair).
            int direcao_visual_esquiva = 0;
            if (direcao_esquiva == ESQUIVA_ESQUERDA) {
                direcao_visual_esquiva = -1;
            } else if (direcao_esquiva == ESQUIVA_DIREITA) {
                direcao_visual_esquiva = 1;
            }

            rlPushMatrix();
            rlTranslatef(480.0f, 445.0f, 0.0f);
            rlRotatef(ang_jogador, 0.0f, 0.0f, 1.0f);
            rlTranslatef(-480.0f, -445.0f, 0.0f);

            desenhar_jogador_de_costas(
                480,
                270,
                tempo_soco > 0 && (soco_esquerdo || soco_direito),
                jogador.defendendo,
                jogador.esquivando,
                direcao_visual_esquiva
            );

            rlPopMatrix();

            if (jogador.defendendo) {
                DrawEllipseLines(
                    480, 386, 91, 113,
                    (Color){80, 170, 255, 180}
                );
            }

            if (em_nocaute && alvo_ko == KO_ALVO_JOGADOR) {
                desenhar_estrelas(480.0f - 117.0f, 445.0f, 36.0f);
                desenhar_juiz_nocaute(alvo_ko, fase_ko, tempo_ko);
            }

            if (!em_nocaute) {
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
            } else {
                // Mensagens do nocaute.
                if (tempo_ko < 0.25f && fase_ko == KO_QUEDA) {
                    DrawRectangle(
                        0, 0, LARGURA_TELA, ALTURA_TELA,
                        Fade(WHITE, 0.6f * (1.0f - tempo_ko / 0.25f))
                    );
                }

                if (fase_ko == KO_QUEDA || fase_ko == KO_JUIZ_ENTRA || fase_ko == KO_FIM) {
                    desenhar_texto_centralizado("NOCAUTE!", 172, 64, BLACK);
                    desenhar_texto_centralizado("NOCAUTE!", 168, 64, RED);
                }

                if (fase_ko == KO_CONTAGEM && alvo_ko == KO_ALVO_JOGADOR) {
                    if (((int)(GetTime() * 3.0)) % 2 == 0) {
                        desenhar_texto_centralizado(
                            "APERTE ESPAÇO PARA LEVANTAR!", 340, 30, YELLOW
                        );
                    }
                } else if (fase_ko == KO_CONTAGEM) {
                    DrawText(
                        "O chefe tenta se levantar...", 40, 505, 18,
                        (Color){190, 195, 205, 255}
                    );
                }
            }
        } else if (tela == TELA_FINAL) {
            ClearBackground((Color){22, 24, 29, 255});

            if (resultado == COMBATE_VITORIA && numero_boss < 4) {
                desenhar_texto_centralizado("VITORIA", 170, 52, GREEN);
                desenhar_texto_centralizado(
                    TextFormat("PONTOS: %d", pontuacao), 235, 24,
                    (Color){255, 205, 55, 255}
                );
                desenhar_texto_centralizado("ENTER: PRÓXIMO COMBATE", 285, 22, RAYWHITE);
            } else if (resultado == COMBATE_VITORIA) {
                desenhar_texto_centralizado("CAMPEÃO", 40, 52, (Color){255, 205, 55, 255});
                desenhar_texto_centralizado("VOCÊ VENCEU A CAMPANHA", 105, 24, RAYWHITE);
                desenhar_texto_centralizado(
                    TextFormat("PONTUAÇÃO: %d", pontuacao), 145, 30,
                    (Color){255, 205, 55, 255}
                );
                desenhar_resumo(est, 205);
                desenhar_texto_centralizado("ENTER: REGISTRAR PONTUAÇÃO", 440, 22, RAYWHITE);
            } else {
                desenhar_texto_centralizado("DERROTA", 150, 52, RED);
                desenhar_texto_centralizado(
                    TextFormat("CONTINUES RESTANTES: %d", continues),
                    235, 24, RAYWHITE
                );
                desenhar_texto_centralizado(
                    "ENTER: RECOMEÇAR A LUTA DO ZERO", 285, 24, YELLOW
                );
                desenhar_texto_centralizado(
                    "G: DESISTIR E IR PARA O PLACAR", 330, 24,
                    (Color){190, 195, 205, 255}
                );
                desenhar_texto_centralizado(
                    TextFormat("PONTUAÇÃO ATUAL: %d", pontuacao), 400, 22,
                    (Color){255, 205, 55, 255}
                );
            }
        } else if (tela == TELA_GAME_OVER) {
            desenhar_game_over(pontuacao, est);
        } else if (tela == TELA_NOME) {
            desenhar_tela_nome(&ep, &novo_registro);
        } else if (tela == TELA_PLACAR) {
            desenhar_placar(&ep);
        }

        EndDrawing();
    }

    // libera os nos da lista do placar
    placar_liberar(&ep.placar);

    CloseWindow();
    return 0;
}