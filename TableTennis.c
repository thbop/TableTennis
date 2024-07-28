#include "raylib.h"
#include "raymath.h"
#include "stdio.h"
#include "math.h"

// float absf(float value) {
//     long v = (*(long*) &value);// & 0x7FFF;
//     printf("%f\n", v);
//     return *(float*) &v;
// }

Vector3 Vector3MultiplyValue(Vector3 p, float v) {
    return (Vector3){ p.x * v, p.y * v, p.z * v };
}

// Typedefs
typedef unsigned char u8;

// Defines and globals
#define FPS       60
#define WIDTH     200
#define HALFWIDTH 100
#define HEIGHT    192
#define WINRATIO  4
#define WINWIDTH  WINRATIO*WIDTH
#define WINHEIGHT WINRATIO*HEIGHT

struct {
    Vector3 pos, vel;
    float radius;
} ball;

Vector2 ball_project(u8 shadow, u8 overtable) {
    return (Vector2){
        fabsf( 0.4f*ball.pos.y ) * ball.pos.x + HALFWIDTH,
        ball.pos.y - (shadow ? overtable : ball.pos.z)
    };
}

void ball_move() {
    if ( ball.pos.z <= 0.0f ) {
        ball.vel.z = -ball.vel.z;
    }
    else if ( ball.vel.z > -2.0f ) ball.vel.z -= 0.01;

    ball.pos = Vector3Add(ball.pos, ball.vel);
    // Vector2MultiplyValue(ball.vel, 0.99f);
}

void ball_init() {
    ball.pos = (Vector3){ 0.0f, 100.0f, 10.0f };
    ball.vel = (Vector3){ 0.0f, 0.0f, 0.0f };

    ball.radius = 3.0f;
}


RenderTexture2D screen;
Texture2D background, table;

void Init() {
    InitWindow(WINWIDTH, WINHEIGHT, "Table Tennis!");
    SetTargetFPS(FPS);
    ball_init();

    screen = LoadRenderTexture(WIDTH, HEIGHT);
    background = LoadTexture("graphics/background.png");
    table = LoadTexture("graphics/table.png");
}

void Unload() {
    UnloadRenderTexture(screen);
    UnloadTexture(background);
    UnloadTexture(table);
    CloseWindow();
}

void Update() {
    ball_move();
}


void Draw() {
    DrawTexture(background, 0, 0, WHITE);
    DrawTexture(table, 0, 0, WHITE);
    DrawCircleV(ball_project(1, 0), ball.radius, BLACK);
    DrawCircleV(ball_project(0, 0), ball.radius, WHITE);
}


int main() {
    Init();


    while ( !WindowShouldClose() ) {
        Update();

        BeginTextureMode(screen);
            ClearBackground(BLACK);
            Draw();
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