/*
 * ============================================================================
 *  BEYOND THE FALLEN  -  Boss Fight 2D  (Raylib + C)
 * ============================================================================
 *  Versão "experiência surreal":
 *   - Heroi e 3 chefes desenhados proceduralmente, com esqueleto animado
 *     (idle, corrida, pulo, dash, combo de 3 golpes, dano) - sem usar imagens
 *   - Chefes: Colosso de Pedra, Senhor das Brasas e Lorde do Vazio, cada um com
 *     arena, ataques, telegrafos de ataque e fase 2 (enfurecido) proprios
 *   - Cenarios com parallax, nevoa, estrelas, particulas e brilho aditivo
 *   - Hit-stop, camera lenta, tremor, zoom de impacto, numeros de dano
 *   - Audio 100% sintetizado em codigo (efeitos + musica de menu e de batalha)
 *   - Mapa do modo historia com arvores, tochas, portoes magicos e vagalumes
 *
 *  CONTROLES (batalha)
 *   [A/D ou setas] mover     [W/Cima] pular (duplo pulo)
 *   [J/K/Espaco] atacar (aperte em sequencia para combo de 3 golpes)
 *   [Shift/L] esquiva (dash com invulnerabilidade)
 *   [Z] lock-on             [E/P/Esc] pausar
 *
 *  COMPILAR (Linux):   gcc menu.c -o jogo -lraylib -lm -lGL -lpthread -ldl -lrt -lX11
 *  COMPILAR (Windows): gcc menu.c -o jogo.exe -lraylib -lopengl32 -lgdi32 -lwinmm
 * ============================================================================
 */

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define LARGURA_TELA 900
#define ALTURA_TELA 500
#define CHAO_Y 400
#define GRAVIDADE 1200.0f
#define FORCA_PULO -550.0f
#define MAX_PARTICULAS 900
#define MAX_TEXTOS 40
#define MAX_PROJETEIS 24
#define MAX_RASTROS 10
#define LARGURA_MAPA 3000
#define NUM_BOSSES 3

typedef enum {
    ESTADO_MENU_INICIAL,
    ESTADO_SELECAO_MODO,
    ESTADO_CARREGAR,
    ESTADO_CREDITOS,
    ESTADO_EXPLORACAO,
    ESTADO_JOGANDO,
    ESTADO_PAUSADO,
    ESTADO_FIM_DE_JOGO
} EstadoJogo;

typedef enum { MODO_HISTORIA, MODO_ARCADE } ModoJogo;
typedef enum { BOSS_COLOSSO, BOSS_DEMONIO, BOSS_VAZIO } TipoBoss;
typedef enum { B_ANDAR, B_PREPARO, B_ATAQUE, B_RECUPERA } AcaoBoss;
typedef enum { AT_GOLPE, AT_PISAO, AT_FOGO, AT_INVESTIDA, AT_ORBES, AT_TELEPORTE } AtaqueBoss;

typedef struct {
    Vector2 posicao;
    Vector2 velocidade;
    Color cor;
    bool ativa;
    float vida_util;
    float vida_max;
    float tamanho;
    float gravidade;
    int tipo; /* 0 quadrado, 1 brilho, 2 faisca, 3 fumaca, 4 anel */
} Particula;

typedef struct {
    Vector2 pos;
    char txt[16];
    Color cor;
    float vida;
    int tam;
    bool ativo;
} TextoFlutuante;

typedef struct {
    Vector2 pos, vel;
    float raio, vida, anim;
    int tipo; /* 0 bola de fogo, 1 onda de choque, 2 orbe do vazio */
    int dano;
    bool ativa;
} Projetil;

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
    /* animacao e combate (heroi) */
    float anim, duracao_ataque, squash, invencivel, empurrao;
    int combo;
    float tempo_combo;
    bool buffer_ataque, acertou;
    float tempo_dash, recarga_dash, rastro_t;
    int dir_dash, pulos;
    /* chefe */
    int tipo, acao, ataque;
    float tempo_acao, tempo_total, recarga, alvo_x, vel_x, alpha, raiva, tempo_part;
    bool enfurecido, fx;
} Personagem;

typedef struct {
    Personagem copia;
    float vida;
} Rastro;

/* ------------------------------------------------------------------------- */
/*  ESTADO GLOBAL                                                            */
/* ------------------------------------------------------------------------- */
static Sound som_ataque, som_dano, som_pulo, som_clique, som_rugido, som_fogo;
static Sound som_impacto, som_dash, som_vitoria, som_derrota;
static Sound musicas[4];
static int faixa_atual = -1;

static Particula particulas[MAX_PARTICULAS];
static TextoFlutuante textos[MAX_TEXTOS];
static Projetil projeteis[MAX_PROJETEIS];
static Rastro rastros[MAX_RASTROS];

static Vector2 pos_jogador_mapa = { 100, 250 };
static Vector2 dir_mapa = { 1, 0 };
static float cam_x_mapa = 450.0f;

static EstadoJogo estado = ESTADO_MENU_INICIAL;
static EstadoJogo estado_anterior = ESTADO_MENU_INICIAL;
static ModoJogo modo_atual = MODO_HISTORIA;
static Personagem jogador, chefao, vitrine_heroi, vitrine_boss;
static int fase_atual = 1;

static float tempo_global = 0.0f, tremor_tela = 0.0f, zoom_punch = 0.0f;
static float hit_stop = 0.0f, flash_tela = 0.0f, fade_tela = 1.0f;
static Color flash_cor = { 255, 255, 255, 255 };
static float timer_intro = 0.0f, intro_total = 2.6f;
static bool intro_queda = true, intro_impacto = false;
static float timer_fim = 0.0f, tempo_explosao = 0.0f;
static bool vitoria = false, mira_travada = false, quer_sair = false;
static int combo_hits = 0;
static float tempo_combo_visual = 0.0f;
static float vida_fantasma_jog = 100.0f, vida_fantasma_boss = 100.0f;
static char mensagem_status[64] = "";
static float timer_mensagem = 0.0f;
static float tempo_dica = 0.0f;

static const float portoes_boss[3] = { 700.0f, 1500.0f, 2400.0f };
static const char *NOMES_BOSS[NUM_BOSSES] = { "COLOSSO DE PEDRA", "SENHOR DAS BRASAS", "LORDE DO VAZIO" };
static const char *SUBS_BOSS[NUM_BOSSES] = {
    "O Guardião Esquecido", "Soberano do Abismo em Chamas", "Devorador de Estrelas"
};

/* ------------------------------------------------------------------------- */
/*  UTILITARIOS                                                              */
/* ------------------------------------------------------------------------- */
static float Rnd(float a, float b) { return a + (b - a) * ((float)GetRandomValue(0, 10000) / 10000.0f); }

static float Hash(int n) {
    float s = sinf((float)n * 127.1f) * 43758.5453f;
    return s - floorf(s);
}

static float Suave(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static Color Misturar(Color a, Color b, float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return (Color){ (unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
                    (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t) };
}

static Color AcentoBoss(int tipo) {
    if (tipo == BOSS_COLOSSO) return (Color){ 90, 255, 210, 255 };
    if (tipo == BOSS_DEMONIO) return (Color){ 255, 120, 40, 255 };
    return (Color){ 200, 90, 255, 255 };
}

/* triangulo que aceita qualquer ordem de vertices */
static void Tri(Vector2 a, Vector2 b, Vector2 c, Color col) {
    float cr = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cr < 0) DrawTriangle(a, c, b, col);
    else DrawTriangle(a, b, c, col);
}

/* brilho aditivo (aureolas, olhos, fogo) */
static float g_alpha = 1.0f, g_branco = 0.0f;
static bool g_tint_on = false;
static Color g_tint = { 255, 255, 255, 255 };

static Color Pt(Color c) { /* aplica flash de dano, silhueta e transparencia */
    if (g_tint_on) c = Misturar(c, g_tint, 0.75f);
    if (g_branco > 0.0f) c = Misturar(c, WHITE, g_branco);
    c.a = (unsigned char)(c.a * g_alpha);
    return c;
}

static void Brilho(Vector2 c, float r, Color cor, float k) {
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 5; i++) {
        float f = (float)i / 5.0f;
        DrawCircleV(c, r * (1.0f - f * 0.85f), Fade(cor, Clamp(0.045f * k * (1.0f + i) * g_alpha, 0.0f, 1.0f)));
    }
    EndBlendMode();
}

/* "Pose": origem no pe do personagem, x+ = frente, y- = cima, com espelhamento e squash */
typedef struct { float x, y, d, sx, sy; } Pose;

static Vector2 L(Pose p, float x, float y) { return (Vector2){ p.x + p.d * x * p.sx, p.y + y * p.sy }; }

static void Seg(Pose p, Vector2 a, Vector2 b, float esp, Color c) {
    Vector2 A = L(p, a.x, a.y), B = L(p, b.x, b.y);
    DrawLineEx(A, B, esp, Pt(c));
    DrawCircleV(A, esp * 0.5f, Pt(c));
    DrawCircleV(B, esp * 0.5f, Pt(c));
}

/* osso: parte de 'ini' com angulo (0 = para baixo, PI/2 = para frente, PI = para cima) */
static Vector2 Osso(Pose p, Vector2 ini, float ang, float comp, float esp, Color c) {
    Vector2 fim = { ini.x + sinf(ang) * comp, ini.y + cosf(ang) * comp };
    Seg(p, ini, fim, esp, c);
    return fim;
}

static void Ret(Pose p, float x0, float y0, float x1, float y1, float arred, Color c) {
    Vector2 a = L(p, x0, y0), b = L(p, x1, y1);
    Rectangle r = { fminf(a.x, b.x), fminf(a.y, b.y), fabsf(b.x - a.x), fabsf(b.y - a.y) };
    if (arred > 0.0f) DrawRectangleRounded(r, arred, 4, Pt(c));
    else DrawRectangleRec(r, Pt(c));
}

static void TriL(Pose p, Vector2 a, Vector2 b, Vector2 c, Color col) {
    Tri(L(p, a.x, a.y), L(p, b.x, b.y), L(p, c.x, c.y), Pt(col));
}

static void Circ(Pose p, float x, float y, float r, Color c) {
    DrawCircleV(L(p, x, y), r, Pt(c));
}

/* arco/crescente (aneis parciais) - angulos na mesma convencao dos ossos */
static void Arco(Pose p, Vector2 centro, float r0, float r1, float a0, float a1, Color cor) {
    const int N = 12;
    for (int k = 0; k < N; k++) {
        float u0 = (float)k / N, u1 = (float)(k + 1) / N;
        float t0 = a0 + (a1 - a0) * u0, t1 = a0 + (a1 - a0) * u1;
        Vector2 p0o = L(p, centro.x + sinf(t0) * r1, centro.y + cosf(t0) * r1);
        Vector2 p1o = L(p, centro.x + sinf(t1) * r1, centro.y + cosf(t1) * r1);
        Vector2 p0i = L(p, centro.x + sinf(t0) * r0, centro.y + cosf(t0) * r0);
        Vector2 p1i = L(p, centro.x + sinf(t1) * r0, centro.y + cosf(t1) * r0);
        Color c = cor;
        c.a = (unsigned char)(cor.a * u1 * g_alpha);
        Tri(p0o, p1o, p1i, c);
        Tri(p0o, p1i, p0i, c);
    }
}

static void TextoCentro(const char *t, int y, int tam, Color c) {
    DrawText(t, LARGURA_TELA / 2 - MeasureText(t, tam) / 2, y, tam, c);
}

static void TextoTitulo(const char *t, float cx, float y, float tam, float esp, Color c, Color brilho) {
    Font f = GetFontDefault();
    Vector2 m = MeasureTextEx(f, t, tam, esp);
    Vector2 pos = { cx - m.x / 2.0f, y };
    for (int i = 0; i < 8; i++) {
        float a = (float)i * PI / 4.0f;
        DrawTextEx(f, t, (Vector2){ pos.x + cosf(a) * 3.0f, pos.y + sinf(a) * 3.0f }, tam, esp, Fade(brilho, 0.22f));
    }
    DrawTextEx(f, t, (Vector2){ pos.x + 2, pos.y + 3 }, tam, esp, Fade(BLACK, 0.65f));
    DrawTextEx(f, t, pos, tam, esp, c);
}

/* ------------------------------------------------------------------------- */
/*  AUDIO SINTETICO                                                          */
/* ------------------------------------------------------------------------- */
static float Ruido(void) { return ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f; }

static Sound SomDeBuffer(float *buf, int n, float vol) {
    Wave w = { 0 };
    w.frameCount = (unsigned int)n;
    w.sampleRate = 44100;
    w.sampleSize = 16;
    w.channels = 1;
    short *d = (short *)RL_MALLOC(n * sizeof(short));
    for (int i = 0; i < n; i++) d[i] = (short)(Clamp(buf[i], -1.0f, 1.0f) * 30000.0f);
    w.data = d;
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    SetSoundVolume(s, vol);
    free(buf);
    return s;
}

static Sound GerarSomAtaque(void) {
    const int sr = 44100; int n = (int)(sr * 0.18f);
    float *b = (float *)malloc(n * sizeof(float)); float lp = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float)i / sr, u = t / 0.18f;
        float env = sinf(PI * u); env *= env;
        lp += (Ruido() - lp) * (0.04f + 0.45f * u);
        b[i] = lp * env * 2.2f + sinf(2.0f * PI * (900.0f - 700.0f * u) * t) * env * 0.12f;
    }
    return SomDeBuffer(b, n, 0.35f);
}

static Sound GerarSomDano(void) {
    const int sr = 44100; int n = (int)(sr * 0.25f);
    float *b = (float *)malloc(n * sizeof(float)); float ph = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float)i / sr;
        ph += 2.0f * PI * (55.0f + 150.0f * expf(-t * 28.0f)) / sr;
        b[i] = sinf(ph) * expf(-t * 12.0f) * 0.9f + Ruido() * expf(-t * 35.0f) * 0.5f;
    }
    return SomDeBuffer(b, n, 0.5f);
}

static Sound GerarSomPulo(void) {
    const int sr = 44100; int n = (int)(sr * 0.14f);
    float *b = (float *)malloc(n * sizeof(float)); float ph = 0.0f;
    for (int i = 0; i < n; i++) {
        float u = (float)i / n;
        ph += 2.0f * PI * (180.0f + 380.0f * u) / sr;
        b[i] = sinf(ph) * (1.0f - u) * 0.45f;
    }
    return SomDeBuffer(b, n, 0.3f);
}

static Sound GerarSomClique(void) {
    const int sr = 44100; int n = (int)(sr * 0.06f);
    float *b = (float *)malloc(n * sizeof(float));
    for (int i = 0; i < n; i++) {
        float t = (float)i / sr, u = t / 0.06f;
        b[i] = (sinf(2.0f * PI * 880.0f * t) * 0.6f + sinf(2.0f * PI * 1320.0f * t) * 0.25f) * (1.0f - u);
    }
    return SomDeBuffer(b, n, 0.3f);
}

static Sound GerarSomRugido(void) {
    const int sr = 44100; int n = sr; float *b = (float *)malloc(n * sizeof(float));
    float ph = 0.0f, lp = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float)i / sr, u = t;
        ph += 2.0f * PI * (78.0f - 28.0f * u + 6.0f * sinf(t * 38.0f)) / sr;
        float x = ph / (2.0f * PI);
        float saw = 2.0f * (x - floorf(x)) - 1.0f;
        float env = fminf(u / 0.1f, 1.0f) * powf(1.0f - u, 1.5f);
        float v = (saw * 0.6f + sinf(ph * 0.5f) * 0.5f + Ruido() * 0.25f) * env;
        lp += (v - lp) * 0.18f;
        b[i] = lp * 1.6f;
    }
    return SomDeBuffer(b, n, 0.7f);
}

static Sound GerarSomFogo(void) {
    const int sr = 44100; int n = (int)(sr * 0.4f); float *b = (float *)malloc(n * sizeof(float));
    float lp = 0.0f, ph = 0.0f;
    for (int i = 0; i < n; i++) {
        float u = (float)i / n;
        lp += (Ruido() - lp) * (0.5f - 0.42f * u);
        ph += 2.0f * PI * (240.0f - 150.0f * u) / sr;
        b[i] = lp * (1.0f - u) * 1.8f + sinf(ph) * 0.3f * (1.0f - u);
    }
    return SomDeBuffer(b, n, 0.45f);
}

static Sound GerarSomImpacto(void) {
    const int sr = 44100; int n = (int)(sr * 0.7f); float *b = (float *)malloc(n * sizeof(float));
    float ph = 0.0f, lp = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float)i / sr;
        ph += 2.0f * PI * (28.0f + 95.0f * expf(-t * 5.0f)) / sr;
        lp += (Ruido() - lp) * 0.12f;
        b[i] = sinf(ph) * expf(-t * 5.5f) + lp * expf(-t * 8.0f) * 1.2f;
    }
    return SomDeBuffer(b, n, 0.6f);
}

static Sound GerarSomDash(void) {
    const int sr = 44100; int n = (int)(sr * 0.2f); float *b = (float *)malloc(n * sizeof(float));
    float lp = 0.0f;
    for (int i = 0; i < n; i++) {
        float u = (float)i / n; float r = Ruido();
        lp += (r - lp) * 0.3f;
        b[i] = (r - lp) * sinf(PI * u) * 0.8f;
    }
    return SomDeBuffer(b, n, 0.35f);
}

static Sound GerarSomVitoria(void) {
    const int sr = 44100; int n = (int)(sr * 1.6f); float *b = (float *)calloc(n, sizeof(float));
    float notas[4] = { 523.25f, 659.25f, 783.99f, 1046.5f };
    for (int i = 0; i < n; i++) {
        float t = (float)i / sr;
        for (int k = 0; k < 4; k++) {
            float tt = t - k * 0.13f;
            if (tt < 0.0f) continue;
            float env = expf(-tt * 3.0f) * fminf(tt / 0.01f, 1.0f);
            b[i] += (sinf(2.0f * PI * notas[k] * tt) * 0.3f + sinf(4.0f * PI * notas[k] * tt) * 0.1f) * env;
        }
    }
    return SomDeBuffer(b, n, 0.6f);
}

static Sound GerarSomDerrota(void) {
    const int sr = 44100; int n = (int)(sr * 2.0f); float *b = (float *)calloc(n, sizeof(float));
    float notas[4] = { 440.0f, 349.23f, 293.66f, 220.0f };
    for (int i = 0; i < n; i++) {
        float t = (float)i / sr;
        for (int k = 0; k < 4; k++) {
            float tt = t - k * 0.32f;
            if (tt < 0.0f) continue;
            float env = expf(-tt * 2.2f) * fminf(tt / 0.02f, 1.0f);
            b[i] += (sinf(2.0f * PI * notas[k] * tt) * 0.3f + sinf(4.0f * PI * notas[k] * tt) * 0.15f
                     + sinf(6.0f * PI * notas[k] * tt) * 0.08f) * env;
        }
    }
    return SomDeBuffer(b, n, 0.55f);
}

static float NotaPluck(float f, float ts, float decay) {
    if (ts < 0.0f) return 0.0f;
    float env = expf(-ts * decay) * fminf(ts / 0.004f, 1.0f);
    return (sinf(2.0f * PI * f * ts) + 0.35f * sinf(4.0f * PI * f * ts)) * env;
}

/* musica em loop perfeito: progressao Am - F - C - G */
static Sound GerarMusica(float bpm, int batalha, int transpor) {
    const int sr = 44100;
    float beat = 60.0f / bpm, barlen = beat * 4.0f, dur = beat * 16.0f;
    int n = (int)(dur * sr);
    float *b = (float *)calloc(n, sizeof(float));
    const float semi[4] = { 0, -4, 3, -2 };
    const int menor[4] = { 1, 0, 0, 0 };
    const int patt[8] = { 0, 1, 2, 1, 0, 2, 1, 2 };
    const float oit[4] = { 1.0f, 1.0f, 2.0f, 1.0f };

    float passo_arp = beat * (batalha ? 0.25f : 0.5f);
    float passo_bx = beat * (batalha ? 0.5f : 2.0f);
    int total_arp = (int)roundf(dur / passo_arp), total_bx = (int)roundf(dur / passo_bx);

    for (int i = 0; i < n; i++) {
        float t = (float)i / sr;
        int bar = (int)(t / barlen); if (bar > 3) bar = 3;
        float tb = t - bar * barlen;
        float root = 110.0f * powf(2.0f, (semi[bar] + transpor) / 12.0f);
        int tons[3] = { 0, menor[bar] ? 3 : 4, 7 };
        float v = 0.0f;

        /* pad */
        float penv = fminf(tb / 0.25f, 1.0f) * fminf((barlen - tb) / 0.35f, 1.0f);
        float trem = 0.8f + 0.2f * sinf(2.0f * PI * 0.25f * t);
        for (int k = 0; k < 3; k++)
            v += sinf(2.0f * PI * root * 2.0f * powf(2.0f, tons[k] / 12.0f) * t) * 0.03f * penv * trem;

        /* baixo (com cauda sobreposta para nao estalar) */
        int sb = (int)(t / passo_bx);
        for (int back = 0; back < 2; back++) {
            int sk = sb - back; float ts = t - sk * passo_bx;
            int skw = ((sk % total_bx) + total_bx) % total_bx;
            float tk = skw * passo_bx; int bk = (int)(tk / barlen); if (bk > 3) bk = 3;
            float rk = 110.0f * powf(2.0f, (semi[bk] + transpor) / 12.0f);
            float f = rk * (batalha ? oit[skw % 4] : 1.0f);
            v += NotaPluck(f, ts, batalha ? 7.0f : 1.0f) * (batalha ? 0.22f : 0.2f);
        }

        /* arpejo */
        int sa = (int)(t / passo_arp);
        for (int back = 0; back < 3; back++) {
            int sk = sa - back; float ts = t - sk * passo_arp;
            int skw = ((sk % total_arp) + total_arp) % total_arp;
            float tk = skw * passo_arp; int bk = (int)(tk / barlen); if (bk > 3) bk = 3;
            float rk = 110.0f * powf(2.0f, (semi[bk] + transpor) / 12.0f);
            int tk3[3] = { 0, menor[bk] ? 3 : 4, 7 };
            float f = rk * (batalha ? 2.0f : 4.0f) * powf(2.0f, tk3[patt[skw % 8]] / 12.0f);
            v += NotaPluck(f, ts, batalha ? 9.0f : 3.5f) * (batalha ? 0.07f : 0.055f);
        }

        /* percussao */
        if (batalha) {
            float tsb = fmodf(t, beat);
            float ph = 2.0f * PI * (45.0f * tsb + (110.0f / 30.0f) * (1.0f - expf(-30.0f * tsb)));
            v += sinf(ph) * expf(-tsb * 14.0f) * 0.45f;
            int bi = (int)(t / beat);
            if (bi % 2 == 1) v += Ruido() * expf(-tsb * 18.0f) * 0.14f;
            float th = tsb - beat * 0.5f;
            if (th > 0.0f) v += Ruido() * expf(-th * 60.0f) * 0.07f;
        }
        b[i] = tanhf(v * 1.3f);
    }
    return SomDeBuffer(b, n, 0.4f);
}

