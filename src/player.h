#ifndef PLAYER_H
#define PLAYER_H

typedef struct {
    char nome[50];
    int hp;
    int velocidade;
    int dano;
} Player;

void player_iniciar(Player *p);

#endif