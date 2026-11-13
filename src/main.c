#include "player.h"
#include "boss.h"
#include <stdio.h>

int main(void) {
    Player j;
    Boss c;

    player_iniciar(&j);
    boss_iniciar_bobrovsky(&c);

    return 0;
}