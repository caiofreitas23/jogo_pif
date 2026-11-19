#ifndef ESTATISTICAS_H
#define ESTATISTICAS_H
#include "config.h"

// colunas da matriz
#define STAT_SOCOS 0
#define STAT_CONTRA_ATAQUES 1
#define STAT_ESQUIVAS 2
#define STAT_TOTAL 3

// matriz: cada linha e um chefe, cada coluna e um tipo de acao
typedef struct {
    int dados[NUMERO_CHEFES][STAT_TOTAL];
} Estatisticas;

void estatisticas_zerar(Estatisticas *e);
void estatisticas_registrar(Estatisticas *e, int chefe, int tipo);
int estatisticas_pontos_chefe(const Estatisticas *e, int chefe);
int estatisticas_pontuacao(const Estatisticas *e);

#endif
