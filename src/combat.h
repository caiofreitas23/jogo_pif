#ifndef COMBAT_H
#define COMBAT_H

#include "player.h"
#include "boss.h"

typedef enum {
    ACAO_ATACAR,
    ACAO_DEFENDER,
    ACAO_ESQUIVAR,
    ACAO_PULAR
} Acao;

void combat_atacar(Player *p, Boss *b);
void combat_defender(Player *p);
void combat_esquivar(Player *p);
void combat_pular(Player *p);


void combat_boss_atacar(Boss *b, Player *p);


void combat_executar_acao(Player *p, Boss *b, Acao acao);
int combat_verificar_fim(Player *p, Boss *b);

#endif