/* ------------------------------------------------------------------------- */
/*  PARTICULAS, TEXTOS E EFEITOS                                             */
/* ------------------------------------------------------------------------- */
static void Part(Vector2 pos, Vector2 vel, Color cor, float vida, float tam, int tipo, float grav) {
    for (int j = 0; j < MAX_PARTICULAS; j++) {
        if (!particulas[j].ativa) {
            particulas[j] = (Particula){ .posicao = pos, .velocidade = vel, .cor = cor, .ativa = true,
                                         .vida_util = vida, .vida_max = vida, .tamanho = tam,
                                         .gravidade = grav, .tipo = tipo };
            return;
        }
    }
}

static void Rajada(Vector2 pos, Color cor, int qtd, float vmax, float tam, float vida, float grav, int tipo) {
    for (int i = 0; i < qtd; i++) {
        float ang = Rnd(0.0f, 2.0f * PI), v = Rnd(vmax * 0.2f, vmax);
        Part(pos, (Vector2){ cosf(ang) * v, sinf(ang) * v }, cor, vida * Rnd(0.6f, 1.2f), tam * Rnd(0.6f, 1.3f), tipo, grav);
    }
}

static void Anel(Vector2 pos, Color cor, float raio, float vida) {
    Part(pos, (Vector2){ 0, 0 }, cor, vida, raio, 4, 0.0f);
}

static void ImpactoVisual(Vector2 pos, Color cor, int qtd) {
    Rajada(pos, cor, qtd, 420.0f, 3.0f, 0.45f, 700.0f, 2);
    Rajada(pos, cor, qtd / 2, 260.0f, 5.0f, 0.4f, 500.0f, 0);
    Rajada(pos, WHITE, 3, 60.0f, 16.0f, 0.18f, 0.0f, 1);
    Anel(pos, cor, 46.0f, 0.25f);
}

static void GerarParticulas(Vector2 pos, Color cor, int quantidade) { ImpactoVisual(pos, cor, quantidade); }

static void LimparEfeitos(void) {
    for (int i = 0; i < MAX_PARTICULAS; i++) particulas[i].ativa = false;
    for (int i = 0; i < MAX_TEXTOS; i++) textos[i].ativo = false;
    for (int i = 0; i < MAX_PROJETEIS; i++) projeteis[i].ativa = false;
    for (int i = 0; i < MAX_RASTROS; i++) rastros[i].vida = 0.0f;
}

static void AtualizarParticulas(float dt) {
    for (int i = 0; i < MAX_PARTICULAS; i++) {
        Particula *p = &particulas[i];
        if (!p->ativa) continue;
        p->vida_util -= dt;
        if (p->vida_util <= 0.0f) { p->ativa = false; continue; }
        p->posicao.x += p->velocidade.x * dt;
        p->posicao.y += p->velocidade.y * dt;
        p->velocidade.y += p->gravidade * dt;
        if (p->tipo == 1 || p->tipo == 3) {
            float d = expf(-2.0f * dt);
            p->velocidade.x *= d; p->velocidade.y *= d;
        }
    }
}

static void DesenharParticulas(void) {
    for (int i = 0; i < MAX_PARTICULAS; i++) {
        Particula *p = &particulas[i];
        if (!p->ativa) continue;
        float f = p->vida_util / p->vida_max;
        if (p->tipo == 0) {
            float s = p->tamanho * f + 1.0f;
            DrawRectanglePro((Rectangle){ p->posicao.x, p->posicao.y, s, s }, (Vector2){ s / 2, s / 2 }, f * 360.0f, Fade(p->cor, f));
        } else if (p->tipo == 3) {
            DrawCircleV(p->posicao, p->tamanho * (1.0f + (1.0f - f) * 1.6f), Fade(p->cor, f * 0.45f));
        }
    }
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < MAX_PARTICULAS; i++) {
        Particula *p = &particulas[i];
        if (!p->ativa) continue;
        float f = p->vida_util / p->vida_max;
        if (p->tipo == 1) {
            DrawCircleV(p->posicao, p->tamanho * (0.4f + 0.6f * f), Fade(p->cor, f));
            DrawCircleV(p->posicao, p->tamanho * 2.2f * f, Fade(p->cor, 0.22f * f));
        } else if (p->tipo == 2) {
            Vector2 cauda = { p->posicao.x - p->velocidade.x * 0.04f, p->posicao.y - p->velocidade.y * 0.04f };
            DrawLineEx(p->posicao, cauda, p->tamanho * f + 0.6f, Fade(p->cor, f));
        } else if (p->tipo == 4) {
            float pr = 1.0f - f;
            float ext = p->tamanho * Suave(pr) + 1.0f;
            float in = fmaxf(0.0f, ext - (6.0f * f + 1.0f));
            DrawRing(p->posicao, in, ext, 0.0f, 360.0f, 36, Fade(p->cor, f * 0.9f));
        }
    }
    EndBlendMode();
}

static void NovoTexto(Vector2 pos, const char *txt, Color cor, int tam) {
    for (int i = 0; i < MAX_TEXTOS; i++) {
        if (!textos[i].ativo) {
            textos[i].ativo = true; textos[i].pos = pos; textos[i].cor = cor;
            textos[i].vida = 0.9f; textos[i].tam = tam;
            snprintf(textos[i].txt, sizeof(textos[i].txt), "%s", txt);
            return;
        }
    }
}

static void AtualizarTextos(float dt) {
    for (int i = 0; i < MAX_TEXTOS; i++) {
        if (!textos[i].ativo) continue;
        textos[i].vida -= dt;
        textos[i].pos.y -= 55.0f * dt * Clamp(textos[i].vida / 0.9f + 0.2f, 0.0f, 1.0f);
        if (textos[i].vida <= 0.0f) textos[i].ativo = false;
    }
}

static void DesenharTextos(void) {
    for (int i = 0; i < MAX_TEXTOS; i++) {
        if (!textos[i].ativo) continue;
        float f = Clamp(textos[i].vida / 0.9f, 0.0f, 1.0f);
        float pop = 1.0f + (1.0f - Clamp((0.9f - textos[i].vida) / 0.12f, 0.0f, 1.0f)) * 0.6f;
        int tam = (int)(textos[i].tam * pop);
        int w = MeasureText(textos[i].txt, tam);
        int x = (int)textos[i].pos.x - w / 2, y = (int)textos[i].pos.y;
        DrawText(textos[i].txt, x + 2, y + 2, tam, Fade(BLACK, f * 0.8f));
        DrawText(textos[i].txt, x, y, tam, Fade(textos[i].cor, Clamp(f * 2.0f, 0.0f, 1.0f)));
    }
}

/* ------------------------------------------------------------------------- */
/*  DANO, PROJETEIS                                                          */
/* ------------------------------------------------------------------------- */
static void MudarEstado(EstadoJogo novo) {
    estado = novo;
    fade_tela = 1.0f;
    if (novo == ESTADO_EXPLORACAO) {
        cam_x_mapa = Clamp(pos_jogador_mapa.x, LARGURA_TELA / 2.0f, LARGURA_MAPA - LARGURA_TELA / 2.0f);
        LimparEfeitos();
    }
}

static void DanoJogador(int dano, int dir) {
    Personagem *j = &jogador;
    if (j->invencivel > 0.0f || j->tempo_dash > 0.0f || timer_fim > 0.0f || timer_intro > 0.0f) return;
    j->vida -= dano;
    j->tempo_dano = 0.28f;
    j->invencivel = 1.1f;
    j->esta_atacando = false;
    j->buffer_ataque = false;
    j->velocidade.y = -240.0f;
    j->esta_no_chao = false;
    j->empurrao = dir * 380.0f;
    tremor_tela = 0.3f;
    hit_stop = 0.08f;
    flash_tela = 0.35f;
    flash_cor = (Color){ 255, 40, 40, 255 };
    combo_hits = 0;
    PlaySound(som_dano);
    Vector2 c = { j->rect.x + j->rect.width / 2, j->rect.y + 30 };
    ImpactoVisual(c, (Color){ 255, 90, 60, 255 }, 16);
    char t[16]; snprintf(t, sizeof(t), "-%d", dano);
    NovoTexto((Vector2){ c.x, j->rect.y - 10 }, t, (Color){ 255, 90, 90, 255 }, 26);
    if (j->vida <= 0) {
        j->vida = 0;
        vitoria = false;
        timer_fim = 2.0f;
        hit_stop = 0.18f;
        tremor_tela = 0.5f;
        Rajada(c, (Color){ 120, 230, 255, 255 }, 30, 300.0f, 10.0f, 1.2f, -120.0f, 1);
    }
}

static void NovoProjetil(int tipo, Vector2 pos, Vector2 vel, float raio, float vida, int dano) {
    for (int i = 0; i < MAX_PROJETEIS; i++) {
        if (!projeteis[i].ativa) {
            projeteis[i] = (Projetil){ .pos = pos, .vel = vel, .raio = raio, .vida = vida, .anim = 0,
                                       .tipo = tipo, .dano = dano, .ativa = true };
            return;
        }
    }
}

static void ExplodirProjetil(Projetil *p) {
    Color c = (p->tipo == 2) ? (Color){ 210, 100, 255, 255 } : (Color){ 255, 150, 40, 255 };
    ImpactoVisual(p->pos, c, 10);
    p->ativa = false;
}

static void AtualizarProjeteis(float dt) {
    Vector2 cj = { jogador.rect.x + jogador.rect.width / 2, jogador.rect.y + jogador.rect.height / 2 };
    for (int i = 0; i < MAX_PROJETEIS; i++) {
        Projetil *p = &projeteis[i];
        if (!p->ativa) continue;
        p->vida -= dt; p->anim += dt;

        if (p->tipo == 2) { /* orbe do vazio: persegue o heroi */
            Vector2 d = { cj.x - p->pos.x, cj.y - p->pos.y };
            float len = sqrtf(d.x * d.x + d.y * d.y) + 0.001f;
            float vel = chefao.enfurecido ? 230.0f : 190.0f;
            Vector2 des = { d.x / len * vel, d.y / len * vel };
            float k = 1.0f - expf(-1.6f * dt);
            p->vel.x += (des.x - p->vel.x) * k;
            p->vel.y += (des.y - p->vel.y) * k;
        }
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;

        if (p->tipo == 0) {
            if (GetRandomValue(0, 1)) Part(p->pos, (Vector2){ Rnd(-30, 30), Rnd(-30, 30) }, (Color){ 255, 130, 30, 255 }, 0.35f, 7.0f, 1, -60.0f);
            if (p->pos.y > CHAO_Y - 6) { ExplodirProjetil(p); continue; }
        } else if (p->tipo == 1) {
            p->pos.y = CHAO_Y;
            if (GetRandomValue(0, 2) == 0) Part((Vector2){ p->pos.x, CHAO_Y - 2 }, (Vector2){ Rnd(-40, 40), Rnd(-120, -30) }, (Color){ 150, 140, 150, 255 }, 0.5f, 6.0f, 3, 200.0f);
        } else {
            if (GetRandomValue(0, 1)) Part(p->pos, (Vector2){ Rnd(-25, 25), Rnd(-25, 25) }, (Color){ 190, 90, 255, 255 }, 0.4f, 8.0f, 1, 0.0f);
        }

        bool bateu = false;
        if (p->tipo == 1) {
            Rectangle r = { p->pos.x - 18, CHAO_Y - 48, 36, 48 };
            bateu = CheckCollisionRecs(r, jogador.rect);
        } else {
            bateu = CheckCollisionCircleRec(p->pos, p->raio, jogador.rect);
        }
        if (bateu && jogador.invencivel <= 0.0f && jogador.tempo_dash <= 0.0f) {
            DanoJogador(p->dano, p->vel.x >= 0 ? 1 : -1);
            ExplodirProjetil(p);
            continue;
        }
        if (p->vida <= 0.0f || p->pos.x < -80 || p->pos.x > LARGURA_TELA + 80 || p->pos.y < -150) {
            if (p->tipo == 2 && p->vida <= 0.0f) ExplodirProjetil(p);
            p->ativa = false;
        }
    }
}

static void DesenharProjeteis(void) {
    for (int i = 0; i < MAX_PROJETEIS; i++) {
        Projetil *p = &projeteis[i];
        if (!p->ativa) continue;
        g_alpha = 1.0f;
        if (p->tipo == 0) {
            float len = sqrtf(p->vel.x * p->vel.x + p->vel.y * p->vel.y) + 0.001f;
            Vector2 d = { p->vel.x / len, p->vel.y / len };
            for (int k = 3; k >= 1; k--)
                DrawCircleV((Vector2){ p->pos.x - d.x * k * 9.0f, p->pos.y - d.y * k * 9.0f }, p->raio * (1.0f - k * 0.18f), Fade((Color){ 255, 100, 20, 255 }, 0.5f - k * 0.1f));
            Brilho(p->pos, p->raio * 3.4f, (Color){ 255, 110, 20, 255 }, 2.0f);
            DrawCircleV(p->pos, p->raio, (Color){ 255, 170, 40, 255 });
            DrawCircleV(p->pos, p->raio * 0.6f, (Color){ 255, 240, 160, 255 });
        } else if (p->tipo == 1) {
            Color ac = AcentoBoss(chefao.tipo);
            Brilho((Vector2){ p->pos.x, CHAO_Y - 8 }, 46.0f, ac, 1.4f);
            for (int k = -1; k <= 1; k++) {
                float bx = p->pos.x + k * 13.0f;
                float h = 30.0f + 16.0f * (1.0f - fabsf((float)k)) + sinf(p->anim * 20.0f + k) * 3.0f;
                Tri((Vector2){ bx - 9, CHAO_Y }, (Vector2){ bx + 9, CHAO_Y }, (Vector2){ bx + k * 3.0f, CHAO_Y - h }, (Color){ 98, 88, 104, 255 });
                Tri((Vector2){ bx - 2, CHAO_Y }, (Vector2){ bx + 9, CHAO_Y }, (Vector2){ bx + k * 3.0f, CHAO_Y - h }, (Color){ 140, 130, 150, 255 });
            }
        } else {
            Color roxo = (Color){ 200, 90, 255, 255 };
            Brilho(p->pos, p->raio * 3.6f, roxo, 2.2f);
            DrawCircleV(p->pos, p->raio, (Color){ 8, 2, 18, 255 });
            DrawRing(p->pos, p->raio - 2.0f, p->raio + 1.0f, 0.0f, 360.0f, 24, Fade(roxo, 0.9f));
            for (int k = 0; k < 3; k++) {
                float a = p->anim * 6.0f + k * 2.094f;
                DrawCircleV((Vector2){ p->pos.x + cosf(a) * (p->raio + 5), p->pos.y + sinf(a) * (p->raio + 5) }, 2.0f, (Color){ 255, 120, 230, 255 });
            }
        }
    }
}

/* ------------------------------------------------------------------------- */
/*  SAVE / LOAD / CONFIGURACAO                                               */
/* ------------------------------------------------------------------------- */
static void SalvarJogo(Personagem *jog, Personagem *boss, int fase, ModoJogo modo, Vector2 pos_mapa) {
    FILE *arquivo = fopen("savegame.txt", "w");
    if (arquivo != NULL) {
        fprintf(arquivo, "%d %d %d %d %f %f\n", jog->vida, boss->vida, fase, (int)modo, pos_mapa.x, pos_mapa.y);
        fclose(arquivo);
    }
}

