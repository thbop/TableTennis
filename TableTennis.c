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
typedef          char i8;
typedef unsigned char u8;

// Defines and globals
#define FPS       60
#define WIDTH     200
#define HALFWIDTH 100
#define HEIGHT    192
#define WINRATIO  4
#define WINWIDTH  WINRATIO*WIDTH
#define WINHEIGHT WINRATIO*HEIGHT

#define BALLCOLOR (Color){ 235, 235, 235, 255 }

struct {
    Vector3 pos, vel;
    Vector2 projected_shadow;
    float radius;
} ball;

Vector2 ball_project(u8 shadow) {
    return (Vector2){
        fabsf( 0.28f*ball.pos.y+24 ) * ball.pos.x * 0.03 + HALFWIDTH,
        ball.pos.y - (shadow ? 0.0f : ball.pos.z)
    };
}

// Vector2 project(Vector3 p, u8 shadow, u8 overtable) {
//     return (Vector2){
//         (fabsf( 0.28f*p.y+24 ) * p.x * 0.03f + HALFWIDTH),
//         p.y - (shadow ? overtable : p.z)
//     };
// }


void ball_move() {
    // Project
    ball.projected_shadow = ball_project(1);

    // Floor Bounce
    if ( ball.pos.z <= 0.0f ) ball.vel.z = -ball.vel.z;
    else if ( ball.vel.z > -3.0f ) ball.vel.z -= 0.02;

    // Wall Bounce
    if ( fabsf(ball.pos.x) >= 45.0f )                  ball.vel.x = -ball.vel.x;
    if ( ball.pos.y <= 45.0f || ball.pos.y >= HEIGHT ) ball.vel.y = -ball.vel.y;

    ball.pos = Vector3Add(ball.pos, ball.vel);
    // Vector2MultiplyValue(ball.vel, 0.99f);
}

void ball_init() {
    ball.pos = (Vector3){ 10.0f, 100.0f, 10.0f };
    ball.vel = (Vector3){ 0.2f, 0.2f, 0.0f };
    ball.projected_shadow = ball_project(1);

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
    // printf("%f\n", ball.pos.x);
}


void Draw() {
    DrawTexture(background, 0, 0, WHITE);

    DrawTexture(table, 0, 0, WHITE);
    
    DrawCircleV(ball.projected_shadow, ball.radius, BLACK);
    DrawCircleV(ball_project(0), ball.radius, BALLCOLOR);

    // DrawLineV( project((Vector3){45.0f, 45.0f, 0.0f}, 1, 0), project((Vector3){45.0f, WIDTH, 0.0f}, 1, 0), GREEN );
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