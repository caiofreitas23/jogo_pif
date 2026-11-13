#include "player.h"
#include "boss.h"
#include <stdio.h>

int main(void) {
    Player j;
    Boss bobrovsky;
    Boss vladimir;
    Boss ivan;
    Boss anastasia;

    player_iniciar(&j);
    boss_iniciar_bobrovsky(&bobrovsky);
    boss_iniciar_vladimir(&vladimir);
    boss_iniciar_ivan(&ivan);
    boss_iniciar_anastasia(&anastasia);

    printf("%s HP: %d\n", j.nome, j.hp);
    printf("%s HP: %d\n", bobrovsky.nome, bobrovsky.hp);
    printf("%s HP: %d\n", vladimir.nome, vladimir.hp);
    printf("%s HP: %d\n", ivan.nome, ivan.hp);
    printf("%s HP: %d\n", anastasia.nome, anastasia.hp);

    return 0;
}