static bool CarregarJogo(Personagem *jog, Personagem *boss, int *fase, ModoJogo *modo, Vector2 *pos_mapa) {
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

static void ConfigurarBoss(Personagem *b, int tipo, int nivel) {
    int vida = 300 + nivel * 50;
    float w, h, vel; Color cor;
    switch (tipo) {
        case BOSS_COLOSSO: w = 100; h = 130; vel = 125.0f + nivel * 12.0f; cor = (Color){ 96, 100, 112, 255 }; break;
        case BOSS_DEMONIO: w = 88; h = 120; vel = 165.0f + nivel * 14.0f; cor = (Color){ 192, 57, 43, 255 }; break;
        default: w = 70; h = 120; vel = 130.0f + nivel * 12.0f; cor = (Color){ 110, 50, 170, 255 }; break;
    }
    if (vel > 300.0f) vel = 300.0f;
    *b = (Personagem){
        .rect = { 650, CHAO_Y - h, w, h }, .cor = cor, .vida = vida, .vida_max = vida,
        .velocidade_mov = vel, .esta_no_chao = true, .direcao = -1,
        .tipo = tipo, .acao = B_ANDAR, .alpha = 1.0f, .recarga = 1.2f
    };
    if (tipo == BOSS_VAZIO) b->rect.y = CHAO_Y - h - 28;
}

static void ReiniciarJogo(Personagem *jog, Personagem *boss, int nivel) {
    *jog = (Personagem){
        .rect = { 150, CHAO_Y - 70, 45, 70 },
        .velocidade = { 0, 0 },
        .cor = (Color){ 41, 128, 185, 255 },
        .vida = 100, .vida_max = 100,
        .velocidade_mov = 320.0f,
        .esta_no_chao = true,
        .direcao = 1
    };
    ConfigurarBoss(boss, (nivel - 1) % NUM_BOSSES, nivel);
    LimparEfeitos();
}

/* ------------------------------------------------------------------------- */
/*  CENARIOS DA ARENA (3 temas com parallax)                                 */
/* ------------------------------------------------------------------------- */
static void Estrelas(float t, int qtd, float brilho, float par, float foco) {
    for (int i = 0; i < qtd; i++) {
        float x = Hash(i * 3 + 1) * (LARGURA_TELA + 40) - 20 - foco * par;
        float y = Hash(i * 3 + 2) * (CHAO_Y - 90);
        float tw = 0.5f + 0.5f * sinf(t * (1.0f + Hash(i * 5)) * 2.0f + i);
        float s = 0.8f + Hash(i * 3) * 1.4f;
        DrawCircleV((Vector2){ x, y }, s, Fade(WHITE, brilho * (0.25f + 0.75f * tw)));
    }
}

static void Cordilheira(float base_y, float altura, float larg, int semente, Color cor, float desloc) {
    int n = (int)(LARGURA_TELA / larg) + 3;
    for (int i = -1; i < n; i++) {
        float x = i * larg - desloc;
        float h = altura * (0.45f + 0.55f * Hash(semente + i + 20));
        Tri((Vector2){ x - larg, base_y }, (Vector2){ x + larg, base_y }, (Vector2){ x, base_y - h }, cor);
    }
    DrawRectangle(0, (int)base_y - 1, LARGURA_TELA, CHAO_Y - (int)base_y + 2, cor);
}

static void Neblina(float t, Color cor, float alfa, float y) {
    for (int k = 0; k < 6; k++) {
        float x = fmodf(t * (8.0f + k * 3.0f) + k * 230.0f, LARGURA_TELA + 600.0f) - 300.0f;
        DrawEllipse((int)x, (int)(y + sinf(t * 0.3f + k) * 8.0f + (k % 2) * 14), 260.0f, 34.0f, Fade(cor, alfa));
    }
}

static void CirculoRitual(float t, Color ac) {
    float cx = LARGURA_TELA / 2.0f, cy = CHAO_Y + 55.0f;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawEllipseLines((int)cx, (int)cy, 310.0f, 38.0f, Fade(ac, 0.35f));
    DrawEllipseLines((int)cx, (int)cy, 250.0f, 30.0f, Fade(ac, 0.2f));
    for (int k = 0; k < 24; k++) {
        float a = k * 2.0f * PI / 24.0f + t * 0.2f;
        float x = cx + cosf(a) * 310.0f, y = cy + sinf(a) * 38.0f;
        DrawCircleV((Vector2){ x, y }, 2.0f, Fade(ac, 0.5f));
        if (k % 3 == 0) DrawLineEx((Vector2){ x, y }, (Vector2){ cx + cosf(a) * 250.0f, cy + sinf(a) * 30.0f }, 1.0f, Fade(ac, 0.18f));
    }
    EndBlendMode();
}

static void DesenharCenario(int tema, float t, float foco) {
    Color ac = AcentoBoss(tema);
    if (tema == BOSS_COLOSSO) {
        DrawRectangleGradientV(0, 0, LARGURA_TELA, CHAO_Y, (Color){ 8, 12, 34, 255 }, (Color){ 112, 78, 116, 255 });
        Estrelas(t, 60, 0.9f, 0.01f, foco);
        Vector2 lua = { 690.0f - foco * 0.02f, 105.0f };
        Brilho(lua, 150.0f, (Color){ 170, 180, 255, 255 }, 1.2f);
        DrawCircleV(lua, 46.0f, (Color){ 232, 236, 255, 255 });
        DrawCircleV((Vector2){ lua.x - 14, lua.y - 8 }, 9.0f, (Color){ 205, 210, 240, 255 });
        DrawCircleV((Vector2){ lua.x + 12, lua.y + 14 }, 6.0f, (Color){ 210, 215, 242, 255 });
        DrawCircleV((Vector2){ lua.x + 16, lua.y - 16 }, 4.0f, (Color){ 212, 217, 244, 255 });
        Cordilheira(CHAO_Y - 70, 150, 130, 0, (Color){ 40, 38, 74, 255 }, foco * 0.03f);
        Cordilheira(CHAO_Y - 30, 90, 90, 40, (Color){ 28, 28, 56, 255 }, foco * 0.06f);
        /* ruinas / colunas quebradas */
        for (int i = 0; i < 7; i++) {
            float x = 40.0f + i * 135.0f - foco * 0.09f;
            float h = 90.0f + Hash(i + 70) * 90.0f;
            DrawRectangle((int)x, (int)(CHAO_Y - h), 26, (int)h, (Color){ 22, 22, 46, 255 });
            Tri((Vector2){ x, CHAO_Y - h }, (Vector2){ x + 26, CHAO_Y - h }, (Vector2){ x + 8 + Hash(i + 3) * 14, CHAO_Y - h - 16 }, (Color){ 22, 22, 46, 255 });
            DrawRectangle((int)x - 4, CHAO_Y - 10, 34, 10, (Color){ 18, 18, 40, 255 });
            if (i % 2 == 0) DrawRectangle((int)x + 9, (int)(CHAO_Y - h * 0.6f), 8, 14, Fade(ac, 0.25f + 0.15f * sinf(t * 2 + i)));
        }
        Neblina(t, (Color){ 140, 130, 190, 255 }, 0.07f, CHAO_Y - 20);
        DrawRectangleGradientV(0, CHAO_Y, LARGURA_TELA, ALTURA_TELA - CHAO_Y, (Color){ 52, 54, 72, 255 }, (Color){ 22, 22, 34, 255 });
    } else if (tema == BOSS_DEMONIO) {
        DrawRectangleGradientV(0, 0, LARGURA_TELA, CHAO_Y, (Color){ 22, 4, 8, 255 }, (Color){ 168, 52, 18, 255 });
        Estrelas(t, 20, 0.35f, 0.01f, foco);
        Vector2 sol = { 450.0f - foco * 0.02f, 150.0f };
        Brilho(sol, 230.0f, (Color){ 255, 80, 20, 255 }, 1.6f);
        BeginBlendMode(BLEND_ADDITIVE);
        DrawRing(sol, 64.0f, 74.0f + sinf(t * 2.0f) * 2.0f, 0.0f, 360.0f, 48, Fade((Color){ 255, 150, 50, 255 }, 0.55f));
        DrawRing(sol, 74.0f, 92.0f, 0.0f, 360.0f, 48, Fade((Color){ 255, 90, 20, 255 }, 0.2f));
        EndBlendMode();
        DrawCircleV(sol, 64.0f, (Color){ 6, 0, 2, 255 });
        Cordilheira(CHAO_Y - 60, 170, 120, 10, (Color){ 40, 10, 12, 255 }, foco * 0.03f);
        /* brilho de lava nas encostas */
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = 0; i < 9; i++) {
            float x = i * 110.0f - foco * 0.03f + Hash(i) * 40.0f;
            DrawLineEx((Vector2){ x, CHAO_Y - 60 }, (Vector2){ x + 20 + Hash(i + 2) * 20, CHAO_Y - 100 - Hash(i + 4) * 40 }, 2.0f, Fade((Color){ 255, 100, 20, 255 }, 0.3f + 0.2f * sinf(t * 3 + i)));
        }
        EndBlendMode();
        Cordilheira(CHAO_Y - 25, 85, 70, 60, (Color){ 22, 6, 8, 255 }, foco * 0.07f);
        Neblina(t, (Color){ 255, 90, 40, 255 }, 0.05f, CHAO_Y - 10);
        DrawRectangleGradientV(0, CHAO_Y, LARGURA_TELA, ALTURA_TELA - CHAO_Y, (Color){ 58, 28, 26, 255 }, (Color){ 20, 8, 8, 255 });
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = 0; i < 14; i++) {
            float x = Hash(i + 100) * LARGURA_TELA, y = CHAO_Y + 18 + Hash(i + 120) * 70;
            float pulso = 0.25f + 0.2f * sinf(t * 2.5f + i);
            Vector2 a = { x, y }, b = { x + 28, y + 8 }, c = { x + 44, y - 4 }, d = { x + 70, y + 6 };
            DrawLineEx(a, b, 2.0f, Fade((Color){ 255, 110, 20, 255 }, pulso));
            DrawLineEx(b, c, 2.0f, Fade((Color){ 255, 110, 20, 255 }, pulso));
            DrawLineEx(c, d, 2.0f, Fade((Color){ 255, 110, 20, 255 }, pulso));
        }
        EndBlendMode();
    } else {
        DrawRectangleGradientV(0, 0, LARGURA_TELA, CHAO_Y, (Color){ 3, 1, 12, 255 }, (Color){ 46, 12, 84, 255 });
        Estrelas(t, 110, 1.0f, 0.015f, foco);
        BeginBlendMode(BLEND_ADDITIVE);
        for (int k = 0; k < 4; k++) {
            Vector2 c = { 160.0f + k * 220.0f - foco * 0.02f, 120.0f + 40.0f * sinf(k * 2.0f + t * 0.1f) };
            DrawCircleV(c, 120.0f, Fade((k % 2) ? (Color){ 40, 140, 200, 255 } : (Color){ 150, 50, 200, 255 }, 0.05f));
            DrawCircleV(c, 70.0f, Fade((k % 2) ? (Color){ 40, 140, 200, 255 } : (Color){ 150, 50, 200, 255 }, 0.05f));
        }
        EndBlendMode();
        /* planeta com aneis */
        Vector2 pl = { 150.0f - foco * 0.03f, 120.0f };
        DrawCircleV(pl, 52.0f, (Color){ 40, 24, 82, 255 });
        DrawCircleV((Vector2){ pl.x - 12, pl.y - 12 }, 38.0f, (Color){ 58, 36, 112, 255 });
        DrawEllipseLines((int)pl.x, (int)pl.y, 96.0f, 20.0f, Fade((Color){ 190, 140, 255, 255 }, 0.7f));
        DrawEllipseLines((int)pl.x, (int)pl.y, 82.0f, 16.0f, Fade((Color){ 190, 140, 255, 255 }, 0.4f));
        /* ilhas flutuantes */
        for (int i = 0; i < 4; i++) {
            float x = 120.0f + i * 230.0f - foco * 0.05f;
            float y = 230.0f + Hash(i + 30) * 50.0f + sinf(t * 0.8f + i * 1.7f) * 8.0f;
            float w = 60.0f + Hash(i + 33) * 40.0f;
            DrawEllipse((int)x, (int)y, w, 12.0f, (Color){ 30, 18, 58, 255 });
            Tri((Vector2){ x - w, y }, (Vector2){ x + w, y }, (Vector2){ x + Hash(i) * 10.0f, y + 40.0f + Hash(i + 5) * 30.0f }, (Color){ 22, 12, 46, 255 });
            Tri((Vector2){ x - 8, y - 6 }, (Vector2){ x + 8, y - 6 }, (Vector2){ x, y - 30.0f - Hash(i + 9) * 16.0f }, Fade((Color){ 190, 120, 255, 255 }, 0.8f));
        }
        Neblina(t, (Color){ 130, 70, 210, 255 }, 0.06f, CHAO_Y - 25);
        DrawRectangleGradientV(0, CHAO_Y, LARGURA_TELA, ALTURA_TELA - CHAO_Y, (Color){ 30, 18, 62, 255 }, (Color){ 8, 4, 22, 255 });
        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = -10; i <= 10; i++)
            DrawLineEx((Vector2){ 450.0f + i * 34.0f, CHAO_Y }, (Vector2){ 450.0f + i * 150.0f, ALTURA_TELA }, 1.0f, Fade(ac, 0.18f));
        for (int k = 0; k < 6; k++) {
            float u = fmodf(k / 6.0f + t * 0.08f, 1.0f);
            float y = CHAO_Y + 100.0f * u * u;
            DrawLineEx((Vector2){ 0, y }, (Vector2){ LARGURA_TELA, y }, 1.0f, Fade(ac, 0.22f * u + 0.05f));
        }
        EndBlendMode();
    }
    /* chao: linha de brilho e pedras */
    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangleGradientV(0, CHAO_Y - 16, LARGURA_TELA, 16, Fade(ac, 0.0f), Fade(ac, 0.22f));
    EndBlendMode();
    DrawRectangle(0, CHAO_Y, LARGURA_TELA, 4, Misturar(ac, BLACK, 0.2f));
    DrawRectangle(0, CHAO_Y + 4, LARGURA_TELA, 2, Fade(BLACK, 0.4f));
    if (tema != BOSS_VAZIO) {
        for (int r = 0; r < 3; r++) {
            int y = CHAO_Y + 28 + r * 28;
            DrawLine(0, y, LARGURA_TELA, y, Fade(BLACK, 0.3f));
            for (int x = (r % 2) * 45; x < LARGURA_TELA; x += 90) DrawLine(x, y, x, y + 28, Fade(BLACK, 0.3f));
        }
    }
    CirculoRitual(t, ac);
}

static void DesenharAmbiente(int tema, float t) {
    int qtd = 38;
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < qtd; i++) {
        float h1 = Hash(i * 7 + 1), h2 = Hash(i * 7 + 2), h3 = Hash(i * 7 + 3);
        float x, y, r; Color c;
        if (tema == BOSS_DEMONIO) {
            x = fmodf(h1 * LARGURA_TELA + sinf(t + i) * 24.0f + LARGURA_TELA, LARGURA_TELA);
            y = ALTURA_TELA - fmodf(t * (30.0f + h3 * 50.0f) + h2 * ALTURA_TELA, ALTURA_TELA + 20.0f);
            c = (Color){ 255, (unsigned char)(90 + h2 * 120), 30, 255 }; r = 1.5f + h3 * 2.0f;
        } else if (tema == BOSS_VAZIO) {
            x = fmodf(h1 * LARGURA_TELA + t * (6.0f + h3 * 10.0f), LARGURA_TELA);
            y = fmodf(h2 * ALTURA_TELA + t * (12.0f + h3 * 16.0f), ALTURA_TELA);
            c = (i % 2) ? (Color){ 120, 220, 255, 255 } : (Color){ 220, 110, 255, 255 }; r = 1.2f + h3 * 1.8f;
        } else {
            x = fmodf(h1 * LARGURA_TELA + sinf(t * 0.6f + i) * 40.0f + LARGURA_TELA, LARGURA_TELA);
            y = CHAO_Y - 30.0f - h2 * 260.0f + sinf(t * 0.8f + i * 2.0f) * 14.0f;
            c = (Color){ 190, 255, 230, 255 }; r = 1.4f + h3 * 1.6f;
        }
        float tw = 0.4f + 0.6f * (0.5f + 0.5f * sinf(t * 3.0f + i * 1.3f));
        DrawCircleV((Vector2){ x, y }, r * 3.2f, Fade(c, 0.08f * tw));
        DrawCircleV((Vector2){ x, y }, r, Fade(c, 0.8f * tw));
    }
    EndBlendMode();
}

static void DesenharVinheta(float forca) {
    int e = 130;
    Color a = Fade(BLACK, 0.55f * forca), z = Fade(BLACK, 0.0f);
    DrawRectangleGradientV(0, 0, LARGURA_TELA, e, a, z);
    DrawRectangleGradientV(0, ALTURA_TELA - e, LARGURA_TELA, e, z, a);
    DrawRectangleGradientH(0, 0, e, ALTURA_TELA, a, z);
    DrawRectangleGradientH(LARGURA_TELA - e, 0, e, ALTURA_TELA, z, a);
}

/* ------------------------------------------------------------------------- */
/*  HEROI (esqueleto procedural animado)                                     */
/* ------------------------------------------------------------------------- */
static void DesenharHeroi(const Personagem *h, float alpha, bool tint) {
    static const float A0[3] = { 3.6f, -0.4f, 4.1f };
    static const float A1[3] = { 0.25f, 3.0f, -0.7f };
    float t = h->anim;
    float cx = h->rect.x + h->rect.width * 0.5f;
    float pe = h->rect.y + h->rect.height;
    float vx = Clamp(fabsf(h->velocidade.x) / 320.0f, 0.0f, 1.2f);
    bool no_ar = !h->esta_no_chao;
    bool atk = h->esta_atacando && h->duracao_ataque > 0.0f;
    float pa = atk ? Clamp(1.0f - h->tempo_ataque / h->duracao_ataque, 0.0f, 1.0f) : 0.0f;
    bool dash = h->tempo_dash > 0.0f;
    bool ferido = h->tempo_dano > 0.0f;
    int c = h->combo; if (c < 0 || c > 2) c = 0;

    g_alpha = alpha; g_tint_on = tint; g_tint = (Color){ 120, 230, 255, 255 };
    g_branco = ferido ? 0.85f : 0.0f;
    if (!tint && h->invencivel > 0.0f && !ferido && !dash && ((int)(h->invencivel * 22.0f)) % 2 == 0) g_alpha *= 0.4f;

    float sy = 1.0f, sx = 1.0f;
    if (no_ar) { sy = 1.0f + Clamp(-h->velocidade.y / 3000.0f, -0.08f, 0.12f); sx = 1.0f / sy; }
    sy -= h->squash * 0.28f; sx += h->squash * 0.28f;
    if (dash) { sx = 1.22f; sy = 0.88f; }
    Pose p = { cx, pe, (float)h->direcao, sx, sy };

    Color aco = { 86, 140, 204, 255 }, aco_esc = { 48, 88, 148, 255 }, ouro = { 242, 202, 92, 255 };
    Color capa = { 30, 96, 138, 255 }, capa2 = { 14, 36, 84, 255 }, helm = { 156, 180, 210, 255 };
    Color neon = { 120, 232, 255, 255 }, pluma = { 226, 62, 84, 255 }, perna_t = { 40, 74, 128, 255 };

    /* esqueleto */
    float fase = t * 13.0f, vxs = 0.55f + 0.45f * vx;
    float bob = no_ar ? 0.0f : (vx > 0.15f ? fabsf(sinf(fase)) * 3.2f * vx : sinf(t * 2.6f) * 1.0f);
    if (atk) bob = 0.0f;
    float incl = (vx > 0.15f ? 0.10f * vx : 0.0f) + (dash ? 0.35f : 0.0f) + (atk ? sinf(pa * PI) * 0.18f : 0.0f) - (ferido ? 0.3f : 0.0f);
    Vector2 quadril = { incl * 6.0f, -32.0f - bob };
    Vector2 ombro = { incl * 24.0f, -54.0f - bob };
    Vector2 cabeca = { incl * 34.0f + 2.0f, -65.0f - bob };

    float tf, bf, tt, bt;
    if (dash) { tf = 1.0f; bf = 0.7f; tt = -1.1f; bt = 0.2f; }
    else if (ferido) { tf = 0.25f; bf = 0.2f; tt = -0.3f; bt = 0.6f; }
    else if (no_ar) { tf = 0.9f; bf = 1.0f; tt = -0.3f; bt = 1.1f; }
    else if (atk) { tf = 0.55f; bf = 0.6f; tt = -0.5f; bt = 0.2f; }
    else if (vx > 0.15f) {
        float s1 = sinf(fase);
        tf = s1 * 0.95f * vxs; bf = 0.25f + 0.9f * fmaxf(0.0f, cosf(fase));
        tt = -s1 * 0.95f * vxs; bt = 0.25f + 0.9f * fmaxf(0.0f, -cosf(fase));
    } else { tf = 0.12f; bf = 0.05f; tt = -0.14f; bt = 0.0f; }

    float arm_f, esp_ang, arm_t;
    if (atk) { arm_f = Lerp(A0[c], A1[c], Suave(pa)); esp_ang = arm_f + 0.05f; arm_t = -0.9f; }
    else if (dash) { arm_f = -1.0f; esp_ang = -1.3f; arm_t = -1.3f; }
    else if (ferido) { arm_f = -0.6f; esp_ang = 1.8f; arm_t = -0.7f; }
    else if (no_ar) { arm_f = 2.2f; esp_ang = 2.7f; arm_t = 2.4f; }
    else if (vx > 0.15f) { arm_f = -sinf(fase) * 0.8f * vxs + 0.35f; esp_ang = 2.0f; arm_t = sinf(fase) * 0.8f * vxs - 0.1f; }
    else { arm_f = 0.45f + sinf(t * 2.6f) * 0.04f; esp_ang = 2.25f + sinf(t * 2.6f) * 0.05f; arm_t = -0.2f - sinf(t * 2.6f) * 0.04f; }
    float cot = atk ? 0.08f : 0.55f;

    /* capa */
    float vcape = vx * 14.0f + (dash ? 14.0f : 0.0f) + (atk ? 3.0f : 0.0f);
    Vector2 pf = { 0, 0 }, pb = { 0, 0 };
    for (int k = 0; k <= 6; k++) {
        float s = (float)k / 6.0f;
        float onda = sinf(t * 7.0f - s * 4.5f) * (2.0f + 7.0f * s) * (0.35f + vx * 0.7f);
        float px = ombro.x - 3.0f - s * (7.0f + vcape) + onda * 0.4f;
        float py = ombro.y + 2.0f + s * (36.0f + (no_ar ? 8.0f : 0.0f)) + onda * 0.6f;
        Vector2 f = { px + 3.0f + s * 2.0f, py }, bk = { px - 3.0f - s * 4.0f, py };
        if (k > 0) {
            Color cc = Misturar(capa, capa2, s);
            TriL(p, pf, f, pb, cc);
            TriL(p, pb, f, bk, cc);
            Seg(p, pb, bk, 1.5f, Fade(neon, 0.5f));
        }
        pf = f; pb = bk;
    }

    /* braco de tras */
    Vector2 cot_t = Osso(p, ombro, arm_t, 12.0f, 6.0f, aco_esc);
    Vector2 mao_t = Osso(p, cot_t, arm_t + 0.5f, 12.0f, 5.5f, aco_esc);
    Circ(p, mao_t.x, mao_t.y, 3.5f, ouro);

    /* perna de tras */
    Vector2 jt = Osso(p, quadril, tt, 15.0f, 7.5f, perna_t);
    Vector2 pt_ = Osso(p, jt, tt - bt, 16.0f, 6.5f, perna_t);
    Osso(p, pt_, 1.45f, 6.0f, 6.0f, aco_esc);

    /* torso */
    Seg(p, quadril, ombro, 15.0f, aco);
    Seg(p, (Vector2){ ombro.x + 3.0f, ombro.y + 4.0f }, (Vector2){ quadril.x + 3.0f, quadril.y - 4.0f }, 5.0f, Misturar(aco, WHITE, 0.25f));
    Seg(p, (Vector2){ quadril.x - 7.0f, quadril.y - 1.0f }, (Vector2){ quadril.x + 7.0f, quadril.y - 1.0f }, 3.5f, ouro);
    float sai = sinf(t * 5.0f) * 1.5f + vx * 2.0f;
    TriL(p, (Vector2){ quadril.x - 7.0f, quadril.y }, (Vector2){ quadril.x + 7.0f, quadril.y }, (Vector2){ quadril.x - sai, quadril.y + 14.0f }, capa);
    float pulso = 0.6f + 0.4f * sinf(t * 4.0f);
    Circ(p, ombro.x + 4.0f, ombro.y + 11.0f, 2.6f, neon);
    Brilho(L(p, ombro.x + 4.0f, ombro.y + 11.0f), 14.0f, neon, 1.0f * pulso);
    Circ(p, ombro.x, ombro.y, 7.5f, ouro);
    Circ(p, ombro.x, ombro.y, 6.0f, aco_esc);

    /* perna da frente */
    Vector2 jf = Osso(p, quadril, tf, 15.0f, 8.0f, aco_esc);
    Vector2 pf_ = Osso(p, jf, tf - bf, 16.0f, 7.0f, aco_esc);
    Circ(p, jf.x, jf.y, 4.5f, ouro);
    Osso(p, pf_, 1.45f, 7.0f, 6.5f, ouro);

    /* cabeca */
    float sw = sinf(t * 6.0f);
    Seg(p, (Vector2){ cabeca.x - 3.0f, cabeca.y - 8.0f }, (Vector2){ cabeca.x - 10.0f - vcape * 0.6f, cabeca.y - 11.0f + sw * 2.0f }, 4.5f, pluma);
    Seg(p, (Vector2){ cabeca.x - 10.0f - vcape * 0.6f, cabeca.y - 11.0f + sw * 2.0f }, (Vector2){ cabeca.x - 19.0f - vcape * 0.9f, cabeca.y - 5.0f + sw * 4.0f }, 3.0f, pluma);
    Circ(p, cabeca.x, cabeca.y, 9.5f, helm);
    Circ(p, cabeca.x - 2.0f, cabeca.y - 2.0f, 6.0f, Misturar(helm, WHITE, 0.3f));
    Seg(p, (Vector2){ cabeca.x + 1.0f, cabeca.y + 1.0f }, (Vector2){ cabeca.x + 9.5f, cabeca.y + 1.0f }, 4.0f, (Color){ 14, 20, 40, 255 });
    Circ(p, cabeca.x + 6.0f, cabeca.y + 1.0f, 1.9f, neon);
    Brilho(L(p, cabeca.x + 6.0f, cabeca.y + 1.0f), 12.0f, neon, 1.2f);

    /* braco da frente + espada */
    Vector2 cot_f = Osso(p, ombro, arm_f, 12.0f, 6.5f, aco);
    Vector2 mao = Osso(p, cot_f, arm_f + cot, 12.0f, 6.0f, aco);
    Circ(p, mao.x, mao.y, 4.0f, ouro);
    Vector2 dir = { sinf(esp_ang), cosf(esp_ang) };
    Vector2 guarda = { cosf(esp_ang) * 5.5f, -sinf(esp_ang) * 5.5f };
    Seg(p, (Vector2){ mao.x - dir.x * 8.0f, mao.y - dir.y * 8.0f }, mao, 3.5f, (Color){ 90, 56, 40, 255 });
    Seg(p, (Vector2){ mao.x - guarda.x, mao.y - guarda.y }, (Vector2){ mao.x + guarda.x, mao.y + guarda.y }, 3.2f, ouro);
    Vector2 ponta = { mao.x + dir.x * 40.0f, mao.y + dir.y * 40.0f };
    Seg(p, (Vector2){ mao.x + dir.x * 3.0f, mao.y + dir.y * 3.0f }, ponta, 5.5f, (Color){ 214, 228, 242, 255 });
    Seg(p, (Vector2){ mao.x + dir.x * 6.0f, mao.y + dir.y * 6.0f }, ponta, 1.8f, neon);
    Brilho(L(p, mao.x + dir.x * 22.0f, mao.y + dir.y * 22.0f), 30.0f, neon, atk ? 2.0f : 0.6f);

    /* rastro do golpe */
    if (atk && pa > 0.08f && pa < 0.92f) {
        float a_cur = arm_f, a_ini = A0[c], a_tail = Lerp(a_cur, a_ini, 0.6f);
        float alc = (c == 2) ? 14.0f : 0.0f;
        Color cs = (c == 2) ? (Color){ 255, 220, 120, 255 } : neon;
        BeginBlendMode(BLEND_ADDITIVE);
        Arco(p, ombro, 24.0f, 58.0f + alc, a_tail, a_cur, Fade(cs, 0.5f));
        Arco(p, ombro, 44.0f, 62.0f + alc, a_tail, a_cur, Fade(WHITE, 0.55f));
        EndBlendMode();
    }
    g_alpha = 1.0f; g_branco = 0.0f; g_tint_on = false;
}

/* ------------------------------------------------------------------------- */
/*  CHEFE 1 - COLOSSO DE PEDRA                                               */
/* ------------------------------------------------------------------------- */
static void EstilhacosColosso(const Personagem *b, Pose p, bool frente, Color runa, Color cl) {
    float vel = 0.9f + b->raiva * 1.1f;
    for (int k = 0; k < 5; k++) {
        float a = b->anim * vel + k * 1.2566f, s = sinf(a);
        if ((s > 0.0f) != frente) continue;
        float x = cosf(a) * 92.0f, y = -95.0f + s * 26.0f - 10.0f * sinf(b->anim * 2.0f + k);
        float sz = 7.0f + (k % 3) * 2.0f;
        TriL(p, (Vector2){ x - sz, y + sz }, (Vector2){ x + sz, y + sz * 0.6f }, (Vector2){ x + sz * 0.2f, y - sz * 1.2f }, cl);
        Circ(p, x, y, 1.8f, runa);
    }
}

