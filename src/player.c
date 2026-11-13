#include "player.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

void player_iniciar(Player *p) {
    printf("Digite seu nome: ");
    fgets(p->nome, sizeof(p->nome), stdin);
    p->nome[strcspn(p->nome, "\n")] = '\0';
    if (p->nome[0] == '\0') {
        strcpy(p->nome, "Jogador");
    }
    
    p->hp = PLAYER_VIDA;
    p->velocidade = PLAYER_VELOCIDADE;
    p->dano = PLAYER_DANO_BASE;
}