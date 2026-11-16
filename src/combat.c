
#include "combat.h"
#include "config.h"

static int calcular_dano(Player *p) {
    if (p->dano < 0) {
        return 0;
    }

    return p->dano;
}

void combat_atacar(Player *p, Boss *b) {
    if (p->hp <= 0 || b->hp <= 0) {
        return;
    }

    b->hp -= calcular_dano(p);

    if (b->hp < 0) {
        b->hp = 0;
    }
}

void combat_defender(Player *p) {
    if (p->hp <= 0 || p->cooldown_defesa > 0) {
        return;
    }

    p->defendendo = 1;
    p->esquivando = 0;
}

void combat_esquivar(Player *p) {
    if (p->hp <= 0 || p->cooldown_esquiva > 0) {
        return;
    }

    p->esquivando = 1;
    p->defendendo = 0;
}

void combat_pular(Player *p) {
    combat_esquivar(p);
}

void combat_boss_atacar(Boss *b, Player *p) {
    if (p->hp <= 0 || b->hp <= 0) {
        return;
    }

    int dano = b->dano;

    if (p->esquivando) {
        p->esquivando = 0;
        p->cooldown_esquiva = COOLDOWN_ESQUIVA;
        return;
    }

    if (p->defendendo) {
        dano = (dano + 1) / 2;

        p->defendendo = 0;
        p->cooldown_defesa = COOLDOWN_DEFESA;
    }

    p->hp -= dano;

    if (p->hp < 0) {
        p->hp = 0;
    }
}

void combat_executar_acao(Player *p, Boss *b, Acao acao) {
    if (p->hp <= 0 || b->hp <= 0) {
        return;
    }

    switch (acao) {
        case ACAO_ATACAR:
            combat_atacar(p, b);
            break;

        case ACAO_DEFENDER:
            combat_defender(p);
            break;

        case ACAO_ESQUIVAR:
            combat_esquivar(p);
            break;

        case ACAO_PULAR:
            combat_pular(p);
            break;
    }
}

void combat_atualizar(Player *p, float tempo) {
    if (tempo < 0) {
        return;
    }

    if (p->cooldown_defesa > 0) {
        p->cooldown_defesa -= tempo;

        if (p->cooldown_defesa < 0) {
            p->cooldown_defesa = 0;
        }
    }

    if (p->cooldown_esquiva > 0) {
        p->cooldown_esquiva -= tempo;

        if (p->cooldown_esquiva < 0) {
            p->cooldown_esquiva = 0;
        }
    }
}

int combat_verificar_fim(Player *p, Boss *b) {
    if (p->hp <= 0 && b->hp <= 0) {
        return COMBATE_EMPATE;
    }

    if (b->hp <= 0) {
        return COMBATE_VITORIA;
    }

    if (p->hp <= 0) {
        return COMBATE_DERROTA;
    }

    return COMBATE_EM_ANDAMENTO;
}
