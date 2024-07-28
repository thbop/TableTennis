#include "raylib.h"
#include "stdio.h"

// Defines and globals
#define FPS       60
#define WIDTH     200
#define HEIGHT    192
#define WINRATIO  4
#define WINWIDTH  WINRATIO*WIDTH
#define WINHEIGHT WINRATIO*HEIGHT

struct {
    Vector3 pos, vel;
    Color color;
} ball;

RenderTexture2D screen;
Texture2D background;

void Init() {
    InitWindow(WINWIDTH, WINHEIGHT, "Table Tennis!");
    SetTargetFPS(FPS);

    screen = LoadRenderTexture(WIDTH, HEIGHT);
    background = LoadTexture("graphics/background.png");
}

void Unload() {
    UnloadRenderTexture(screen);
    UnloadTexture(background);
    CloseWindow();
}

int main() {
    Init();

    while ( !WindowShouldClose() ) {
        BeginTextureMode(screen);
            ClearBackground(BLACK);
            DrawTexture(background, 0, 0, WHITE);
        EndTextureMode();

        BeginDrawing();
            DrawTexturePro(
                screen.texture,
                (Rectangle){ 0.0f, 0.0f, WIDTH, -HEIGHT },
                (Rectangle){ 0.0f, 0.0f, WINWIDTH, WINHEIGHT },
                (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE
            );
        EndDrawing();
    }

    Unload();
}