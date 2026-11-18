#include "estatisticas.h"
#define NUM_CHEFES 4

void estat_zerar(Estatisticas *e) {
    int i, j;

    for (i = 0; i < NUM_CHEFES; i++) {
        for (j = 0; j < STAT_TOTAL; j++) {
            e->dados[i][j] = 0;
        }
    }
}

// soma 1 na acao feita contra o chefe (chefe vai de 1 a 4)
void estat_registrar(Estatisticas *e, int chefe, int tipo) {
    e->dados[chefe - 1][tipo]++;
}

// pontos de um chefe: quantidade de cada acao vezes o valor dela
int estat_pontos_chefe(const Estatisticas *e, int chefe) {
    const int *linha = e->dados[chefe - 1];

    return linha[STAT_SOCOS] * PONTOS_SOCO
         + linha[STAT_CONTRA_ATAQUES] * PONTOS_SOCO_CONTRA_ATAQUE
         + linha[STAT_ESQUIVAS] * PONTOS_ESQUIVA;
}

int estat_pontuacao(const Estatisticas *e) {
    int total = 0;
    int i;

    for (i = 1; i <= NUM_CHEFES; i++) {
        total += estat_pontos_chefe(e, i);
    }

    return total;
}
