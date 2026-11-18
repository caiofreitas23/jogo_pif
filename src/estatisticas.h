#ifndef ESTATISTICAS_H
#define ESTATISTICAS_H
#define NUM_CHEFES 4
#include "config.h"

// colunas da matriz
#define STAT_SOCOS 0
#define STAT_CONTRA_ATAQUES 1
#define STAT_ESQUIVAS 2
#define STAT_TOTAL 3

// matriz: cada linha e um chefe, cada coluna e um tipo de acao
typedef struct {
    int dados[NUM_CHEFES][STAT_TOTAL];
} Estatisticas;

void estat_zerar(Estatisticas *e);
void estat_registrar(Estatisticas *e, int chefe, int tipo);
int estat_pontos_chefe(const Estatisticas *e, int chefe);
int estat_pontuacao(const Estatisticas *e);

#endif