static void BracoColosso(Pose p, Vector2 S, float ang, float dob, Color pedra, Color esc, Color cl, Color runa) {
    Vector2 c = Osso(p, S, ang, 40.0f, 27.0f, pedra);
    Vector2 m = Osso(p, c, ang + dob, 36.0f, 24.0f, esc);
    Circ(p, c.x, c.y, 15.0f, pedra);
    DrawRing(L(p, m.x, m.y), 15.0f, 18.0f, 0.0f, 360.0f, 20, Pt(Fade(runa, 0.9f)));
    Circ(p, m.x, m.y, 19.0f, pedra);
    Circ(p, m.x - 4.0f, m.y - 5.0f, 8.0f, cl);
    Seg(p, (Vector2){ m.x - 6.0f, m.y + 6.0f }, (Vector2){ m.x + 7.0f, m.y + 9.0f }, 2.0f, esc);
}

static void DesenharColosso(const Personagem *b, Pose p, float prep, float atq, float mov) {
    float t = b->anim, raiva = b->raiva;
    Color pedra = { 96, 100, 112, 255 }, esc = { 58, 62, 74, 255 }, cl = { 140, 146, 160, 255 };
    Color runa = Misturar((Color){ 90, 255, 210, 255 }, (Color){ 255, 110, 50, 255 }, raiva);
    float pulso = 0.65f + 0.35f * sinf(t * 3.0f);
    float resp = sinf(t * 1.8f) * 1.5f;
    bool pis_prep = (b->acao == B_PREPARO && b->ataque == AT_PISAO);
    float hy = pis_prep ? Suave(prep) * 14.0f : 0.0f;
    bool golpe = (b->ataque == AT_GOLPE);

    float af = 0.12f + sinf(t * 1.8f) * 0.04f + sinf(t * 5.0f) * 0.28f * mov;
    float ab = 0.10f - sinf(t * 5.0f) * 0.28f * mov;
    if (golpe && b->acao == B_PREPARO) af = Lerp(0.12f, 3.0f, Suave(prep));
    else if (golpe && b->acao == B_ATAQUE) af = Lerp(3.0f, 0.15f, Suave(atq * 1.6f));
    if (b->ataque == AT_PISAO) {
        if (b->acao == B_PREPARO) af = ab = Lerp(0.12f, 2.9f, Suave(prep));
        else if (b->acao == B_ATAQUE) af = ab = 2.9f;
        else if (b->acao == B_RECUPERA) af = ab = 0.2f;
    }
    float dob_f = (af > 1.5f) ? -0.05f : 0.35f, dob_b = (ab > 1.5f) ? -0.05f : 0.35f;

    EstilhacosColosso(b, p, false, runa, cl);
    BracoColosso(p, (Vector2){ -54.0f, -104.0f + hy * 0.6f + resp }, ab, dob_b, Misturar(pedra, BLACK, 0.25f), Misturar(esc, BLACK, 0.2f), esc, runa);

    /* pernas */
    for (int s = 0; s < 2; s++) {
        float x0 = (s ? 24.0f : -24.0f);
        float lift = mov > 0.1f ? fmaxf(0.0f, sinf(t * 5.0f + (s ? 0.0f : PI))) * 7.0f * mov : 0.0f;
        Color cp = s ? pedra : Misturar(pedra, BLACK, 0.2f);
        Ret(p, x0 - 16.0f, -54.0f + hy, x0 + 16.0f, -lift - 8.0f, 0.2f, cp);
        Ret(p, x0 - 20.0f, -lift - 11.0f, x0 + 20.0f, -lift, 0.3f, esc);
        Ret(p, x0 - 12.0f, -50.0f + hy, x0 - 4.0f, -lift - 14.0f, 0.0f, Misturar(cp, WHITE, 0.12f));
    }

    /* torso */
    float ty = -108.0f + hy * 0.6f + resp;
    Vector2 a = { -36.0f, -50.0f + hy }, bb = { 36.0f, -50.0f + hy }, c = { 56.0f, ty }, d = { -56.0f, ty };
    TriL(p, a, bb, c, pedra); TriL(p, a, c, d, pedra);
    Vector2 a2 = { -26.0f, -58.0f + hy }, b2 = { 26.0f, -58.0f + hy }, c2 = { 38.0f, ty + 12.0f }, d2 = { -38.0f, ty + 12.0f };
    TriL(p, a2, b2, c2, esc); TriL(p, a2, c2, d2, esc);
    Seg(p, d, c, 5.0f, cl);
    Ret(p, -22.0f, ty - 5.0f, -2.0f, ty + 2.0f, 0.5f, (Color){ 74, 120, 74, 255 });
    Vector2 nucleo = { 0.0f, -80.0f + hy * 0.6f };
    Brilho(L(p, nucleo.x, nucleo.y), 48.0f, runa, 1.5f * pulso + raiva * 0.8f);
    DrawRing(L(p, nucleo.x, nucleo.y), 10.0f, 13.0f, 0.0f, 360.0f, 24, Pt(runa));
    Circ(p, nucleo.x, nucleo.y, 8.0f, Misturar(runa, WHITE, 0.5f));
    Seg(p, nucleo, (Vector2){ -14.0f, nucleo.y + 16.0f }, 2.2f, Fade(runa, 0.6f + 0.4f * pulso));
    Seg(p, (Vector2){ -14.0f, nucleo.y + 16.0f }, (Vector2){ -24.0f, nucleo.y + 26.0f }, 2.2f, Fade(runa, 0.6f + 0.4f * pulso));
    Seg(p, nucleo, (Vector2){ 16.0f, nucleo.y - 12.0f }, 2.2f, Fade(runa, 0.6f + 0.4f * pulso));
    Seg(p, (Vector2){ 16.0f, nucleo.y - 12.0f }, (Vector2){ 27.0f, nucleo.y - 20.0f }, 2.2f, Fade(runa, 0.6f + 0.4f * pulso));
    Seg(p, nucleo, (Vector2){ -9.0f, nucleo.y - 16.0f }, 2.2f, Fade(runa, 0.5f + 0.4f * pulso));

    /* cabeca */
    float hdy = ty - 14.0f;
    Ret(p, -9.0f, hdy - 13.0f, 25.0f, hdy + 13.0f, 0.35f, pedra);
    Ret(p, -9.0f, hdy - 13.0f, 25.0f, hdy - 5.0f, 0.3f, esc);
    Ret(p, 3.0f, hdy + 6.0f, 23.0f, hdy + 15.0f, 0.3f, esc);
    float ol = 1.0f + prep * 1.6f + raiva * 0.8f;
    Ret(p, 8.0f, hdy - 3.0f, 22.0f, hdy + 1.5f, 0.0f, runa);
    Brilho(L(p, 15.0f, hdy - 1.0f), 30.0f, runa, 1.6f * ol);
    Seg(p, (Vector2){ 5.0f, hdy + 8.0f }, (Vector2){ 20.0f, hdy + 8.0f }, 1.5f, Fade(runa, 0.7f));

    /* ombros */
    for (int s = 0; s < 2; s++) {
        float x = s ? 54.0f : -54.0f;
        Circ(p, x, ty + 4.0f, 22.0f, s ? pedra : Misturar(pedra, BLACK, 0.2f));
        Circ(p, x - 5.0f, ty - 1.0f, 12.0f, cl);
        DrawEllipse((int)L(p, x, ty - 12.0f).x, (int)L(p, x, ty - 12.0f).y, 12.0f, 5.0f, Pt((Color){ 66, 120, 70, 255 }));
    }
    EstilhacosColosso(b, p, true, runa, cl);
    BracoColosso(p, (Vector2){ 54.0f, -104.0f + hy * 0.6f + resp }, af, dob_f, pedra, esc, cl, runa);
}

/* ------------------------------------------------------------------------- */
/*  CHEFE 2 - SENHOR DAS BRASAS                                              */
/* ------------------------------------------------------------------------- */
static void DesenharDemonio(const Personagem *b, Pose p, float prep, float atq, float mov) {
    float t = b->anim, raiva = b->raiva;
    Color pele = { 178, 38, 34, 255 }, esc = { 104, 18, 26, 255 }, cl = { 228, 80, 54, 255 };
    Color chifre = { 238, 226, 196, 255 }, asa = { 88, 12, 30, 255 }, memb = { 150, 26, 44, 255 };
    Color f1 = { 255, 214, 74, 255 }, f2 = { 255, 92, 22, 255 };
    bool invest = (b->ataque == AT_INVESTIDA && b->acao == B_ATAQUE);
    bool invest_prep = (b->ataque == AT_INVESTIDA && b->acao == B_PREPARO);
    float inc = invest ? 16.0f : (invest_prep ? -6.0f * Suave(prep) : 0.0f);
    float resp = sinf(t * 2.2f) * 1.5f;
    bool agindo = (b->acao == B_PREPARO || b->acao == B_ATAQUE);

    /* asas */
    for (int w = 0; w < 2; w++) {
        Vector2 S = { -6.0f + w * 6.0f + inc * 0.3f, -98.0f + resp };
        float fl = sinf(t * 3.2f - w * 0.5f) * 0.32f + (agindo ? 0.18f : 0.0f);
        Color cm = w == 0 ? Misturar(memb, BLACK, 0.45f) : memb;
        Color ca = w == 0 ? Misturar(asa, BLACK, 0.4f) : asa;
        Vector2 T1 = { S.x + sinf(3.4f + fl) * 74.0f, S.y + cosf(3.4f + fl) * 74.0f };
        Vector2 T2 = { S.x + sinf(3.95f + fl * 0.8f) * 84.0f, S.y + cosf(3.95f + fl * 0.8f) * 84.0f };
        Vector2 T3 = { S.x + sinf(4.5f + fl * 0.6f) * 66.0f, S.y + cosf(4.5f + fl * 0.6f) * 66.0f };
        Vector2 base = { S.x - 8.0f, S.y + 46.0f };
        TriL(p, S, T1, T2, cm); TriL(p, S, T2, T3, cm); TriL(p, S, T3, base, cm);
        Seg(p, S, T1, 4.0f, ca); Seg(p, S, T2, 4.0f, ca); Seg(p, S, T3, 3.5f, ca);
    }

    /* cauda */
    Vector2 pa = { -26.0f, -54.0f };
    for (int k = 1; k <= 7; k++) {
        float amp = (4.0f + k * 2.0f) * (b->acao == B_ATAQUE ? 1.8f : 1.0f);
        Vector2 pb = { -26.0f - k * 10.0f - (invest ? k * 3.0f : 0.0f), -54.0f + k * 3.0f + sinf(t * 4.0f - k * 0.8f) * amp };
        Seg(p, pa, pb, 11.0f - k * 1.2f, esc);
        pa = pb;
    }
    TriL(p, (Vector2){ pa.x + 2.0f, pa.y - 8.0f }, (Vector2){ pa.x + 2.0f, pa.y + 8.0f }, (Vector2){ pa.x - 17.0f, pa.y }, f2);

    /* pernas digitigradas */
    float ph = t * 6.0f, wal = mov > 0.1f ? 0.45f : 0.0f;
    for (int s = 0; s < 2; s++) {
        float sw = sinf(ph + s * PI) * wal;
        Vector2 q = { -6.0f + s * 14.0f, -54.0f };
        Color cp = s ? pele : esc;
        Vector2 jo = Osso(p, q, 0.6f + sw, 28.0f, 17.0f, cp);
        Vector2 tor = Osso(p, jo, -0.45f + sw * 0.5f, 32.0f, 12.0f, cp);
        Osso(p, tor, 1.5f, 13.0f, 8.0f, esc);
        TriL(p, (Vector2){ tor.x + 11.0f, tor.y - 2.0f }, (Vector2){ tor.x + 11.0f, tor.y + 4.0f }, (Vector2){ tor.x + 19.0f, tor.y + 2.0f }, chifre);
    }

    /* tronco */
    float topo = -100.0f + resp;
    Vector2 h1 = { -14.0f, -54.0f }, h2 = { 18.0f, -54.0f }, s1 = { 34.0f + inc, topo }, s2 = { -22.0f + inc, topo - 2.0f };
    TriL(p, h1, h2, s1, pele); TriL(p, h1, s1, s2, pele);
    Vector2 v1 = { -6.0f, -56.0f }, v2 = { 12.0f, -56.0f }, v3 = { 18.0f + inc * 0.7f, -90.0f }, v4 = { -4.0f + inc * 0.7f, -90.0f };
    TriL(p, v1, v2, v3, cl); TriL(p, v1, v3, v4, cl);
    for (int k = 0; k < 3; k++) Seg(p, (Vector2){ 0.0f + inc * 0.2f, -66.0f - k * 8.0f }, (Vector2){ 12.0f + inc * 0.3f, -66.0f - k * 8.0f }, 1.5f, esc);
    Brilho(L(p, 8.0f + inc * 0.5f, -82.0f), 34.0f, f2, 1.0f + raiva * 1.4f);
    Circ(p, 8.0f + inc * 0.5f, -82.0f, 3.5f, f1);
    TriL(p, (Vector2){ -16.0f, -52.0f }, (Vector2){ 20.0f, -52.0f }, (Vector2){ 14.0f, -34.0f }, (Color){ 24, 8, 12, 255 });
    TriL(p, (Vector2){ -16.0f, -52.0f }, (Vector2){ 14.0f, -34.0f }, (Vector2){ -10.0f, -36.0f }, (Color){ 24, 8, 12, 255 });
    Circ(p, 2.0f, -46.0f, 4.0f, (Color){ 240, 200, 90, 255 });

    /* bracos */
    float af = 0.35f + sinf(t * 2.0f) * 0.06f, ab = 0.2f + sinf(t * 2.0f + 1.0f) * 0.06f, dob = 0.5f;
    bool golpe = (b->ataque == AT_GOLPE), fogo = (b->ataque == AT_FOGO);
    if (golpe && b->acao == B_PREPARO) af = Lerp(0.35f, 3.6f, Suave(prep));
    else if (golpe && b->acao == B_ATAQUE) af = Lerp(3.6f, 0.4f, Suave(atq * 1.5f));
    if (fogo && b->acao == B_PREPARO) { af = Lerp(0.35f, 2.3f, Suave(prep)); dob = -0.2f; }
    else if (fogo && b->acao == B_ATAQUE) { af = 1.6f; dob = 0.0f; }
    if (invest || invest_prep) { af = ab = -1.1f; dob = -0.3f; }

    for (int s = 0; s < 2; s++) {
        Vector2 S = s ? (Vector2){ 26.0f + inc, topo + 4.0f } : (Vector2){ -16.0f + inc, topo + 4.0f };
        float ang = s ? af : ab;
        Color ca = s ? pele : esc;
        Vector2 c1 = Osso(p, S, ang, 26.0f, 15.0f, ca);
        Vector2 m = Osso(p, c1, ang + dob, 26.0f, 13.0f, ca);
        Circ(p, m.x, m.y, 7.5f, ca);
        for (int k = -1; k <= 1; k++) {
            float ga = ang + dob + k * 0.4f;
            Seg(p, m, (Vector2){ m.x + sinf(ga) * 12.0f, m.y + cosf(ga) * 12.0f }, 2.6f, chifre);
        }
        if (s && fogo && b->acao == B_PREPARO) {
            Vector2 hp = L(p, m.x, m.y);
            float r = 6.0f + Suave(prep) * 24.0f;
            Brilho(hp, r * 2.8f, f2, 2.0f);
            DrawCircleV(hp, r, Pt(f1));
            DrawCircleV(hp, r * 0.55f, Pt(WHITE));
        }
    }

    /* cabeca */
    Vector2 cab = { 22.0f + inc, -118.0f + resp };
    float jaw = agindo ? 7.0f * (b->acao == B_PREPARO ? Suave(prep) : 1.0f) : 0.0f;
    Seg(p, (Vector2){ cab.x - 4.0f, cab.y - 11.0f }, (Vector2){ cab.x - 12.0f, cab.y - 26.0f }, 7.0f, chifre);
    Seg(p, (Vector2){ cab.x - 12.0f, cab.y - 26.0f }, (Vector2){ cab.x - 6.0f, cab.y - 38.0f }, 5.0f, chifre);
    Seg(p, (Vector2){ cab.x - 6.0f, cab.y - 38.0f }, (Vector2){ cab.x + 5.0f, cab.y - 43.0f }, 3.0f, chifre);
    Brilho(L(p, cab.x - 4.0f, cab.y - 4.0f), 52.0f, f2, 1.2f + raiva * 1.4f);
    BeginBlendMode(BLEND_ADDITIVE);
    for (int k = 0; k < 7; k++) {
        float bx = cab.x - 10.0f - k * 5.0f, by = cab.y - 12.0f + k * 4.0f;
        Vector2 tip = { bx - 10.0f - sinf(t * 9.0f + k) * 6.0f, by - 22.0f - Hash(k) * 10.0f + sinf(t * 11.0f + k * 2.0f) * 5.0f };
        Tri(L(p, bx - 5.0f, by), L(p, bx + 5.0f, by + 2.0f), L(p, tip.x, tip.y), Fade(f2, 0.8f * g_alpha));
        Tri(L(p, bx - 2.5f, by), L(p, bx + 2.5f, by + 1.0f), L(p, (bx + tip.x) * 0.5f, (by + tip.y) * 0.5f), Fade(f1, 0.8f * g_alpha));
    }
    EndBlendMode();
    Circ(p, cab.x, cab.y, 14.0f, pele);
    Circ(p, cab.x - 3.0f, cab.y - 4.0f, 8.0f, cl);
    TriL(p, (Vector2){ cab.x + 6.0f, cab.y - 2.0f }, (Vector2){ cab.x + 26.0f, cab.y + 1.0f }, (Vector2){ cab.x + 8.0f, cab.y + 8.0f }, pele);
    TriL(p, (Vector2){ cab.x + 4.0f, cab.y + 4.0f + jaw * 0.3f }, (Vector2){ cab.x + 22.0f, cab.y + 5.0f + jaw }, (Vector2){ cab.x + 12.0f, cab.y + 14.0f + jaw }, esc);
    for (int k = 0; k < 4; k++) TriL(p, (Vector2){ cab.x + 9.0f + k * 4.5f, cab.y + 5.0f }, (Vector2){ cab.x + 12.0f + k * 4.5f, cab.y + 5.0f }, (Vector2){ cab.x + 10.5f + k * 4.5f, cab.y + 10.0f + jaw * 0.5f }, WHITE);
    TriL(p, (Vector2){ cab.x + 4.0f, cab.y - 8.0f }, (Vector2){ cab.x + 14.0f, cab.y - 4.0f }, (Vector2){ cab.x + 5.0f, cab.y - 1.0f }, f1);
    Brilho(L(p, cab.x + 8.0f, cab.y - 5.0f), 24.0f, f1, 1.6f);
    Seg(p, (Vector2){ cab.x + 2.0f, cab.y - 12.0f }, (Vector2){ cab.x + 14.0f, cab.y - 8.0f }, 2.5f, esc);
}

/* ------------------------------------------------------------------------- */
/*  CHEFE 3 - LORDE DO VAZIO                                                 */
/* ------------------------------------------------------------------------- */
static void MaoVazio(Pose p, Vector2 m, Color pele, Color brilho) {
    Circ(p, m.x, m.y, 5.5f, pele);
    for (int k = 0; k < 4; k++)
        Seg(p, m, (Vector2){ m.x + (k - 1.5f) * 3.6f, m.y - 10.0f - (k == 1 || k == 2 ? 2.0f : 0.0f) }, 1.6f, pele);
    Brilho(L(p, m.x, m.y), 16.0f, brilho, 0.8f);
}

