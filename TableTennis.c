#include "raylib.h"
#include "raymath.h"
#include "stdio.h"
#include "math.h"

// float absf(float value) {
//     long v = (*(long*) &value);// & 0x7FFF;
//     printf("%f\n", v);
//     return *(float*) &v;
// }

#define GETXY(p) (Vector2){ p.x, p.y }

Vector3 Vector3MultiplyValue(Vector3 p, float v) {
    return (Vector3){ p.x * v, p.y * v, p.z * v };
}

Vector2 Vector2MultiplyValue(Vector2 p, float v) {
    return (Vector2){ p.x * v, p.y * v };
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

#define TABLEHEIGHT 5

#define BALLCOLOR (Color){ 235, 235, 235, 255 }

#define PADDLETEXDIM     48.0f
#define PADDLEMAXFRAME   9
#define PADDLEFRAMEDELAY 1

#define PLAYERACC 0.1;

// Ball
struct {
    Vector3 pos, vel;
    float floor_z, radius;
} ball;

Vector2 ball_project(u8 shadow, u8 shadow_offset) {
    return (Vector2){
        fabsf( 0.28f*ball.pos.y+24 ) * ball.pos.x * 0.03 + HALFWIDTH,
        ball.pos.y - (shadow ?  shadow_offset : ball.pos.z)
    };
}

// Vector2 project(Vector3 p) {
//     return (Vector2){
//         (fabsf( 0.28f*p.y+24 ) * p.x * 0.03f + HALFWIDTH),
//         p.y
//     };
// }


void ball_move() {
    if ( ball.pos.x < 25.0f && ball.pos.x > -23.5f && ball.pos.y < 129.0f && ball.pos.y > 65.5f )
         ball.floor_z = TABLEHEIGHT;
    else ball.floor_z = 0.0f;

    // Floor Bounce
    if ( ball.pos.z <= ball.floor_z ) { ball.pos.z = ball.floor_z; ball.vel.z = -ball.vel.z; }
    else if ( ball.vel.z > -3.0f ) ball.vel.z -= 0.02;

    // Ceiling
    if ( ball.pos.z > 15.0f ) ball.pos.z = 15.0f;

    // Wall Bounce
    if ( fabsf(ball.pos.x) >= 45.0f )                  ball.vel.x = -ball.vel.x;
    if ( ball.pos.y <= 45.0f || ball.pos.y >= HEIGHT ) ball.vel.y = -ball.vel.y;

    ball.pos = Vector3Add(ball.pos, ball.vel);
    // Vector2MultiplyValue(ball.vel, 0.99f);

}

void ball_init() {
    ball.pos = (Vector3){ 10.0f, 100.0f, 10.0f };
    ball.vel = (Vector3){ 0.2f, 0.2f, 0.0f };

    ball.floor_z = 0.0f;
    ball.radius = 3.0f;
}

// Paddles
typedef struct {
    Texture2D tex;
    int frame, frametick;
    u8 animate;
    Vector2 pos, vel;
} paddle;

paddle paddle_init( char* texFileName, Vector2 pos ) {
    paddle p;
    p.tex = LoadTexture(texFileName);
    p.frame = p.frametick = p.animate = 0;
    p.pos = pos;
    p.vel = (Vector2){ 0.0f, 0.0f };
    return p;
}

void paddle_animate(paddle* p) {
    if ( p->animate ) {
        if ( p->frametick < PADDLEFRAMEDELAY ) p->frametick++;
        else {
            p->frametick = 0;
            if ( p->frame < PADDLEMAXFRAME ) p->frame++;
            else { p->frame = 0; p->animate = 0; }
        }
    }
    DrawTextureRec(
        p->tex,
        (Rectangle){ p->frame*PADDLETEXDIM, 0.0f, PADDLETEXDIM, PADDLETEXDIM },
        p->pos, WHITE
    );
}

void paddle_unload(paddle p) {
    UnloadTexture(p.tex);
}

// Player
paddle player;
void player_init() {
    player = paddle_init(
        "graphics/player_paddle.png",
        (Vector2){ 100.0f, 110.0f }
    );
}

void player_move() {
    if ( IsKeyDown(KEY_D) ) player.vel.x += PLAYERACC;
    if ( IsKeyDown(KEY_A) ) player.vel.x -= PLAYERACC;

    player.vel.x *= 0.98f;
    player.pos = Vector2Add( player.pos, player.vel );

    // printf("%f\n", player.pos.x);
    if      ( player.pos.x > 155.0f ) { player.vel.x = -player.vel.x*0.5f; player.pos.x = 155.0f; }
    else if ( player.pos.x < -15.0f ) { player.vel.x = -player.vel.x*0.5f; player.pos.x = -15.0f; }

    if ( IsKeyPressed(KEY_SPACE) ) {
        player.animate = 1;
    }
}


RenderTexture2D screen;
Texture2D background, table, table_mask;
Shader shadow_shader;
int table_mask_loc;

// float a;

void Init() {
    InitWindow(WINWIDTH, WINHEIGHT, "Table Tennis!");
    SetTargetFPS(FPS);

    screen = LoadRenderTexture(WIDTH, HEIGHT);
    background = LoadTexture("graphics/background.png");
    table = LoadTexture("graphics/table.png");
    table_mask = LoadTexture("graphics/table_mask.png");

    shadow_shader = LoadShader(0, "shadow.glsl");
    table_mask_loc = GetShaderLocation(shadow_shader, "mask");

    ball_init();
    player_init();

    // a = 0.0f;
}

void Unload() {
    UnloadRenderTexture(screen);
    UnloadTexture(background);
    UnloadTexture(table);
    UnloadTexture(table_mask);
    UnloadShader(shadow_shader);
    CloseWindow();
}

void Update() {
    ball_move();
    player_move();
    // printf("%f\n", ball.pos.x);

    // if (IsKeyDown(KEY_RIGHT)) a+=0.5f;
    // if (IsKeyDown(KEY_LEFT)) a-=0.5f;
    // printf("%f\n", a);
}


void Draw() {
    DrawTexture(background, 0, 0, WHITE);

    DrawCircleV(ball_project(1, 0), ball.radius, BLACK);
    DrawTexture(table, 0, 0, WHITE);
    
    BeginShaderMode(shadow_shader);
        SetShaderValueTexture(shadow_shader, table_mask_loc, table_mask);
        DrawCircleV(ball_project(1, 5), ball.radius, BLACK);
        // DrawRectangle(0, 0, HALFWIDTH, HEIGHT, WHITE);
    EndShaderMode();

    DrawCircleV(ball_project(0, 0), ball.radius, BALLCOLOR);
    // Vector2 a = ball_project(0);
    // printf("%f\n", a.y);

    // DrawLineV( project((Vector3){a, 45.0f, 0.0f}), project((Vector3){a, WIDTH, 0.0f}), GREEN );

    paddle_animate(&player);
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