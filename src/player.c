#include "player.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

void player_iniciar(Player *p) {
    printf("Digite seu nome: ");
    fflush(stdout);

    // se nao der para ler o nome, usa o nome padrao
    if (fgets(p->nome, sizeof(p->nome), stdin) == NULL) {
        p->nome[0] = '\0';
    }

    p->nome[strcspn(p->nome, "\n")] = '\0';
    if (p->nome[0] == '\0') {
        strcpy(p->nome, "Jogador");
    }
    
    p->hp = PLAYER_VIDA;
    p->velocidade = PLAYER_VELOCIDADE;
    p->dano = PLAYER_DANO_BASE;

    p->defendendo = 0;
    p->esquivando = 0;

    p->cooldown_defesa = 0;
    p->cooldown_esquiva = 0;
}