static void DesenharVazio(const Personagem *b, Pose p, float prep, float atq) {
    float t = b->anim, raiva = b->raiva;
    Color manto = { 36, 16, 66, 255 }, manto_cl = { 96, 46, 156, 255 }, vazio = { 6, 2, 14, 255 };
    Color cristal = { 200, 132, 255, 255 }, olho = { 255, 70, 210, 255 }, ciano = { 120, 240, 255, 255 };
    Color roxo = { 170, 80, 255, 255 }, osso = { 214, 204, 236, 255 };
    float flut = sinf(t * 2.0f) * 3.0f;
    int n_orb = b->enfurecido ? 5 : 3;
    bool golpe = (b->ataque == AT_GOLPE), orbes = (b->ataque == AT_ORBES), tele = (b->ataque == AT_TELEPORTE);

    Brilho(L(p, 0.0f, -72.0f), 130.0f, roxo, 1.0f + raiva * 0.9f);
    Brilho(L(p, 0.0f, -100.0f + flut), 66.0f, ciano, 0.5f);

    /* halo giratorio */
    Vector2 hc = { 0.0f, -100.0f + flut };
    BeginBlendMode(BLEND_ADDITIVE);
    for (int k = 0; k < 6; k++) {
        float a0 = t * 0.8f + k * 1.0472f;
        Arco(p, hc, 38.0f, 41.0f, a0, a0 + 0.75f, Fade(ciano, 0.7f));
    }
    for (int k = 0; k < 3; k++) {
        float a0 = -t * 1.3f + k * 2.0944f;
        Arco(p, hc, 30.0f, 31.5f, a0, a0 + 1.2f, Fade(olho, 0.6f));
    }
    EndBlendMode();

    /* orbes (parte de tras) */
    for (int k = 0; k < n_orb; k++) {
        float a = t * 1.6f + k * 6.2832f / n_orb, s = sinf(a);
        if (s > 0.0f) continue;
        Vector2 o = { cosf(a) * 64.0f, -72.0f + s * 20.0f };
        Brilho(L(p, o.x, o.y), 18.0f, roxo, 1.2f);
        Circ(p, o.x, o.y, 7.0f, vazio);
        DrawRing(L(p, o.x, o.y), 6.0f, 8.0f, 0.0f, 360.0f, 14, Pt(Fade(cristal, 0.8f)));
    }

    /* manto rasgado */
    Vector2 o1 = { -26.0f, -108.0f + flut }, o2 = { 26.0f, -108.0f + flut }, o3 = { 38.0f, -40.0f }, o4 = { -38.0f, -40.0f };
    TriL(p, o1, o2, o3, manto); TriL(p, o1, o3, o4, manto);
    for (int k = 0; k < 7; k++) {
        float x = -40.0f + k * 13.3f;
        float tipx = x + sinf(t * 2.0f + k) * 5.0f;
        float tipy = -8.0f - Hash(k + 30) * 14.0f + sinf(t * 3.0f + k * 1.3f) * 5.0f;
        TriL(p, (Vector2){ x - 7.0f, -42.0f }, (Vector2){ x + 7.0f, -42.0f }, (Vector2){ tipx, tipy }, (k % 2) ? manto : Misturar(manto, manto_cl, 0.45f));
    }
    Seg(p, o1, o4, 2.0f, Fade(manto_cl, 0.9f));
    Seg(p, o2, o3, 2.0f, Fade(manto_cl, 0.9f));
    for (int i = 0; i < 12; i++) {
        float x = -24.0f + Hash(i) * 48.0f, y = -100.0f + Hash(i + 9) * 66.0f;
        float tw = 0.4f + 0.6f * (0.5f + 0.5f * sinf(t * 3.0f + i * 1.7f));
        DrawCircleV(L(p, x, y), 1.3f, Pt(Fade(WHITE, tw)));
    }

    /* maos flutuantes e golpe de foice */
    Vector2 hl = { -44.0f, -70.0f + sinf(t * 2.2f) * 5.0f }, hr = { 46.0f, -72.0f + sinf(t * 2.6f + 1.0f) * 5.0f };
    if (golpe && b->acao == B_PREPARO) { hr.x = Lerp(46.0f, 38.0f, Suave(prep)); hr.y = Lerp(-72.0f, -128.0f, Suave(prep)); }
    if (orbes && b->acao == B_PREPARO) {
        hl.x = Lerp(-44.0f, -40.0f, Suave(prep)); hl.y = Lerp(-70.0f, -118.0f, Suave(prep));
        hr.x = Lerp(46.0f, 40.0f, Suave(prep)); hr.y = Lerp(-72.0f, -118.0f, Suave(prep));
    }
    if (tele && b->acao == B_PREPARO) { hl.x = Lerp(-44.0f, -8.0f, Suave(prep)); hr.x = Lerp(46.0f, 8.0f, Suave(prep)); hl.y = hr.y = -84.0f; }
    if (golpe && b->acao == B_ATAQUE) {
        float a = Lerp(3.4f, 0.3f, Suave(atq * 1.4f));
        hr = (Vector2){ sinf(a) * 70.0f, -100.0f + cosf(a) * 70.0f };
        float a_tail = Lerp(a, 3.4f, 0.55f);
        BeginBlendMode(BLEND_ADDITIVE);
        Arco(p, (Vector2){ 0.0f, -100.0f }, 52.0f, 100.0f, a_tail, a, Fade(olho, 0.55f));
        Arco(p, (Vector2){ 0.0f, -100.0f }, 90.0f, 102.0f, a_tail, a, Fade(WHITE, 0.6f));
        EndBlendMode();
    }
    MaoVazio(p, hl, osso, roxo);
    MaoVazio(p, hr, osso, roxo);
    if (orbes && b->acao == B_PREPARO) {
        float r = 8.0f + Suave(prep) * 22.0f;
        Brilho(L(p, hl.x, hl.y - 14.0f), r * 2.5f, olho, 1.8f);
        Brilho(L(p, hr.x, hr.y - 14.0f), r * 2.5f, olho, 1.8f);
        Circ(p, hl.x, hl.y - 14.0f, r * 0.5f, vazio);
        Circ(p, hr.x, hr.y - 14.0f, r * 0.5f, vazio);
    }

    /* capuz e rosto */
    TriL(p, (Vector2){ -19.0f, -106.0f + flut }, (Vector2){ 19.0f, -106.0f + flut }, (Vector2){ 2.0f, -146.0f + flut }, manto);
    Circ(p, 0.0f, -118.0f + flut, 19.0f, manto);
    Circ(p, 3.0f, -117.0f + flut, 13.0f, vazio);
    float pe = 0.7f + 0.3f * sinf(t * 5.0f);
    float ol = 1.0f + prep * 1.5f + raiva * 0.6f;
    Circ(p, 6.0f, -121.0f + flut, 3.0f, olho);
    Circ(p, 9.0f, -113.0f + flut, 2.3f, ciano);
    Circ(p, 1.0f, -112.0f + flut, 2.3f, ciano);
    Brilho(L(p, 6.0f, -121.0f + flut), 26.0f, olho, 1.8f * pe * ol);
    Brilho(L(p, 5.0f, -112.0f + flut), 16.0f, ciano, 1.0f * pe);

    /* coroa de cristais */
    for (int k = 0; k < 5; k++) {
        float bx = -12.0f + k * 6.0f, by = -134.0f + flut + sinf(t * 2.0f + k) * 2.0f;
        float h = 14.0f + (2 - abs(k - 2)) * 6.0f;
        TriL(p, (Vector2){ bx - 3.0f, by }, (Vector2){ bx + 3.0f, by }, (Vector2){ bx + (k - 2) * 3.0f, by - h }, cristal);
        TriL(p, (Vector2){ bx, by }, (Vector2){ bx + 3.0f, by }, (Vector2){ bx + (k - 2) * 3.0f, by - h }, Misturar(cristal, WHITE, 0.5f));
    }
    Brilho(L(p, 0.0f, -148.0f + flut), 40.0f, cristal, 0.9f);

    /* orbes (parte da frente) */
    for (int k = 0; k < n_orb; k++) {
        float a = t * 1.6f + k * 6.2832f / n_orb, s = sinf(a);
        if (s <= 0.0f) continue;
        Vector2 o = { cosf(a) * 64.0f, -72.0f + s * 20.0f };
        Brilho(L(p, o.x, o.y), 20.0f, roxo, 1.4f);
        Circ(p, o.x, o.y, 7.5f, vazio);
        DrawRing(L(p, o.x, o.y), 6.0f, 8.5f, 0.0f, 360.0f, 14, Pt(Fade(cristal, 0.9f)));
    }
}

/* ------------------------------------------------------------------------- */
/*  CHEFE: despacho, areas de golpe e telegrafos                             */
/* ------------------------------------------------------------------------- */
static Rectangle AreaGolpe(const Personagem *b) {
    float alc = 90.0f;
    float x0 = (b->direcao == 1) ? b->rect.x + b->rect.width * 0.35f : b->rect.x - alc;
    float y = b->rect.y + 10.0f;
    return (Rectangle){ x0, y, alc + b->rect.width * 0.65f, CHAO_Y - y };
}

static void DesenharBossPose(const Personagem *b, float ox) {
    float cx = b->rect.x + b->rect.width * 0.5f + ox, pe = b->rect.y + b->rect.height;
    float prep = (b->acao == B_PREPARO && b->tempo_total > 0.0f) ? Clamp(1.0f - b->tempo_acao / b->tempo_total, 0.0f, 1.0f) : 0.0f;
    float atq = (b->acao == B_ATAQUE && b->tempo_total > 0.0f) ? Clamp(1.0f - b->tempo_acao / b->tempo_total, 0.0f, 1.0f) : 0.0f;
    float mov = Clamp(fabsf(b->velocidade.x) / 150.0f, 0.0f, 1.4f);
    Pose p = { cx, pe, (float)b->direcao, 1.0f + b->squash * 0.22f, 1.0f - b->squash * 0.2f };
    if (b->tipo == BOSS_COLOSSO) DesenharColosso(b, p, prep, atq, mov);
    else if (b->tipo == BOSS_DEMONIO) DesenharDemonio(b, p, prep, atq, mov);
    else DesenharVazio(b, p, prep, atq);
}

static void DesenharBoss(const Personagem *b, float ox) {
    if (b->alpha <= 0.01f) return;
    g_tint_on = false;
    if (b->tipo == BOSS_VAZIO && b->alpha < 0.99f) {
        g_alpha = b->alpha * 0.55f; g_branco = 0.0f; g_tint_on = true;
        g_tint = (Color){ 120, 240, 255, 255 };
        DesenharBossPose(b, ox - 7.0f);
        g_tint = (Color){ 255, 70, 210, 255 };
        DesenharBossPose(b, ox + 7.0f);
        g_tint_on = false;
    }
    g_alpha = b->alpha;
    g_branco = b->tempo_dano > 0.0f ? 0.6f : 0.0f;
    DesenharBossPose(b, ox);
    g_alpha = 1.0f; g_branco = 0.0f; g_tint_on = false;
}

static void DesenharTelegrafo(const Personagem *b) {
    if (b->acao != B_PREPARO || b->tempo_total <= 0.0f) return;
    float prep = Clamp(1.0f - b->tempo_acao / b->tempo_total, 0.0f, 1.0f);
    Color ac = { 255, 60, 40, 255 };
    float cb = b->rect.x + b->rect.width / 2;
    BeginBlendMode(BLEND_ADDITIVE);
    if (b->ataque == AT_GOLPE) {
        Rectangle r = AreaGolpe(b);
        DrawRectangleGradientV((int)r.x, CHAO_Y - 150, (int)r.width, 150, Fade(ac, 0.0f), Fade(ac, 0.05f + 0.22f * prep));
        DrawRectangle((int)r.x, CHAO_Y - 4, (int)r.width, 8, Fade(ac, 0.25f + 0.6f * prep));
    } else if (b->ataque == AT_INVESTIDA) {
        for (int k = 0; k < 6; k++) {
            float x = cb + b->direcao * (70.0f + k * 55.0f), s = 16.0f;
            float a = Clamp(prep * 1.4f - k * 0.08f, 0.0f, 1.0f) * 0.7f;
            DrawLineEx((Vector2){ x - b->direcao * s, CHAO_Y - 30.0f }, (Vector2){ x, CHAO_Y - 15.0f }, 3.0f, Fade(ac, a));
            DrawLineEx((Vector2){ x - b->direcao * s, CHAO_Y }, (Vector2){ x, CHAO_Y - 15.0f }, 3.0f, Fade(ac, a));
        }
    } else if (b->ataque == AT_PISAO) {
        float r = 70.0f * (1.5f - 0.5f * prep);
        DrawEllipse((int)b->alvo_x, CHAO_Y + 4, r, r * 0.2f, Fade(ac, 0.08f + 0.2f * prep));
        DrawEllipseLines((int)b->alvo_x, CHAO_Y + 4, r, r * 0.2f, Fade(ac, 0.3f + 0.6f * prep));
        DrawEllipseLines((int)b->alvo_x, CHAO_Y + 4, 70.0f, 14.0f, Fade(ac, 0.25f));
    }
    EndBlendMode();
}

/* ------------------------------------------------------------------------- */
/*  COMBATE: HEROI                                                           */
/* ------------------------------------------------------------------------- */
static void MatarBoss(void) {
    chefao.vida = 0;
    vitoria = true;
    timer_fim = 2.6f;
    hit_stop = 0.22f;
    tremor_tela = 0.7f;
    flash_tela = 0.5f;
    flash_cor = WHITE;
    chefao.acao = B_RECUPERA;
    chefao.tempo_acao = chefao.tempo_total = 99.0f;
    chefao.velocidade.x = 0.0f;
    for (int i = 0; i < MAX_PROJETEIS; i++) if (projeteis[i].ativa) ExplodirProjetil(&projeteis[i]);
    SetSoundPitch(som_rugido, 0.7f);
    PlaySound(som_rugido);
    tempo_explosao = 0.0f;
}

static void AcertarBoss(void) {
    Personagem *j = &jogador;
    int c = j->combo;
    int dano = (c == 2) ? 30 : 18;
    chefao.vida -= dano;
    chefao.tempo_dano = 0.12f;
    j->acertou = true;
    combo_hits++;
    tempo_combo_visual = 2.0f;
    hit_stop = (c == 2) ? 0.10f : 0.055f;
    tremor_tela = (c == 2) ? 0.25f : 0.12f;
    zoom_punch = 1.0f;
    Color cor = (chefao.tipo == BOSS_COLOSSO) ? (Color){ 180, 255, 230, 255 }
              : (chefao.tipo == BOSS_DEMONIO) ? (Color){ 255, 170, 60, 255 } : (Color){ 240, 140, 255, 255 };
    Vector2 imp = { (j->direcao == 1) ? chefao.rect.x + 12 : chefao.rect.x + chefao.rect.width - 12, j->rect.y + 30 };
    ImpactoVisual(imp, cor, c == 2 ? 24 : 12);
    SetSoundPitch(som_dano, 0.85f + 0.12f * c);
    PlaySound(som_dano);
    char t[16]; snprintf(t, sizeof(t), "%d", dano);
    NovoTexto((Vector2){ chefao.rect.x + chefao.rect.width / 2 + Rnd(-20, 20), chefao.rect.y - 8 }, t,
              c == 2 ? GOLD : WHITE, c == 2 ? 30 : 22);
    chefao.rect.x = Clamp(chefao.rect.x + j->direcao * (c == 2 ? 10.0f : 4.0f), 5.0f, LARGURA_TELA - chefao.rect.width - 5.0f);
    if (chefao.vida <= 0) MatarBoss();
}

static void IniciarAtaqueJogador(void) {
    Personagem *j = &jogador;
    j->combo = (j->tempo_combo > 0.0f) ? (j->combo + 1) % 3 : 0;
    j->duracao_ataque = (j->combo == 2) ? 0.36f : 0.24f;
    j->tempo_ataque = j->duracao_ataque;
    j->esta_atacando = true;
    j->acertou = false;
    j->buffer_ataque = false;
    j->empurrao += j->direcao * (j->combo == 2 ? 260.0f : 150.0f);
    SetSoundPitch(som_ataque, 0.9f + 0.18f * j->combo);
    PlaySound(som_ataque);
}

static void AtualizarJogador(float dt, bool controle) {
    Personagem *j = &jogador;
    j->anim += dt;
    if (j->tempo_dano > 0.0f) j->tempo_dano -= dt;
    if (j->invencivel > 0.0f) j->invencivel -= dt;
    if (j->recarga_dash > 0.0f) j->recarga_dash -= dt;
    if (j->tempo_combo > 0.0f) j->tempo_combo -= dt;
    if (j->squash > 0.0f) j->squash = fmaxf(0.0f, j->squash - dt * 3.5f);
    bool estava_no_chao = j->esta_no_chao;

    float ax = 0.0f;
    if (controle) {
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) ax -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) ax += 1.0f;
        if (IsKeyPressed(KEY_Z)) { mira_travada = !mira_travada; PlaySound(som_clique); }
    }

    /* esquiva */
    if (controle && (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_L)) && j->recarga_dash <= 0.0f && j->tempo_dash <= 0.0f) {
        j->dir_dash = (ax != 0.0f) ? (int)ax : j->direcao;
        j->tempo_dash = 0.18f;
        j->recarga_dash = 0.75f;
        j->invencivel = fmaxf(j->invencivel, 0.3f);
        j->esta_atacando = false;
        j->buffer_ataque = false;
        PlaySound(som_dash);
        Vector2 pe = { j->rect.x + j->rect.width / 2, j->rect.y + j->rect.height };
        Anel(pe, (Color){ 120, 232, 255, 255 }, 40.0f, 0.3f);
        Rajada(pe, (Color){ 120, 232, 255, 255 }, 8, 180.0f, 5.0f, 0.3f, 0.0f, 1);
    }

    float vx;
    if (j->tempo_dash > 0.0f) {
        j->tempo_dash -= dt;
        vx = j->dir_dash * 760.0f;
        j->velocidade.y = 0.0f;
        j->rastro_t -= dt;
        if (j->rastro_t <= 0.0f) {
            static int idx = 0;
            j->rastro_t = 0.03f;
            rastros[idx].copia = *j;
            rastros[idx].vida = 0.28f;
            idx = (idx + 1) % MAX_RASTROS;
        }
    } else {
        vx = ax * j->velocidade_mov * (j->esta_atacando ? 0.45f : 1.0f) + j->empurrao;
        j->empurrao *= expf(-9.0f * dt);
        if (ax != 0.0f && !mira_travada && !j->esta_atacando) j->direcao = (ax > 0.0f) ? 1 : -1;
        j->velocidade.y += GRAVIDADE * dt;
    }
    if (mira_travada && chefao.alpha > 0.0f) {
        float cb = chefao.rect.x + chefao.rect.width / 2, cj = j->rect.x + j->rect.width / 2;
        if (!j->esta_atacando) j->direcao = (cb >= cj) ? 1 : -1;
    }
    j->velocidade.x = vx;
    j->rect.x += vx * dt;

    /* pulo e pulo duplo */
    if (controle && (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))) {
        if (j->esta_no_chao) {
            j->velocidade.y = FORCA_PULO; j->esta_no_chao = false; j->pulos = 1;
            SetSoundPitch(som_pulo, 1.0f); PlaySound(som_pulo);
        } else if (j->pulos < 2 && j->tempo_dash <= 0.0f) {
            j->velocidade.y = FORCA_PULO * 0.9f; j->pulos = 2;
            SetSoundPitch(som_pulo, 1.35f); PlaySound(som_pulo);
            Vector2 pe = { j->rect.x + j->rect.width / 2, j->rect.y + j->rect.height };
            Anel(pe, (Color){ 200, 240, 255, 255 }, 30.0f, 0.3f);
        }
    }
    j->rect.y += j->velocidade.y * dt;
    if (j->rect.y >= CHAO_Y - j->rect.height) {
        j->rect.y = CHAO_Y - j->rect.height;
        if (!estava_no_chao && j->velocidade.y > 100.0f) {
            j->squash = Clamp(j->velocidade.y / 900.0f, 0.0f, 1.0f);
            Vector2 pe = { j->rect.x + j->rect.width / 2, CHAO_Y - 2 };
            for (int i = 0; i < 6; i++) Part(pe, (Vector2){ Rnd(-110, 110), Rnd(-60, -10) }, (Color){ 200, 200, 215, 255 }, 0.4f, 6.0f, 3, 150.0f);
        }
        j->velocidade.y = 0.0f;
        j->esta_no_chao = true;
        j->pulos = 0;
    }
    j->rect.x = Clamp(j->rect.x, 0.0f, LARGURA_TELA - j->rect.width);

    /* poeira ao correr */
    if (j->esta_no_chao && fabsf(vx) > 200.0f && GetRandomValue(0, 5) == 0)
        Part((Vector2){ j->rect.x + j->rect.width / 2, CHAO_Y - 2 }, (Vector2){ -j->direcao * Rnd(10, 40), Rnd(-40, -10) }, (Color){ 190, 190, 205, 255 }, 0.4f, 5.0f, 3, 60.0f);

    /* ataque */
    if (controle && (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_K) || IsKeyPressed(KEY_SPACE))) {
        if (!j->esta_atacando && j->tempo_dash <= 0.0f) IniciarAtaqueJogador();
        else if (j->esta_atacando) j->buffer_ataque = true;
    }
    if (j->esta_atacando) {
        j->tempo_ataque -= dt;
        float pa = 1.0f - j->tempo_ataque / j->duracao_ataque;
        if (!j->acertou && pa > 0.22f && pa < 0.78f) {
            float alc = (j->combo == 2) ? 100.0f : 78.0f;
            j->area_ataque = (Rectangle){ (j->direcao == 1) ? j->rect.x + j->rect.width - 6.0f : j->rect.x - alc + 6.0f,
                                          j->rect.y - 8.0f, alc, j->rect.height + 6.0f };
            if (chefao.vida > 0 && chefao.alpha > 0.3f && CheckCollisionRecs(j->area_ataque, chefao.rect)) AcertarBoss();
        }
        if (j->tempo_ataque <= 0.0f) {
            j->esta_atacando = false;
            j->tempo_combo = 0.4f;
            if (j->buffer_ataque && controle) IniciarAtaqueJogador();
        }
    }
}

/* ------------------------------------------------------------------------- */
/*  COMBATE: CHEFE (IA com telegrafos)                                       */
/* ------------------------------------------------------------------------- */
static void EscolherAtaque(float dist) {
    Personagem *b = &chefao;
    float f = b->enfurecido ? 1.35f : 1.0f;
    static const float dur[6] = { 0.55f, 0.75f, 0.65f, 0.60f, 0.80f, 0.60f };
    int r = GetRandomValue(0, 99), at = AT_GOLPE;
    bool perto = dist <= 110.0f;
    switch (b->tipo) {
        case BOSS_COLOSSO: at = perto ? ((r < 55) ? AT_GOLPE : AT_PISAO) : AT_PISAO; break;
        case BOSS_DEMONIO:
            if (perto) at = (r < 50) ? AT_GOLPE : ((r < 80) ? AT_INVESTIDA : AT_FOGO);
            else at = (r < 55) ? AT_FOGO : AT_INVESTIDA;
            break;
        default:
            if (perto) at = (r < 35) ? AT_GOLPE : ((r < 65) ? AT_TELEPORTE : AT_ORBES);
            else at = (r < 50) ? AT_ORBES : AT_TELEPORTE;
            break;
    }
    b->ataque = at;
    b->acao = B_PREPARO;
    b->tempo_total = b->tempo_acao = dur[at] / f;
    b->alvo_x = jogador.rect.x + jogador.rect.width / 2;
}

static void TeleportarBoss(void) {
    Personagem *b = &chefao;
    float f = b->enfurecido ? 1.35f : 1.0f;
    Color ac = AcentoBoss(BOSS_VAZIO);
    Vector2 de = { b->rect.x + b->rect.width / 2, b->rect.y + b->rect.height / 2 };
    Rajada(de, ac, 16, 260.0f, 8.0f, 0.6f, -60.0f, 1);
    Anel(de, ac, 70.0f, 0.4f);
    float cj = jogador.rect.x + jogador.rect.width / 2;
    float alvo = cj - jogador.direcao * 105.0f - b->rect.width / 2;
    if (alvo < 10.0f || alvo > LARGURA_TELA - b->rect.width - 10.0f) alvo = cj + jogador.direcao * 105.0f - b->rect.width / 2;
    b->rect.x = Clamp(alvo, 10.0f, LARGURA_TELA - b->rect.width - 10.0f);
    b->direcao = (cj >= b->rect.x + b->rect.width / 2) ? 1 : -1;
    b->alpha = 1.0f;
    Vector2 para = { b->rect.x + b->rect.width / 2, b->rect.y + b->rect.height / 2 };
    Rajada(para, ac, 16, 260.0f, 8.0f, 0.6f, -60.0f, 1);
    Anel(para, ac, 70.0f, 0.4f);
    SetSoundPitch(som_dash, 0.5f);
    PlaySound(som_dash);
    b->ataque = AT_GOLPE;
    b->acao = B_PREPARO;
    b->tempo_total = b->tempo_acao = 0.34f / f;
    b->fx = false;
}

