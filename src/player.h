#ifndef PLAYER_H
#define PLAYER_H

typedef struct {
    char nome[50];
    int hp;
    int velocidade;
    int dano;

    int defendendo;
    int esquivando;
    
    float cooldown_defesa;
    float cooldown_esquiva;
} Player;

void player_iniciar(Player *p);

#endif