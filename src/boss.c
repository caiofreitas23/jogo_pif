#include "boss.h"
#include "config.h"
#include <string.h>
#include "raylib.h"

void boss_iniciar_bobrovsky(Boss *b) {
    strcpy(b->nome, "Bobrovsky");
    b->hp = BOBROVSKY_VIDA;
    b->dano = BOBROVSKY_DANO;
    b->velocidade = BOBROVSKY_VELOCIDADE;
    b->id = 1;
    b->x = 600;   //Posição X
    b->y = 300;   //Posição Y
    b->anim_frame = 0;
    b->timer = 0.0f;
}

void boss_iniciar_vladimir(Boss *b) {
    strcpy(b->nome, "Vladimir");
    b->hp = VLADIMIR_VIDA;
    b->dano = VLADIMIR_DANO;
    b->velocidade = VLADIMIR_VELOCIDADE;
    b->id = 2;
    b->x = 600;   //Posição X
    b->y = 300;   //Posição Y
    b->anim_frame = 0;
    b->timer = 0.0f;

}

void boss_iniciar_ivan(Boss *b) {
    strcpy(b->nome, "Ivan");
    b->hp = IVAN_VIDA;
    b->dano = IVAN_DANO;
    b->velocidade = IVAN_VELOCIDADE;
    b->id = 3;
    b->x = 600;   //Posição X
    b->y = 300;   //Posição Y
    b->anim_frame = 0;
    b->timer = 0.0f;
    
}

void boss_iniciar_anastasia(Boss *b) {
    strcpy(b->nome, "Anastasia");
    b->hp = ANASTASIA_VIDA;
    b->dano = ANASTASIA_DANO;
    b->velocidade = ANASTASIA_VELOCIDADE;
    b->id = 4;
    b->x = 600;   //Posição X
    b->y = 300;   //Posição Y
    b->anim_frame = 0;
    b->timer = 0.0f;
}

void boss_update(Boss *b) {
    b->timer += GetFrameTime();
    if (b->timer >= 0.2f) { //timer do frame da animação
        b->anim_frame = (b->anim_frame + 1) % 4;
        b->timer = 0.0f;
    }
}

void boss_draw(const Boss *b) {
    switch (b->id) {
        case 1: // Bobrovsky
            DrawRectanglePro(
                (Rectangle){ b->x, b->y, 80, 120 }, 
                (Vector2){ 40, 60 }, 
                (float)(b->anim_frame * 2), 
                RED
            );
            break;
        case 2: //Vladimir
            DrawRectangle((int)(b->x + (b->anim_frame * 3)), (int)b->y, 90, 110, BLUE);
            break;
        case 3: //Ivan
            DrawRectangle((int)b->x, (int)b->y, 100, 100, (b->anim_frame % 2 == 0) ? DARKGREEN : GREEN);
            break;
        case 4: //Anastasia 
            DrawCircle((int)b->x + 50, (int)b->y + 50, 50 + (b->anim_frame * 2), PURPLE);
            break;
        default:
            DrawRectangle((int)b->x, (int)b->y, 80, 80, GRAY);
            break;
    }
}