static void ExecutarAtaqueBoss(void) {
    Personagem *b = &chefao;
    int nv = fase_atual > 10 ? 10 : fase_atual;
    float f = b->enfurecido ? 1.35f : 1.0f;
    float cb = b->rect.x + b->rect.width / 2, cj = jogador.rect.x + jogador.rect.width / 2;
    b->acertou = false;
    b->fx = false;
    switch (b->ataque) {
        case AT_GOLPE:
            b->acao = B_ATAQUE; b->tempo_total = b->tempo_acao = 0.3f;
            break;
        case AT_PISAO:
            b->acao = B_ATAQUE; b->tempo_total = b->tempo_acao = 3.0f;
            b->velocidade.y = -640.0f; b->esta_no_chao = false;
            b->vel_x = Clamp((b->alvo_x - cb) / 1.07f, -450.0f, 450.0f);
            SetSoundPitch(som_pulo, 0.5f); PlaySound(som_pulo);
            for (int i = 0; i < 8; i++) Part((Vector2){ cb, CHAO_Y - 2 }, (Vector2){ Rnd(-160, 160), Rnd(-80, -10) }, (Color){ 170, 160, 170, 255 }, 0.6f, 9.0f, 3, 120.0f);
            break;
        case AT_FOGO: {
            b->acao = B_ATAQUE; b->tempo_total = b->tempo_acao = 0.45f;
            int n = b->enfurecido ? 5 : 3;
            Vector2 o = { cb + b->direcao * (b->rect.width * 0.5f), b->rect.y + b->rect.height * 0.35f };
            float base = atan2f(jogador.rect.y + 35.0f - o.y, cj - o.x);
            for (int k = 0; k < n; k++) {
                float a = base + (k - (n - 1) / 2.0f) * 0.22f;
                NovoProjetil(0, o, (Vector2){ cosf(a) * 370.0f, sinf(a) * 370.0f }, 13.0f, 4.0f, 11 + nv);
            }
            Rajada(o, (Color){ 255, 150, 40, 255 }, 14, 260.0f, 8.0f, 0.4f, 0.0f, 1);
            SetSoundPitch(som_fogo, 1.0f); PlaySound(som_fogo);
        } break;
        case AT_INVESTIDA:
            b->acao = B_ATAQUE; b->tempo_total = b->tempo_acao = 0.5f;
            b->vel_x = b->direcao * 650.0f * (f > 1.0f ? 1.15f : 1.0f);
            SetSoundPitch(som_dash, 0.6f); PlaySound(som_dash);
            break;
        case AT_ORBES: {
            b->acao = B_ATAQUE; b->tempo_total = b->tempo_acao = 0.45f;
            int n = b->enfurecido ? 4 : 3;
            for (int k = 0; k < n; k++) {
                float a = -PI / 2.0f + (k - (n - 1) / 2.0f) * 0.9f;
                Vector2 pos = { cb + cosf(a) * 70.0f, b->rect.y + 20.0f + sinf(a) * 40.0f };
                NovoProjetil(2, pos, (Vector2){ cosf(a) * 120.0f, sinf(a) * 120.0f }, 14.0f, 4.5f, 10 + nv);
            }
            SetSoundPitch(som_fogo, 0.5f); PlaySound(som_fogo);
        } break;
        default: break;
    }
}

static void AtualizarBoss(float dt) {
    Personagem *b = &chefao;
    int nv = fase_atual > 10 ? 10 : fase_atual;
    static float passo_t = 0.0f;
    b->anim += dt;
    if (b->tempo_dano > 0.0f) b->tempo_dano -= dt;
    if (b->squash > 0.0f) b->squash = fmaxf(0.0f, b->squash - dt * 3.0f);
    b->raiva += ((b->enfurecido ? 1.0f : 0.0f) - b->raiva) * fminf(1.0f, dt * 2.5f);

    if (!b->enfurecido && b->vida <= b->vida_max / 2) {
        b->enfurecido = true;
        b->recarga = 0.4f;
        Vector2 c = { b->rect.x + b->rect.width / 2, b->rect.y + b->rect.height / 2 };
        Anel(c, AcentoBoss(b->tipo), 240.0f, 0.8f);
        Rajada(c, AcentoBoss(b->tipo), 30, 380.0f, 8.0f, 0.8f, 0.0f, 1);
        NovoTexto((Vector2){ c.x, b->rect.y - 30 }, "ENFURECIDO!", (Color){ 255, 70, 60, 255 }, 26);
        tremor_tela = 0.6f;
        flash_tela = 0.3f;
        flash_cor = (Color){ 255, 120, 40, 255 };
        SetSoundPitch(som_rugido, 1.0f);
        PlaySound(som_rugido);
    }
    float f = b->enfurecido ? 1.35f : 1.0f;
    float cj = jogador.rect.x + jogador.rect.width / 2, cb = b->rect.x + b->rect.width / 2;
    float dist = fabsf(cj - cb);
    bool voa = (b->tipo == BOSS_VAZIO);
    float mov = 0.0f;

    /* fisica vertical */
    float chao_b = CHAO_Y - b->rect.height;
    bool pousou = false;
    if (voa) {
        b->rect.y = chao_b - 28.0f + sinf(b->anim * 2.0f) * 8.0f;
        b->esta_no_chao = true;
    } else {
        b->velocidade.y += GRAVIDADE * dt;
        b->rect.y += b->velocidade.y * dt;
        if (b->rect.y >= chao_b) {
            if (!b->esta_no_chao && b->velocidade.y > 200.0f) pousou = true;
            b->rect.y = chao_b; b->velocidade.y = 0.0f; b->esta_no_chao = true;
        } else b->esta_no_chao = false;
    }

    switch (b->acao) {
        case B_ANDAR: {
            b->direcao = (cj >= cb) ? 1 : -1;
            b->recarga -= dt;
            float ideal = voa ? 190.0f : 70.0f;
            float vel = b->velocidade_mov * f * (voa ? 0.75f : 1.0f);
            if (dist > ideal + 8.0f) mov = b->direcao * vel;
            else if (voa && dist < ideal - 70.0f) mov = -b->direcao * vel * 0.8f;
            if (b->recarga <= 0.0f) EscolherAtaque(dist);
        } break;

        case B_PREPARO: {
            b->tempo_acao -= dt;
            float prep = 1.0f - b->tempo_acao / b->tempo_total;
            if (b->ataque == AT_PISAO) { if (prep < 0.65f) b->alvo_x = cj; }
            else if (b->ataque != AT_INVESTIDA && prep < 0.7f) b->direcao = (cj >= cb) ? 1 : -1;
            if (b->ataque == AT_TELEPORTE) {
                b->alpha = Clamp(b->tempo_acao / b->tempo_total, 0.0f, 1.0f);
                b->direcao = (cj >= cb) ? 1 : -1;
            }
            if (b->tipo == BOSS_DEMONIO && b->ataque == AT_FOGO && GetRandomValue(0, 2) == 0)
                Part((Vector2){ cb + b->direcao * 50.0f + Rnd(-12, 12), b->rect.y + 30.0f + Rnd(-12, 12) }, (Vector2){ 0, Rnd(-60, -20) }, (Color){ 255, 160, 40, 255 }, 0.4f, 6.0f, 1, 0.0f);
            if (b->tempo_acao <= 0.0f) {
                if (b->ataque == AT_TELEPORTE) TeleportarBoss();
                else ExecutarAtaqueBoss();
            }
        } break;

        case B_ATAQUE: {
            b->tempo_acao -= dt;
            float q = 1.0f - b->tempo_acao / b->tempo_total;
            switch (b->ataque) {
                case AT_GOLPE: {
                    if (!b->fx && q > 0.18f) {
                        b->fx = true;
                        Vector2 pf = { b->direcao == 1 ? b->rect.x + b->rect.width + 40.0f : b->rect.x - 40.0f, CHAO_Y - 10.0f };
                        Rajada(pf, AcentoBoss(b->tipo), 10, 300.0f, 7.0f, 0.4f, 400.0f, 2);
                        if (b->tipo == BOSS_COLOSSO) {
                            tremor_tela = fmaxf(tremor_tela, 0.18f);
                            SetSoundPitch(som_impacto, 1.3f); PlaySound(som_impacto);
                            Anel((Vector2){ pf.x, CHAO_Y }, (Color){ 200, 200, 210, 255 }, 60.0f, 0.3f);
                        } else {
                            SetSoundPitch(som_ataque, 0.55f); PlaySound(som_ataque);
                        }
                    }
                    if (!b->acertou && q > 0.15f && q < 0.85f) {
                        Rectangle r = AreaGolpe(b);
                        b->area_ataque = r;
                        if (CheckCollisionRecs(r, jogador.rect) && jogador.invencivel <= 0.0f && jogador.tempo_dash <= 0.0f) {
                            b->acertou = true;
                            DanoJogador(18 + nv * 2, b->direcao);
                        }
                    }
                } break;
                case AT_PISAO: {
                    mov = b->vel_x;
                    if (pousou) {
                        float cx = b->rect.x + b->rect.width / 2;
                        NovoProjetil(1, (Vector2){ cx - 30.0f, CHAO_Y }, (Vector2){ -330.0f * f, 0 }, 18.0f, 2.6f, 14 + nv);
                        NovoProjetil(1, (Vector2){ cx + 30.0f, CHAO_Y }, (Vector2){ 330.0f * f, 0 }, 18.0f, 2.6f, 14 + nv);
                        tremor_tela = 0.4f;
                        b->squash = 1.0f;
                        SetSoundPitch(som_impacto, 1.0f); PlaySound(som_impacto);
                        Anel((Vector2){ cx, CHAO_Y }, (Color){ 220, 210, 220, 255 }, 140.0f, 0.5f);
                        for (int i = 0; i < 14; i++) Part((Vector2){ cx + Rnd(-50, 50), CHAO_Y - 2 }, (Vector2){ Rnd(-200, 200), Rnd(-200, -30) }, (Color){ 150, 140, 150, 255 }, 0.7f, 10.0f, 3, 300.0f);
                        Rectangle sob = { b->rect.x - 10.0f, b->rect.y, b->rect.width + 20.0f, b->rect.height };
                        if (CheckCollisionRecs(sob, jogador.rect)) DanoJogador(20 + nv * 2, (cj >= cx) ? 1 : -1);
                        b->acao = B_RECUPERA; b->tempo_total = b->tempo_acao = 0.8f / f;
                        b->vel_x = 0.0f; mov = 0.0f;
                    } else if (b->tempo_acao <= 0.0f) {
                        b->acao = B_RECUPERA; b->tempo_total = b->tempo_acao = 0.6f; b->vel_x = 0.0f; mov = 0.0f;
                    }
                } break;
                case AT_INVESTIDA: {
                    mov = b->vel_x;
                    if (GetRandomValue(0, 1)) Part((Vector2){ cb - b->direcao * 30.0f, b->rect.y + Rnd(30, b->rect.height) }, (Vector2){ -b->direcao * Rnd(40, 120), Rnd(-30, 30) }, (Color){ 255, 120, 30, 255 }, 0.4f, 8.0f, 1, 0.0f);
                    Rectangle corpo = { b->rect.x - 6.0f, b->rect.y, b->rect.width + 12.0f, b->rect.height };
                    if (!b->acertou && CheckCollisionRecs(corpo, jogador.rect) && jogador.invencivel <= 0.0f && jogador.tempo_dash <= 0.0f) {
                        b->acertou = true;
                        DanoJogador(22 + nv * 2, b->direcao);
                    }
                    if (b->tempo_acao <= 0.0f) { b->acao = B_RECUPERA; b->tempo_total = b->tempo_acao = 0.8f / f; b->vel_x = 0.0f; mov = 0.0f; }
                } break;
                default:
                    if (b->tempo_acao <= 0.0f) { b->acao = B_RECUPERA; b->tempo_total = b->tempo_acao = 0.55f / f; }
                    break;
            }
            if (b->ataque == AT_GOLPE && b->tempo_acao <= 0.0f) { b->acao = B_RECUPERA; b->tempo_total = b->tempo_acao = 0.55f / f; }
        } break;

        case B_RECUPERA:
            b->tempo_acao -= dt;
            if (b->tempo_acao <= 0.0f) { b->acao = B_ANDAR; b->recarga = Rnd(0.5f, 1.2f) / f; }
            break;
    }

    b->velocidade.x = mov;
    b->rect.x = Clamp(b->rect.x + mov * dt, 5.0f, LARGURA_TELA - b->rect.width - 5.0f);

    /* pegadas do colosso */
    if (b->tipo == BOSS_COLOSSO && b->acao == B_ANDAR && mov != 0.0f) {
        passo_t -= dt;
        if (passo_t <= 0.0f) {
            passo_t = 0.34f;
            tremor_tela = fmaxf(tremor_tela, 0.05f);
            Part((Vector2){ b->rect.x + b->rect.width / 2, CHAO_Y - 2 }, (Vector2){ Rnd(-60, 60), Rnd(-50, -10) }, (Color){ 150, 140, 150, 255 }, 0.5f, 8.0f, 3, 100.0f);
        }
    }

    /* aura de particulas */
    b->tempo_part -= dt;
    if (b->tempo_part <= 0.0f && b->alpha > 0.5f) {
        b->tempo_part = 0.07f;
        float x = b->rect.x + Rnd(0.0f, b->rect.width), y = b->rect.y + Rnd(10.0f, b->rect.height);
        if (b->tipo == BOSS_COLOSSO) {
            if (b->raiva > 0.3f) Part((Vector2){ x, b->rect.y + 25 }, (Vector2){ Rnd(-10, 10), -Rnd(30, 60) }, (Color){ 255, 130, 60, 255 }, 0.8f, 9.0f, 3, 0.0f);
            else if (GetRandomValue(0, 3) == 0) Part((Vector2){ b->rect.x + b->rect.width / 2, b->rect.y + 50 }, (Vector2){ Rnd(-20, 20), -Rnd(20, 50) }, (Color){ 90, 255, 210, 255 }, 0.8f, 3.0f, 1, 0.0f);
        } else if (b->tipo == BOSS_DEMONIO) {
            Part((Vector2){ x, y }, (Vector2){ Rnd(-20, 20), -Rnd(40, 110) }, (Color){ 255, (unsigned char)(100 + GetRandomValue(0, 100)), 40, 255 }, Rnd(0.6f, 1.1f), Rnd(2, 4), 1, -30.0f);
        } else {
            Part((Vector2){ x, y }, (Vector2){ Rnd(-15, 15), -Rnd(10, 50) }, (Color){ 200, 100, 255, 255 }, Rnd(0.8f, 1.4f), Rnd(2, 4), 1, 0.0f);
        }
    }
}

/* ------------------------------------------------------------------------- */
/*  BATALHA: intro, atualizacao geral                                        */
/* ------------------------------------------------------------------------- */
static void IniciarBatalha(bool curta) {
    ReiniciarJogo(&jogador, &chefao, fase_atual);
    mira_travada = false; vitoria = false;
    timer_fim = 0.0f; tempo_explosao = 0.0f;
    combo_hits = 0; tempo_combo_visual = 0.0f;
    hit_stop = 0.0f; tremor_tela = 0.0f; zoom_punch = 0.0f; flash_tela = 0.0f;
    vida_fantasma_jog = (float)jogador.vida;
    vida_fantasma_boss = (float)chefao.vida;
    intro_queda = !curta;
    intro_impacto = curta;
    intro_total = curta ? 0.9f : 2.6f;
    timer_intro = intro_total;
    if (!curta) {
        chefao.rect.y = -400.0f;
        if (chefao.tipo == BOSS_VAZIO) chefao.alpha = 0.0f;
    }
    tempo_dica = 9.0f;
    MudarEstado(ESTADO_JOGANDO);
}

static void AtualizarIntro(float dt) {
    timer_intro -= dt;
    float p = 1.0f - Clamp(timer_intro / intro_total, 0.0f, 1.0f);
    chefao.anim += dt; jogador.anim += dt;
    float gy = (chefao.tipo == BOSS_VAZIO) ? CHAO_Y - chefao.rect.height - 28.0f : CHAO_Y - chefao.rect.height;
    if (intro_queda && !intro_impacto) {
        float q = Clamp(p / 0.3f, 0.0f, 1.0f);
        chefao.rect.y = gy - 340.0f * (1.0f - q * q);
        if (chefao.tipo == BOSS_VAZIO) chefao.alpha = q;
        if (q >= 1.0f) {
            intro_impacto = true;
            chefao.rect.y = gy;
            tremor_tela = 0.55f;
            Vector2 pe = { chefao.rect.x + chefao.rect.width / 2, CHAO_Y };
            Anel(pe, AcentoBoss(chefao.tipo), 200.0f, 0.7f);
            for (int i = 0; i < 16; i++) Part(pe, (Vector2){ Rnd(-260, 260), Rnd(-220, -30) }, (Color){ 160, 150, 165, 255 }, 0.8f, 10.0f, 3, 300.0f);
            SetSoundPitch(som_impacto, 1.0f); PlaySound(som_impacto);
            SetSoundPitch(som_rugido, 1.0f); PlaySound(som_rugido);
        }
    }
    if (timer_intro <= 0.0f) { timer_intro = 0.0f; chefao.rect.y = gy; chefao.alpha = 1.0f; }
    AtualizarParticulas(dt);
    AtualizarTextos(dt);
    if (tremor_tela > 0.0f) tremor_tela -= dt;
}

static void AtualizarBatalha(float dt_real) {
    if (tremor_tela > 0.0f) tremor_tela -= dt_real;
    if (zoom_punch > 0.0f) zoom_punch = fmaxf(0.0f, zoom_punch - dt_real * 6.0f);
    if (flash_tela > 0.0f) flash_tela -= dt_real;
    if (tempo_dica > 0.0f) tempo_dica -= dt_real;
    if (tempo_combo_visual > 0.0f) { tempo_combo_visual -= dt_real; if (tempo_combo_visual <= 0.0f) combo_hits = 0; }
    vida_fantasma_jog = fmaxf((float)jogador.vida, vida_fantasma_jog - 35.0f * dt_real);
    vida_fantasma_boss = fmaxf((float)chefao.vida, vida_fantasma_boss - 90.0f * dt_real);

    if (timer_intro > 0.0f) { AtualizarIntro(dt_real); return; }

    float dt = dt_real;
    if (hit_stop > 0.0f) { hit_stop -= dt_real; dt = 0.0f; }
    else if (timer_fim > 0.0f) dt = dt_real * 0.3f;
    bool fim = timer_fim > 0.0f;

    if (fim) {
        timer_fim -= dt_real;
        if (vitoria) {
            tempo_explosao -= dt_real;
            if (tempo_explosao <= 0.0f) {
                tempo_explosao = 0.09f;
                Vector2 c = { chefao.rect.x + Rnd(0.0f, chefao.rect.width), chefao.rect.y + Rnd(0.0f, chefao.rect.height) };
                ImpactoVisual(c, AcentoBoss(chefao.tipo), 12);
                tremor_tela = fmaxf(tremor_tela, 0.1f);
            }
            chefao.alpha = Clamp(timer_fim / 1.2f, 0.0f, 1.0f);
        } else {
            Vector2 c = { jogador.rect.x + jogador.rect.width / 2, jogador.rect.y + 20.0f };
            if (GetRandomValue(0, 1)) Part(c, (Vector2){ Rnd(-40, 40), -Rnd(50, 150) }, (Color){ 120, 230, 255, 255 }, 1.2f, 7.0f, 1, -40.0f);
        }
        if (timer_fim <= 0.0f) {
            timer_fim = 0.0f;
            if (vitoria) {
                chefao.alpha = 0.0f;
                Vector2 c = { chefao.rect.x + chefao.rect.width / 2, chefao.rect.y + chefao.rect.height / 2 };
                Anel(c, WHITE, 300.0f, 0.8f);
                Anel(c, AcentoBoss(chefao.tipo), 220.0f, 1.0f);
                Rajada(c, GOLD, 60, 420.0f, 9.0f, 1.4f, 200.0f, 1);
                SetSoundPitch(som_vitoria, 1.0f); PlaySound(som_vitoria);
            } else {
                PlaySound(som_derrota);
            }
            MudarEstado(ESTADO_FIM_DE_JOGO);
        }
    }

    AtualizarJogador(dt, !fim);
    if (!(fim && vitoria)) AtualizarBoss(dt);
    else chefao.anim += dt;
    AtualizarProjeteis(dt);
    AtualizarParticulas(dt);
    AtualizarTextos(dt_real);
    for (int i = 0; i < MAX_RASTROS; i++) if (rastros[i].vida > 0.0f) rastros[i].vida -= dt_real;
}

/* ------------------------------------------------------------------------- */
/*  ARENA: renderizacao                                                      */
/* ------------------------------------------------------------------------- */
static void SombraChao(float cx, float rx, float altura, float alpha) {
    float k = 1.0f / (1.0f + altura / 160.0f);
    DrawEllipse((int)cx, CHAO_Y + 5, rx * k, 7.0f * k + 2.0f, Fade(BLACK, 0.42f * k * alpha));
}

static void DesenharArena(bool com_particulas) {
    int tema = chefao.tipo;
    float cj = jogador.rect.x + jogador.rect.width / 2;
    float foco = cj - LARGURA_TELA / 2.0f;
    float mag = (tremor_tela > 0.0f) ? fminf(tremor_tela * 28.0f, 11.0f) : 0.0f;
    Camera2D cam = { 0 };
    cam.offset = (Vector2){ LARGURA_TELA / 2.0f + Rnd(-mag, mag), ALTURA_TELA / 2.0f + Rnd(-mag, mag) };
    cam.target = (Vector2){ LARGURA_TELA / 2.0f, ALTURA_TELA / 2.0f };
    cam.zoom = 1.0f + 0.025f * Clamp(zoom_punch, 0.0f, 1.0f);

    BeginMode2D(cam);
        DesenharCenario(tema, tempo_global, foco);
        DesenharTelegrafo(&chefao);

        float alt_j = CHAO_Y - (jogador.rect.y + jogador.rect.height);
        float alt_b = CHAO_Y - (chefao.rect.y + chefao.rect.height);
        float ha = 1.0f;
        if (!vitoria && (timer_fim > 0.0f || estado == ESTADO_FIM_DE_JOGO)) ha = Clamp(timer_fim / 2.0f, 0.0f, 1.0f);
        SombraChao(jogador.rect.x + jogador.rect.width / 2, 26.0f, alt_j, ha);
        SombraChao(chefao.rect.x + chefao.rect.width / 2, chefao.rect.width * 0.6f, alt_b, chefao.alpha);

        float ox = (vitoria && timer_fim > 0.0f) ? Rnd(-4.0f, 4.0f) : 0.0f;
        DesenharBoss(&chefao, ox);

        for (int i = 0; i < MAX_RASTROS; i++)
            if (rastros[i].vida > 0.0f) DesenharHeroi(&rastros[i].copia, 0.5f * rastros[i].vida / 0.28f, true);
        if (ha > 0.01f) DesenharHeroi(&jogador, ha, false);

        DesenharProjeteis();

        if (mira_travada && chefao.alpha > 0.3f && chefao.vida > 0) {
            Vector2 c = { chefao.rect.x + chefao.rect.width / 2, chefao.rect.y - 30.0f + sinf(tempo_global * 5.0f) * 3.0f };
            float a = tempo_global * 2.0f;
            DrawRing(c, 9.0f, 11.0f, 0.0f, 360.0f, 24, Fade(RED, 0.9f));
            for (int k = 0; k < 4; k++) {
                float aa = a + k * PI / 2.0f;
                DrawLineEx((Vector2){ c.x + cosf(aa) * 12.0f, c.y + sinf(aa) * 12.0f }, (Vector2){ c.x + cosf(aa) * 18.0f, c.y + sinf(aa) * 18.0f }, 2.5f, Fade(RED, 0.9f));
            }
        }
        if (com_particulas) DesenharParticulas();
        DesenharTextos();
    EndMode2D();

    DesenharAmbiente(tema, tempo_global);
    DesenharVinheta(1.0f);
}

