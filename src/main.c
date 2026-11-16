#include "raylib.h"
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


    InitWindow(900, 600, "Beyond the Fallen");

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        BeginDrawing();

        ClearBackground(DARKGRAY);

        DrawText("BEYOND THE FALLEN", 260, 40, 35, WHITE);

        DrawText("JOGADOR", 80, 130, 25, YELLOW);
        DrawText(j.nome, 80, 180, 22, WHITE);
        DrawText(TextFormat("HP: %d", j.hp), 80, 215, 20, GREEN);

        DrawText("CHEFES", 500, 130, 25, RED);

        DrawText(TextFormat("%s - HP: %d", bobrovsky.nome, bobrovsky.hp), 500, 180, 20, WHITE);

        DrawText(TextFormat("%s - HP: %d", vladimir.nome, vladimir.hp), 500, 225, 20, WHITE);

        DrawText(TextFormat("%s - HP: %d", ivan.nome, ivan.hp), 500, 270, 20, WHITE);

        DrawText(TextFormat("%s - HP: %d", anastasia.nome, anastasia.hp), 500, 315, 20, WHITE);

        DrawText("Pressione ESC para sair", 320, 520, 20, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}