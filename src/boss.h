#ifndef BOSS_H
#define BOSS_H

typedef struct {
    char nome[50];
    int hp;
    int dano;
    int velocidade;
    int id;
    float x, y;
    int vida;
    int anim_frame;
    float timer;
} Boss;

void boss_iniciar_bobrovsky(Boss *b);
void boss_iniciar_vladimir(Boss *b);
void boss_iniciar_ivan(Boss *b);
void boss_iniciar_anastasia(Boss *b);
void boss_update(Boss *b);
void boss_draw(const Boss *b);

#endif