/* ------------------------------------------------------------------------- */
/*  HUD                                                                      */
/* ------------------------------------------------------------------------- */
static void BarraVida(float x, float y, float w, float h, float frac, float frac_fant, Color c1, Color c2, const char *rotulo, const char *valor, bool marca_meio) {
    frac = Clamp(frac, 0.0f, 1.0f); frac_fant = Clamp(frac_fant, 0.0f, 1.0f);
    DrawText(rotulo, (int)x + 1, (int)y - 22, 18, Fade(BLACK, 0.7f));
    DrawText(rotulo, (int)x, (int)y - 23, 18, RAYWHITE);
    DrawRectangleRounded((Rectangle){ x - 3, y - 3, w + 6, h + 6 }, 0.25f, 6, (Color){ 12, 10, 20, 220 });
    DrawRectangle((int)x, (int)y, (int)w, (int)h, (Color){ 34, 26, 40, 255 });
    DrawRectangle((int)x, (int)y, (int)(w * frac_fant), (int)h, (Color){ 255, 226, 150, 210 });
    DrawRectangleGradientH((int)x, (int)y, (int)(w * frac), (int)h, c1, c2);
    DrawRectangle((int)x, (int)y, (int)(w * frac), (int)(h * 0.4f), Fade(WHITE, 0.18f));
    if (marca_meio) DrawRectangle((int)(x + w * 0.5f) - 1, (int)y - 2, 2, (int)h + 4, Fade(WHITE, 0.7f));
    DrawRectangleLinesEx((Rectangle){ x - 3, y - 3, w + 6, h + 6 }, 2.0f, (Color){ 214, 182, 96, 255 });
    DrawRectangleLinesEx((Rectangle){ x, y, w, h }, 1.0f, Fade(BLACK, 0.6f));
    DrawText(valor, (int)(x + w - MeasureText(valor, 14) - 6), (int)y + (int)(h - 14) / 2, 14, Fade(WHITE, 0.9f));
    DrawCircleV((Vector2){ x - 3, y - 3 }, 3.5f, (Color){ 240, 202, 92, 255 });
    DrawCircleV((Vector2){ x + w + 3, y - 3 }, 3.5f, (Color){ 240, 202, 92, 255 });
    DrawCircleV((Vector2){ x - 3, y + h + 3 }, 3.5f, (Color){ 240, 202, 92, 255 });
    DrawCircleV((Vector2){ x + w + 3, y + h + 3 }, 3.5f, (Color){ 240, 202, 92, 255 });
}

static void DesenharHUD(void) {
    char val[32], rot[64];
    snprintf(val, sizeof(val), "%d/%d", jogador.vida, jogador.vida_max);
    BarraVida(34, 44, 260, 20, (float)jogador.vida / jogador.vida_max, vida_fantasma_jog / jogador.vida_max,
              (Color){ 28, 150, 84, 255 }, (Color){ 96, 232, 140, 255 }, "HERÓI", val, false);

    float pr = 1.0f - Clamp(jogador.recarga_dash / 0.75f, 0.0f, 1.0f);
    bool pronto = pr >= 1.0f;
    DrawText("ESQUIVA", 34, 74, 12, Fade(RAYWHITE, 0.75f));
    DrawRectangle(92, 77, 90, 6, (Color){ 20, 20, 30, 220 });
    DrawRectangle(92, 77, (int)(90 * pr), 6, pronto ? Fade((Color){ 120, 232, 255, 255 }, 0.7f + 0.3f * sinf(tempo_global * 8.0f)) : (Color){ 70, 120, 160, 255 });
    DrawText(mira_travada ? "[Z] MIRA: TRAVADA" : "[Z] MIRA: LIVRE", 34, 90, 12, mira_travada ? GREEN : GRAY);

    if (modo_atual == MODO_ARCADE) snprintf(rot, sizeof(rot), "%s  -  ONDA %d", NOMES_BOSS[chefao.tipo], fase_atual);
    else snprintf(rot, sizeof(rot), "%s", NOMES_BOSS[chefao.tipo]);
    int wb = 340;
    snprintf(val, sizeof(val), "%d/%d", chefao.vida, chefao.vida_max);
    float bx = LARGURA_TELA - wb - 34;
    Color c1 = chefao.enfurecido ? (Color){ 230, 40, 30, 255 } : (Color){ 180, 40, 60, 255 };
    BarraVida(bx, 44, (float)wb, 20, (float)chefao.vida / chefao.vida_max, vida_fantasma_boss / chefao.vida_max, c1, (Color){ 255, 110, 90, 255 }, rot, val, true);
    if (chefao.enfurecido && chefao.vida > 0) {
        const char *e = "ENFURECIDO";
        DrawText(e, (int)(bx + wb - MeasureText(e, 14)), 72, 14, Fade((Color){ 255, 80, 60, 255 }, 0.6f + 0.4f * sinf(tempo_global * 8.0f)));
    }

    if (combo_hits >= 2 && tempo_combo_visual > 0.0f) {
        snprintf(rot, sizeof(rot), "%d HITS", combo_hits);
        float a = Clamp(tempo_combo_visual, 0.0f, 1.0f);
        DrawText(rot, 36, 118, 30, Fade(BLACK, a * 0.7f));
        DrawText(rot, 34, 116, 30, Fade(combo_hits >= 6 ? GOLD : (Color){ 255, 230, 160, 255 }, a));
    }

    const char *dica = "[A/D] Mover   [W] Pular (2x)   [J/Espaço] Atacar (combo)   [Shift] Esquiva   [Z] Lock-On   [E] Pausar";
    float da = tempo_dica > 0.0f ? 0.9f : 0.4f;
    DrawText(dica, LARGURA_TELA / 2 - MeasureText(dica, 14) / 2, ALTURA_TELA - 26, 14, Fade(RAYWHITE, da));

    if (flash_tela > 0.0f) DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, Fade(flash_cor, Clamp(flash_tela * 1.2f, 0.0f, 0.5f)));

    /* introducao cinematografica */
    if (timer_intro > 0.0f) {
        float p = 1.0f - timer_intro / intro_total;
        float k = fminf(fminf(p / 0.12f, 1.0f), fminf((1.0f - p) / 0.12f, 1.0f));
        int barra = (int)(58.0f * Suave(k));
        DrawRectangle(0, 0, LARGURA_TELA, barra, BLACK);
        DrawRectangle(0, ALTURA_TELA - barra, LARGURA_TELA, barra, BLACK);
        if (p > 0.3f && p < 0.96f) {
            float a = Suave((p - 0.3f) / 0.15f) * Suave((0.96f - p) / 0.08f);
            float slide = (1.0f - Suave((p - 0.3f) / 0.2f)) * 40.0f;
            Color ac = AcentoBoss(chefao.tipo);
            TextoTitulo(NOMES_BOSS[chefao.tipo], LARGURA_TELA / 2.0f + slide, 190, 50, 6, Fade(RAYWHITE, a), Fade(ac, a));
            Font f = GetFontDefault();
            Vector2 m = MeasureTextEx(f, SUBS_BOSS[chefao.tipo], 20, 3);
            DrawTextEx(f, SUBS_BOSS[chefao.tipo], (Vector2){ LARGURA_TELA / 2.0f - m.x / 2 - slide, 252 }, 20, 3, Fade(ac, a));
        }
    }
}

/* ------------------------------------------------------------------------- */
/*  MAPA (MODO HISTORIA)                                                     */
/* ------------------------------------------------------------------------- */
static float anim_mapa = 0.0f, passo_mapa = 0.0f;

static void AtualizarMapa(float dt) {
    float vel_mapa = 220.0f;
    Vector2 mv = { 0, 0 };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) mv.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) mv.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) mv.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) mv.x += 1.0f;
    bool andando = (mv.x != 0.0f || mv.y != 0.0f);
    if (andando) {
        float len = sqrtf(mv.x * mv.x + mv.y * mv.y);
        mv.x /= len; mv.y /= len;
        dir_mapa.x += (mv.x - dir_mapa.x) * fminf(1.0f, dt * 14.0f);
        dir_mapa.y += (mv.y - dir_mapa.y) * fminf(1.0f, dt * 14.0f);
        pos_jogador_mapa.x += mv.x * vel_mapa * dt;
        pos_jogador_mapa.y += mv.y * vel_mapa * dt;
        anim_mapa += dt;
        passo_mapa -= dt;
        if (passo_mapa <= 0.0f) {
            passo_mapa = 0.16f;
            Part((Vector2){ pos_jogador_mapa.x - mv.x * 8.0f, pos_jogador_mapa.y + 10.0f }, (Vector2){ Rnd(-12, 12), Rnd(-14, -2) }, (Color){ 190, 195, 205, 255 }, 0.5f, 4.0f, 3, 0.0f);
        }
    }
    if (pos_jogador_mapa.y < 190) pos_jogador_mapa.y = 190;
    if (pos_jogador_mapa.y > 310) pos_jogador_mapa.y = 310;
    if (pos_jogador_mapa.x < 50) pos_jogador_mapa.x = 50;
    if (pos_jogador_mapa.x > LARGURA_MAPA - 100) pos_jogador_mapa.x = LARGURA_MAPA - 100;

    float alvo = Clamp(pos_jogador_mapa.x, LARGURA_TELA / 2.0f, LARGURA_MAPA - LARGURA_TELA / 2.0f);
    cam_x_mapa += (alvo - cam_x_mapa) * fminf(1.0f, dt * 6.0f);
    AtualizarParticulas(dt);

    if (fase_atual <= 3) {
        float x_portao = portoes_boss[fase_atual - 1];
        if (pos_jogador_mapa.x >= x_portao - 20 && pos_jogador_mapa.x <= x_portao + 40) {
            IniciarBatalha(false);
        }
    }
}

static void ArvoreMapa(float x, float y, float esc, int seed, float t) {
    float sw = sinf(t * 0.9f + seed) * 2.0f;
    DrawEllipse((int)(x + 10 * esc), (int)(y + 24 * esc), 40 * esc, 14 * esc, Fade(BLACK, 0.3f));
    DrawCircleV((Vector2){ x, y }, 6.0f * esc, (Color){ 70, 44, 30, 255 });
    DrawCircleV((Vector2){ x + sw * 0.3f, y - 2 }, 38.0f * esc, (Color){ 18, 74, 42, 255 });
    DrawCircleV((Vector2){ x - 4 + sw * 0.6f, y - 5 }, 31.0f * esc, (Color){ 28, 104, 54, 255 });
    DrawCircleV((Vector2){ x - 8 + sw, y - 9 }, 22.0f * esc, (Color){ 52, 138, 70, 255 });
    DrawCircleV((Vector2){ x - 12 + sw, y - 13 }, 9.0f * esc, (Color){ 96, 176, 96, 200 });
}

static void HeroiMapa(Vector2 pos, Vector2 dir, float anim, bool andando) {
    float ang = atan2f(dir.y, dir.x), cs = cosf(ang), sn = sinf(ang);
    float bob = andando ? sinf(anim * 14.0f) * 1.5f : sinf(tempo_global * 2.0f) * 0.8f;
    Vector2 P = { pos.x, pos.y + bob };
#define RL_(lx, ly) ((Vector2){ P.x + (lx) * cs - (ly) * sn, P.y + (lx) * sn + (ly) * cs })
    Color aco = { 86, 140, 204, 255 }, esc = { 48, 88, 148, 255 }, ouro = { 242, 202, 92, 255 };
    Color capa = { 30, 96, 138, 255 }, helm = { 156, 180, 210, 255 }, neon = { 120, 232, 255, 255 };

    DrawEllipse((int)pos.x, (int)pos.y + 12, 17, 7, Fade(BLACK, 0.35f));
    float ond = sinf(anim * 10.0f) * (andando ? 4.0f : 1.0f);
    Tri(RL_(-4, -9), RL_(-4, 9), RL_(-28 - ond, ond), capa);
    Tri(RL_(-4, -6), RL_(-4, 6), RL_(-20 - ond * 0.6f, 0), Misturar(capa, neon, 0.2f));
    float pa = andando ? sinf(anim * 14.0f) * 6.0f : 0.0f;
    DrawCircleV(RL_(pa, -6), 4.5f, esc);
    DrawCircleV(RL_(-pa, 6), 4.5f, esc);
    DrawCircleV(P, 12.5f, aco);
    DrawCircleV(RL_(-1, 0), 8.0f, esc);
    DrawCircleV(RL_(0, -11), 6.5f, ouro);
    DrawCircleV(RL_(0, -11), 5.0f, esc);
    DrawCircleV(RL_(0, 11), 6.5f, ouro);
    DrawCircleV(RL_(0, 11), 5.0f, aco);
    DrawLineEx(RL_(4, 13), RL_(32, 13), 3.5f, (Color){ 214, 228, 242, 255 });
    DrawLineEx(RL_(5, 13), RL_(32, 13), 1.2f, neon);
    DrawLineEx(RL_(6, 9), RL_(6, 17), 3.0f, ouro);
    DrawCircleV(RL_(4, 0), 9.0f, helm);
    DrawCircleV(RL_(2, -2), 5.0f, Misturar(helm, WHITE, 0.3f));
    DrawLineEx(RL_(5, -3), RL_(11, -1), 2.2f, (Color){ 14, 20, 40, 255 });
    DrawLineEx(RL_(-3, 0), RL_(-11, 0), 3.0f, (Color){ 226, 62, 84, 255 });
#undef RL_
    Brilho(P, 30.0f, neon, 0.5f);
}

static void PortaoMapa(int i, float t) {
    float x = portoes_boss[i];
    Color ac = AcentoBoss(i);
    bool limpo = (i + 1 < fase_atual), atual = (i + 1 == fase_atual);
    float k = limpo ? 0.0f : (atual ? 1.0f : 0.35f);
    Color pedra = limpo ? (Color){ 66, 70, 80, 255 } : (Color){ 92, 94, 112, 255 };

    if (!limpo) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawRectangleGradientH((int)x - 12, 186, 27, 128, Fade(ac, 0.0f), Fade(ac, 0.4f * k));
        DrawRectangleGradientH((int)x + 15, 186, 27, 128, Fade(ac, 0.4f * k), Fade(ac, 0.0f));
        for (int s = 0; s < 6; s++) {
            float y = 190.0f + fmodf(t * 40.0f + s * 22.0f, 120.0f);
            DrawLineEx((Vector2){ x + 3, y }, (Vector2){ x + 27, y + sinf(t * 3.0f + s) * 6.0f }, 2.0f, Fade(ac, 0.5f * k));
        }
        EndBlendMode();
        Brilho((Vector2){ x + 15, 250 }, 100.0f, ac, 1.1f * k);
    }
    for (int s = 0; s < 2; s++) {
        float py = s ? 322.0f : 132.0f;
        DrawRectangleRounded((Rectangle){ x - 10, py, 50, 48 }, 0.2f, 4, pedra);
        DrawRectangleRounded((Rectangle){ x - 6, py + 4, 42, 14 }, 0.4f, 4, Fade(WHITE, 0.1f));
        DrawRing((Vector2){ x + 15, py + 26 }, 9.0f, 11.0f, 0.0f, 360.0f, 20, limpo ? Fade(GRAY, 0.5f) : Fade(ac, 0.5f + 0.4f * k));
        Vector2 tp = { x + 15, py + 26 };
        if (!limpo) {
            Brilho(tp, 38.0f, (Color){ 255, 150, 50, 255 }, 1.0f * k);
            Tri((Vector2){ tp.x - 4, tp.y + 2 }, (Vector2){ tp.x + 4, tp.y + 2 }, (Vector2){ tp.x + sinf(t * 9.0f + i + s) * 2.0f, tp.y - 11 - sinf(t * 13.0f + s) * 3.0f }, (Color){ 255, 190, 70, 255 });
        } else {
            DrawRectangle((int)x + 2 + s * 14, (int)py + 48 + (s ? -60 : 0), 10, 6, Fade(pedra, 0.9f));
        }
    }
    /* emblema */
    Vector2 em = { x + 15, 100 };
    DrawRing(em, 16.0f, 19.0f, 0.0f, 360.0f, 28, Fade(limpo ? GRAY : ac, limpo ? 0.5f : 0.4f + 0.6f * k));
    Color ic = limpo ? Fade(GRAY, 0.5f) : Fade(ac, 0.5f + 0.5f * k);
    if (i == 0) DrawRectangle((int)em.x - 6, (int)em.y - 6, 12, 12, ic);
    else if (i == 1) { Tri((Vector2){ em.x - 10, em.y - 8 }, (Vector2){ em.x - 3, em.y + 8 }, (Vector2){ em.x - 2, em.y - 10 }, ic); Tri((Vector2){ em.x + 10, em.y - 8 }, (Vector2){ em.x + 3, em.y + 8 }, (Vector2){ em.x + 2, em.y - 10 }, ic); }
    else { DrawRing(em, 5.0f, 8.0f, 0.0f, 360.0f, 16, ic); DrawCircleV(em, 2.5f, ic); }
    if (!limpo) Brilho(em, 40.0f, ac, 0.8f * k);

    char lb[24];
    snprintf(lb, sizeof(lb), "BOSS %d", i + 1);
    DrawText(lb, (int)x + 15 - MeasureText(lb, 16) / 2, 66, 16, Fade(RAYWHITE, limpo ? 0.4f : 1.0f));
    DrawText(NOMES_BOSS[i], (int)x + 15 - MeasureText(NOMES_BOSS[i], 13) / 2, 376, 13, Fade(RAYWHITE, limpo ? 0.35f : 0.8f));
    if (limpo) DrawText("DERROTADO", (int)x + 15 - MeasureText("DERROTADO", 12) / 2, 392, 12, Fade(GREEN, 0.7f));
}

static void DesenharMapa(void) {
    float t = tempo_global;
    Camera2D cam = { 0 };
    cam.target = (Vector2){ cam_x_mapa, 250.0f };
    cam.offset = (Vector2){ LARGURA_TELA / 2.0f, ALTURA_TELA / 2.0f };
    cam.zoom = 1.0f;
    float x0 = cam_x_mapa - 470.0f, x1 = cam_x_mapa + 470.0f;

    BeginMode2D(cam);
        DrawRectangleGradientV((int)x0, 0, 940, ALTURA_TELA, (Color){ 32, 112, 62, 255 }, (Color){ 22, 82, 48, 255 });
        for (int gi = (int)(x0 / 34.0f) - 1; gi < (int)(x1 / 34.0f) + 2; gi++) {
            for (int r = 0; r < 2; r++) {
                float gx = gi * 34.0f + Hash(gi * 3 + r) * 28.0f;
                float gy = (r == 0) ? 8.0f + Hash(gi * 5 + 1) * 160.0f : 332.0f + Hash(gi * 5 + 2) * 160.0f;
                float sw = sinf(t * 1.5f + gi) * 2.5f;
                Color gc = Misturar((Color){ 40, 140, 70, 255 }, (Color){ 76, 188, 96, 255 }, Hash(gi + r * 11));
                for (int k = -1; k <= 1; k++)
                    DrawLineEx((Vector2){ gx + k * 3.0f, gy }, (Vector2){ gx + k * 5.0f + sw, gy - 8.0f - Hash(gi + k + 40) * 4.0f }, 1.6f, gc);
                if (Hash(gi * 9 + r) > 0.84f) {
                    Color fc = (gi % 3 == 0) ? (Color){ 250, 230, 120, 255 } : ((gi % 3 == 1) ? (Color){ 250, 160, 200, 255 } : (Color){ 190, 200, 255, 255 });
                    DrawCircleV((Vector2){ gx + 6, gy - 3 }, 2.4f, fc);
                }
            }
        }
        DrawRectangle((int)x0, 170, 940, 160, Fade(BLACK, 0.16f));
        DrawRectangle((int)x0, 180, 940, 140, (Color){ 104, 116, 132, 255 });
        for (int gi = (int)(x0 / 38.0f) - 1; gi < (int)(x1 / 38.0f) + 2; gi++) {
            for (int row = 0; row < 4; row++) {
                float cx = gi * 38.0f + (row % 2) * 19.0f, cy = 184.0f + row * 34.0f;
                float h = Hash(gi * 7 + row * 3);
                DrawRectangleRounded((Rectangle){ cx, cy, 34, 30 }, 0.3f, 4, Misturar((Color){ 92, 104, 120, 255 }, (Color){ 128, 140, 156, 255 }, h));
            }
        }
        DrawRectangle((int)x0, 180, 940, 6, (Color){ 64, 74, 86, 255 });
        DrawRectangle((int)x0, 314, 940, 6, (Color){ 64, 74, 86, 255 });
        for (int gi = (int)(x0 / 30.0f) - 1; gi < (int)(x1 / 30.0f) + 2; gi++) {
            DrawCircleV((Vector2){ gi * 30.0f + Hash(gi) * 14.0f, 177 }, 4.0f + Hash(gi + 3) * 3.0f, (Color){ 120, 128, 140, 255 });
            DrawCircleV((Vector2){ gi * 30.0f + Hash(gi + 8) * 14.0f, 323 }, 4.0f + Hash(gi + 5) * 3.0f, (Color){ 120, 128, 140, 255 });
        }
        for (int i = 0; i < LARGURA_MAPA / 110 + 1; i++) {
            float x = i * 110.0f + Hash(i) * 50.0f;
            if (x < x0 - 80 || x > x1 + 80) continue;
            ArvoreMapa(x, 50.0f + Hash(i + 5) * 60.0f, 0.9f + Hash(i + 2) * 0.5f, i, t);
            ArvoreMapa(x + 55.0f, 410.0f + Hash(i + 9) * 60.0f, 0.9f + Hash(i + 4) * 0.5f, i + 50, t);
        }
        for (int i = 0; i < 3; i++) PortaoMapa(i, t);

        HeroiMapa(pos_jogador_mapa, dir_mapa, anim_mapa, IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT));
        DesenharParticulas();

        BeginBlendMode(BLEND_ADDITIVE);
        for (int i = 0; i < 40; i++) {
            float fx = cam_x_mapa - 460.0f + fmodf(Hash(i * 3) * 920.0f + sinf(t * 0.5f + i) * 30.0f + 920.0f, 920.0f);
            float fy = 20.0f + Hash(i * 3 + 1) * 460.0f + sinf(t * 0.9f + i * 1.7f) * 14.0f;
            float tw = 0.4f + 0.6f * (0.5f + 0.5f * sinf(t * 2.5f + i));
            DrawCircleV((Vector2){ fx, fy }, 7.0f, Fade((Color){ 200, 255, 140, 255 }, 0.07f * tw));
            DrawCircleV((Vector2){ fx, fy }, 1.8f, Fade((Color){ 230, 255, 170, 255 }, 0.9f * tw));
        }
        EndBlendMode();
    EndMode2D();

    static const Color tom[3] = { { 255, 150, 60, 28 }, { 255, 60, 40, 40 }, { 80, 30, 160, 56 } };
    int ti = fase_atual - 1; if (ti < 0) ti = 0; if (ti > 2) ti = 2;
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, tom[ti]);
    DesenharVinheta(0.9f);

    /* HUD do mapa */
    DrawRectangleGradientV(0, 0, LARGURA_TELA, 74, Fade(BLACK, 0.55f), Fade(BLACK, 0.0f));
    DrawText("MODO HISTÓRIA - MAPA", 30, 16, 22, GOLD);
    DrawText("Siga a estrada e enfrente os chefões!", 30, 44, 16, LIGHTGRAY);
    for (int i = 0; i < 3; i++) {
        Vector2 c = { LARGURA_TELA - 130.0f + i * 44.0f, 32.0f };
        bool limpo = (i + 1 < fase_atual), atual = (i + 1 == fase_atual);
        if (i > 0) DrawLineEx((Vector2){ c.x - 44 + 13, c.y }, (Vector2){ c.x - 13, c.y }, 2.0f, Fade(WHITE, 0.4f));
        DrawCircleV(c, 13.0f, Fade(BLACK, 0.6f));
        DrawRing(c, 11.0f, 13.0f, 0.0f, 360.0f, 24, limpo ? GREEN : (atual ? Fade(AcentoBoss(i), 0.6f + 0.4f * sinf(t * 5.0f)) : Fade(GRAY, 0.6f)));
        if (limpo) DrawText("v", (int)c.x - 4, (int)c.y - 8, 16, GREEN);
        else DrawText(TextFormat("%d", i + 1), (int)c.x - 4, (int)c.y - 8, 16, atual ? WHITE : GRAY);
    }
    if (fase_atual <= 3) {
        float dx = portoes_boss[fase_atual - 1] - cam_x_mapa;
        if (dx > 470.0f) {
            float a = 0.5f + 0.5f * sinf(t * 6.0f);
            for (int k = 0; k < 3; k++) {
                float x = LARGURA_TELA - 50.0f + k * 12.0f;
                DrawLineEx((Vector2){ x, 240 }, (Vector2){ x + 10, 250 }, 3.0f, Fade(GOLD, a * (0.4f + k * 0.3f)));
                DrawLineEx((Vector2){ x + 10, 250 }, (Vector2){ x, 260 }, 3.0f, Fade(GOLD, a * (0.4f + k * 0.3f)));
            }
        }
    }
    DrawText("[W/A/S/D] Mover pelo mapa  |  [E] Pausar", 30, ALTURA_TELA - 32, 15, Fade(WHITE, 0.85f));
}

