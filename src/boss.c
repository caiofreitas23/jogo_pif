#include "boss.h"
#include "config.h"
#include <string.h>

void boss_iniciar_bobrovsky(Boss *b) {
    strcpy(b->nome, "Bobrovsky");
    b->hp = BOBROVSKY_VIDA;
    b->dano = BOBROVSKY_DANO;
    b->velocidade = BOBROVSKY_VELOCIDADE;
}

void boss_iniciar_vladimir(Boss *b) {
    strcpy(b->nome, "Vladimir");
    b->hp = VLADIMIR_VIDA;
    b->dano = VLADIMIR_DANO;
    b->velocidade = VLADIMIR_VELOCIDADE;
}

void boss_iniciar_ivan(Boss *b) {
    strcpy(b->nome, "Ivan");
    b->hp = IVAN_VIDA;
    b->dano = IVAN_DANO;
    b->velocidade = IVAN_VELOCIDADE;
}

void boss_iniciar_anastasia(Boss *b) {
    strcpy(b->nome, "Anastasia");
    b->hp = ANASTASIA_VIDA;
    b->dano = ANASTASIA_DANO;
    b->velocidade = ANASTASIA_VELOCIDADE;
}