/* ------------------------------------------------------------------------- */
/*  MENUS                                                                    */
/* ------------------------------------------------------------------------- */
static bool DesenharBotao(Rectangle rect, const char *texto, Color cor_base, Color cor_hover) {
    Vector2 m = GetMousePosition();
    bool sobre = CheckCollisionPointRec(m, rect);
    Rectangle r = rect;
    if (sobre) { r.x -= 4; r.y -= 2; r.width += 8; r.height += 4; }
    Color cor = sobre ? cor_hover : cor_base;

    DrawRectangleRounded((Rectangle){ r.x + 3, r.y + 5, r.width, r.height }, 0.3f, 8, Fade(BLACK, 0.45f));
    DrawRectangleRounded(r, 0.3f, 8, Misturar(cor, BLACK, 0.45f));
    DrawRectangleRounded((Rectangle){ r.x + 2, r.y + 2, r.width - 4, r.height - 4 }, 0.3f, 8, cor);
    DrawRectangleRounded((Rectangle){ r.x + 5, r.y + 4, r.width - 10, r.height * 0.42f }, 0.5f, 8, Fade(WHITE, sobre ? 0.3f : 0.15f));
    if (sobre) {
        BeginBlendMode(BLEND_ADDITIVE);
        DrawRectangleRounded((Rectangle){ r.x - 4, r.y - 4, r.width + 8, r.height + 8 }, 0.3f, 8, Fade(cor_hover, 0.18f));
        EndBlendMode();
    }
    int tam = 20, w = MeasureText(texto, tam);
    DrawText(texto, (int)(r.x + r.width / 2 - w / 2) + 1, (int)(r.y + r.height / 2 - tam / 2) + 2, tam, Fade(BLACK, 0.6f));
    DrawText(texto, (int)(r.x + r.width / 2 - w / 2), (int)(r.y + r.height / 2 - tam / 2), tam, RAYWHITE);

    if (sobre && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        SetSoundPitch(som_clique, 1.0f);
        PlaySound(som_clique);
        return true;
    }
    return false;
}

static void DesenharFundoMenu(float t, float escuro, bool com_personagens) {
    int tema = ((int)(t / 10.0f)) % 3;
    float ft = fmodf(t, 10.0f), cyc = fmodf(t, 6.0f);
    DesenharCenario(tema, t, 0.0f);

    if (com_personagens) {
        if (vitrine_boss.tipo != tema) {
            ConfigurarBoss(&vitrine_boss, tema, 3);
            vitrine_boss.rect.x = 710.0f;
        }
        Personagem *b = &vitrine_boss, *h = &vitrine_heroi;
        b->direcao = -1; h->direcao = 1;
        b->acao = B_ANDAR; b->ataque = AT_GOLPE;
        int at = (tema == BOSS_DEMONIO) ? AT_FOGO : ((tema == BOSS_COLOSSO) ? AT_GOLPE : AT_ORBES);
        if (cyc >= 3.0f && cyc < 3.8f) { b->acao = B_PREPARO; b->ataque = at; b->tempo_total = 0.8f; b->tempo_acao = 0.8f - (cyc - 3.0f); }
        else if (cyc >= 3.8f && cyc < 4.15f) { b->acao = B_ATAQUE; b->ataque = at; b->tempo_total = 0.35f; b->tempo_acao = 0.35f - (cyc - 3.8f); }
        if (cyc >= 4.1f && cyc < 4.4f) { h->esta_atacando = true; h->duracao_ataque = 0.3f; h->tempo_ataque = 0.3f - (cyc - 4.1f); h->combo = 0; }
        else h->esta_atacando = false;
        h->esta_no_chao = true; h->velocidade.x = 0.0f;
        SombraChao(h->rect.x + h->rect.width / 2, 26.0f, 0.0f, 1.0f);
        SombraChao(b->rect.x + b->rect.width / 2, b->rect.width * 0.6f, 0.0f, 1.0f);
        DesenharBoss(b, 0.0f);
        DesenharHeroi(h, 1.0f, false);
    }
    DesenharAmbiente(tema, t);
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, Fade(BLACK, escuro));
    DesenharVinheta(1.0f);
    float a = 0.0f;
    if (ft < 0.6f) a = 1.0f - ft / 0.6f; else if (ft > 9.4f) a = (ft - 9.4f) / 0.6f;
    if (a > 0.0f) DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, Fade(BLACK, a));
}

static void DesenharRaios(float cx, float cy, Color cor, float t) {
    BeginBlendMode(BLEND_ADDITIVE);
    for (int k = 0; k < 14; k++) {
        float a = k * 2.0f * PI / 14.0f + t * 0.2f;
        Tri((Vector2){ cx, cy }, (Vector2){ cx + cosf(a - 0.05f) * 800.0f, cy + sinf(a - 0.05f) * 800.0f }, (Vector2){ cx + cosf(a + 0.05f) * 800.0f, cy + sinf(a + 0.05f) * 800.0f }, Fade(cor, 0.09f));
    }
    EndBlendMode();
}

/* ------------------------------------------------------------------------- */
/*  MAIN                                                                     */
/* ------------------------------------------------------------------------- */
int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(LARGURA_TELA, ALTURA_TELA, "Beyond the Fallen - Boss Fight 2D");
    InitAudioDevice();
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    som_ataque = GerarSomAtaque();
    som_dano = GerarSomDano();
    som_pulo = GerarSomPulo();
    som_clique = GerarSomClique();
    som_rugido = GerarSomRugido();
    som_fogo = GerarSomFogo();
    som_impacto = GerarSomImpacto();
    som_dash = GerarSomDash();
    som_vitoria = GerarSomVitoria();
    som_derrota = GerarSomDerrota();
    musicas[0] = GerarMusica(78.0f, 0, 0);
    musicas[1] = GerarMusica(112.0f, 1, 0);
    musicas[2] = GerarMusica(138.0f, 1, 2);
    musicas[3] = GerarMusica(124.0f, 1, -3);

    ReiniciarJogo(&jogador, &chefao, fase_atual);
    ReiniciarJogo(&vitrine_heroi, &vitrine_boss, 1);
    vitrine_heroi.rect.x = 110.0f;
    vitrine_boss.rect.x = 710.0f;

    while (!quer_sair && !WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;
        tempo_global += dt;
        if (fade_tela > 0.0f) fade_tela -= dt * 2.2f;
        if (timer_mensagem > 0.0f) timer_mensagem -= dt;

        if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
            if (estado == ESTADO_PAUSADO) {
                estado = estado_anterior;
            } else if ((estado == ESTADO_JOGANDO && timer_fim <= 0.0f) || estado == ESTADO_EXPLORACAO) {
                estado_anterior = estado;
                estado = ESTADO_PAUSADO;
            }
        }

        /* musica */
        int alvo_faixa = faixa_atual;
        if (estado == ESTADO_JOGANDO) alvo_faixa = 1 + chefao.tipo;
        else if (estado == ESTADO_FIM_DE_JOGO) alvo_faixa = -1;
        else if (estado != ESTADO_PAUSADO) alvo_faixa = 0;
        if (alvo_faixa != faixa_atual) {
            if (faixa_atual >= 0) StopSound(musicas[faixa_atual]);
            faixa_atual = alvo_faixa;
            if (faixa_atual >= 0) PlaySound(musicas[faixa_atual]);
        } else if (faixa_atual >= 0 && !IsSoundPlaying(musicas[faixa_atual])) {
            PlaySound(musicas[faixa_atual]);
        }

        /* logica */
        if (estado == ESTADO_EXPLORACAO) AtualizarMapa(dt);
        else if (estado == ESTADO_JOGANDO) AtualizarBatalha(dt);
        else if (estado == ESTADO_FIM_DE_JOGO) {
            AtualizarParticulas(dt);
            AtualizarTextos(dt);
            chefao.anim += dt;
            if (tremor_tela > 0.0f) tremor_tela -= dt;
            if (zoom_punch > 0.0f) zoom_punch = 0.0f;
            if (vitoria && GetRandomValue(0, 2) == 0) {
                Color cf[4] = { GOLD, (Color){ 120, 232, 255, 255 }, (Color){ 255, 120, 200, 255 }, WHITE };
                Part((Vector2){ Rnd(0, LARGURA_TELA), -10 }, (Vector2){ Rnd(-40, 40), Rnd(60, 150) }, cf[GetRandomValue(0, 3)], 3.2f, 7.0f, 0, 60.0f);
            }
        } else if (estado != ESTADO_PAUSADO) {
            vitrine_heroi.anim += dt;
            vitrine_boss.anim += dt;
        }

        BeginDrawing();
            ClearBackground((Color){ 10, 10, 16, 255 });

            bool arena_atras = (estado == ESTADO_JOGANDO || estado == ESTADO_FIM_DE_JOGO || (estado == ESTADO_PAUSADO && estado_anterior == ESTADO_JOGANDO));
            bool mapa_atras = (estado == ESTADO_EXPLORACAO || (estado == ESTADO_PAUSADO && estado_anterior == ESTADO_EXPLORACAO));

            if (mapa_atras) DesenharMapa();
            else if (arena_atras) { DesenharArena(estado != ESTADO_FIM_DE_JOGO); DesenharHUD(); }

            /* MENU INICIAL */
            if (estado == ESTADO_MENU_INICIAL) {
                DesenharFundoMenu(tempo_global, 0.25f, true);
                float bob = sinf(tempo_global * 1.5f) * 3.0f;
                TextoTitulo("BEYOND THE FALLEN", LARGURA_TELA / 2.0f, 54.0f + bob, 50, 5, GOLD, (Color){ 255, 160, 40, 255 });
                const char *sub = "Projeto de Algoritmos & Programação";
                DrawText(sub, LARGURA_TELA / 2 - MeasureText(sub, 16) / 2, 118, 16, Fade(RAYWHITE, 0.7f));
                DrawLine(LARGURA_TELA / 2 - 160, 142, LARGURA_TELA / 2 - 10, 142, Fade(GOLD, 0.6f));
                DrawLine(LARGURA_TELA / 2 + 10, 142, LARGURA_TELA / 2 + 160, 142, Fade(GOLD, 0.6f));
                DrawRectanglePro((Rectangle){ LARGURA_TELA / 2.0f, 142.0f, 8, 8 }, (Vector2){ 4, 4 }, 45.0f, GOLD);

                if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 125, 168, 250, 46 }, "NOVO JOGO", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) MudarEstado(ESTADO_SELECAO_MODO);
                if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 125, 228, 250, 46 }, "CARREGAR JOGO", (Color){ 39, 174, 96, 255 }, (Color){ 46, 204, 113, 255 })) MudarEstado(ESTADO_CARREGAR);
                if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 125, 288, 250, 46 }, "CRÉDITOS", (Color){ 142, 68, 173, 255 }, (Color){ 155, 89, 182, 255 })) MudarEstado(ESTADO_CREDITOS);
                if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 125, 348, 250, 46 }, "SAIR", (Color){ 192, 57, 43, 255 }, (Color){ 231, 76, 60, 255 })) quer_sair = true;
            }

            /* SELECAO DE MODO */
            if (estado == ESTADO_SELECAO_MODO) {
                DesenharFundoMenu(tempo_global, 0.55f, false);
                TextoTitulo("SELEÇÃO DE MODO", LARGURA_TELA / 2.0f, 50, 36, 4, RAYWHITE, (Color){ 120, 180, 255, 255 });
                if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 150, 130, 300, 52 }, "MODO HISTÓRIA", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) {
                    modo_atual = MODO_HISTORIA; fase_atual = 1;
                    pos_jogador_mapa = (Vector2){ 100, 250 };
                    dir_mapa = (Vector2){ 1, 0 };
                    MudarEstado(ESTADO_EXPLORACAO);
                }
                TextoCentro("Atravesse o mapa e derrote 3 chefões lendários!", 190, 15, Fade(RAYWHITE, 0.75f));
                if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 150, 250, 300, 52 }, "MODO ARCADE", (Color){ 211, 84, 0, 255 }, (Color){ 230, 126, 34, 255 })) {
                    modo_atual = MODO_ARCADE; fase_atual = 1;
                    IniciarBatalha(false);
                }
                TextoCentro("Ondas infinitas de chefões com dificuldade crescente!", 310, 15, Fade(RAYWHITE, 0.75f));
                if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 80, 380, 160, 42 }, "VOLTAR", (Color){ 127, 140, 141, 255 }, (Color){ 149, 165, 166, 255 })) MudarEstado(ESTADO_MENU_INICIAL);
            }

            /* CARREGAR */
            if (estado == ESTADO_CARREGAR) {
                DesenharFundoMenu(tempo_global, 0.55f, false);
                TextoTitulo("CARREGAR PROGRESSO", LARGURA_TELA / 2.0f, 70, 36, 4, RAYWHITE, (Color){ 100, 220, 140, 255 });
                if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 130, 190, 260, 52 }, "CARREGAR SAVE 1", (Color){ 39, 174, 96, 255 }, (Color){ 46, 204, 113, 255 })) {
                    if (CarregarJogo(&jogador, &chefao, &fase_atual, &modo_atual, &pos_jogador_mapa)) {
                        mira_travada = false;
                        if (modo_atual == MODO_HISTORIA) MudarEstado(ESTADO_EXPLORACAO);
                        else IniciarBatalha(false);
                    } else {
                        snprintf(mensagem_status, sizeof(mensagem_status), "Nenhum arquivo 'savegame.txt' encontrado!");
                        timer_mensagem = 2.5f;
                    }
                }
                if (timer_mensagem > 0.0f) TextoCentro(mensagem_status, 262, 16, RED);
                if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 80, 340, 160, 42 }, "VOLTAR", (Color){ 127, 140, 141, 255 }, (Color){ 149, 165, 166, 255 })) MudarEstado(ESTADO_MENU_INICIAL);
            }

            /* CREDITOS */
            if (estado == ESTADO_CREDITOS) {
                DesenharFundoMenu(tempo_global, 0.6f, false);
                TextoTitulo("CRÉDITOS DO JOGO", LARGURA_TELA / 2.0f, 40, 36, 4, GOLD, (Color){ 255, 160, 40, 255 });
                TextoCentro("Desenvolvimento Geral & Código em C:", 120, 18, RAYWHITE);
                TextoCentro("Caio Freitas de Andrade Medeiros", 148, 16, LIGHTGRAY);
                TextoCentro("Gabriel Furtado Corrêa Miller", 168, 16, LIGHTGRAY);
                TextoCentro("João Pedro Guedes Alcoforado C Leão", 188, 16, LIGHTGRAY);
                TextoCentro("Engine Gráfica & Áudio:", 232, 18, RAYWHITE);
                TextoCentro("Raylib Framework (C99) - arte e sons gerados por código", 260, 16, LIGHTGRAY);
                TextoCentro("Instituição:", 304, 18, RAYWHITE);
                TextoCentro("Trabalho Universitário de C", 332, 16, LIGHTGRAY);
                if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 80, 392, 160, 42 }, "VOLTAR", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) MudarEstado(ESTADO_MENU_INICIAL);
            }

            /* PAUSA */
            if (estado == ESTADO_PAUSADO) {
                DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, Fade(BLACK, 0.7f));
                TextoTitulo("PAUSADO", LARGURA_TELA / 2.0f, 80, 46, 6, RAYWHITE, (Color){ 120, 180, 255, 255 });
                if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 100, 170, 200, 46 }, "CONTINUAR", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) estado = estado_anterior;
                if (modo_atual == MODO_HISTORIA) {
                    if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 100, 230, 200, 46 }, "SALVAR JOGO", (Color){ 39, 174, 96, 255 }, (Color){ 46, 204, 113, 255 })) {
                        SalvarJogo(&jogador, &chefao, fase_atual, modo_atual, pos_jogador_mapa);
                        snprintf(mensagem_status, sizeof(mensagem_status), "Progresso salvo com sucesso!");
                        timer_mensagem = 2.0f;
                    }
                    if (timer_mensagem > 0.0f) TextoCentro(mensagem_status, 282, 16, GREEN);
                }
                float pos_y_sair = (modo_atual == MODO_HISTORIA) ? 310.0f : 230.0f;
                if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 100, pos_y_sair, 200, 46 }, "MENU INICIAL", (Color){ 192, 57, 43, 255 }, (Color){ 231, 76, 60, 255 })) MudarEstado(ESTADO_MENU_INICIAL);
            }

            /* FIM DE JOGO / VITORIA */
            if (estado == ESTADO_FIM_DE_JOGO) {
                DrawRectangleGradientV(0, 0, LARGURA_TELA, ALTURA_TELA, Fade(BLACK, 0.55f), Fade(BLACK, 0.85f));
                if (vitoria) {
                    DesenharRaios(LARGURA_TELA / 2.0f, 130.0f, GOLD, tempo_global);
                    DesenharParticulas();
                    TextoTitulo("FASE CONCLUÍDA!", LARGURA_TELA / 2.0f, 90, 50, 5, (Color){ 120, 255, 150, 255 }, (Color){ 60, 220, 120, 255 });
                    if (modo_atual == MODO_HISTORIA && fase_atual >= 3) {
                        TextoCentro("PARABÉNS! Você zerou o Modo História!", 160, 22, GOLD);
                        if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 110, 230, 220, 46 }, "MENU INICIAL", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) MudarEstado(ESTADO_MENU_INICIAL);
                    } else if (modo_atual == MODO_HISTORIA) {
                        if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 110, 210, 220, 46 }, "CONTINUAR VIAGEM", (Color){ 39, 174, 96, 255 }, (Color){ 46, 204, 113, 255 })) {
                            pos_jogador_mapa.x = portoes_boss[fase_atual - 1] + 60.0f;
                            fase_atual++;
                            MudarEstado(ESTADO_EXPLORACAO);
                        }
                    } else {
                        if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 110, 200, 220, 46 }, "PRÓXIMA ONDA", (Color){ 39, 174, 96, 255 }, (Color){ 46, 204, 113, 255 })) {
                            fase_atual++;
                            IniciarBatalha(false);
                        }
                        if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 110, 260, 220, 46 }, "MENU INICIAL", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) MudarEstado(ESTADO_MENU_INICIAL);
                    }
                } else {
                    DrawRectangleGradientH(0, 0, 120, ALTURA_TELA, Fade((Color){ 160, 10, 10, 255 }, 0.4f), Fade(BLACK, 0.0f));
                    DrawRectangleGradientH(LARGURA_TELA - 120, 0, 120, ALTURA_TELA, Fade(BLACK, 0.0f), Fade((Color){ 160, 10, 10, 255 }, 0.4f));
                    DesenharParticulas();
                    TextoTitulo("VOCÊ FOI DERROTADO!", LARGURA_TELA / 2.0f, 100 + sinf(tempo_global * 2.0f) * 3.0f, 46, 4, (Color){ 235, 60, 60, 255 }, (Color){ 180, 20, 20, 255 });
                    if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 110, 200, 220, 46 }, "TENTAR NOVAMENTE", (Color){ 41, 128, 185, 255 }, (Color){ 52, 152, 219, 255 })) IniciarBatalha(true);
                    if (DesenharBotao((Rectangle){ LARGURA_TELA / 2 - 110, 260, 220, 46 }, "MENU INICIAL", (Color){ 192, 57, 43, 255 }, (Color){ 231, 76, 60, 255 })) MudarEstado(ESTADO_MENU_INICIAL);
                }
            }

            if (fade_tela > 0.0f) DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, Fade(BLACK, Clamp(fade_tela, 0.0f, 1.0f)));
        EndDrawing();
    }

    UnloadSound(som_ataque); UnloadSound(som_dano); UnloadSound(som_pulo); UnloadSound(som_clique);
    UnloadSound(som_rugido); UnloadSound(som_fogo); UnloadSound(som_impacto); UnloadSound(som_dash);
    UnloadSound(som_vitoria); UnloadSound(som_derrota);
    for (int i = 0; i < 4; i++) UnloadSound(musicas[i]);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
