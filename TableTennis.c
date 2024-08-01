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
#define XYTOXYZ(p, z) (Vector3){ p.x, p.y, z }
#define RECPLUSXY(r, p) (Rectangle){ r.x + p.x, r.y + p.y, r.width, r.height }
//#define QUICKFDIV2(f) (float)((long)f>>1)

Vector3 Vector3MultiplyValue(Vector3 p, float v) {
    return (Vector3){ p.x * v, p.y * v, p.z * v };
}

Vector2 Vector2MultiplyValue(Vector2 p, float v) {
    return (Vector2){ p.x * v, p.y * v };
}

float Q_rsqrt( float number ) {
    // Don't ask, watch this: https://youtu.be/p8u_k2LIZyo?si=n8mRBL3u3PjkUHsR
    long i;
    float x2, y;
    
    x2 = number * 0.5f;
    y = number;
    i = *( long* ) &y;
    i = 0x5F3759DF - ( i >> 1 );
    y = *(float*) &i;
    y *= ( 1.5f - ( x2 * y*y ) );

    return y;
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

#define UISELECT (Color){ 219, 65, 97, 255 }

#define TABLEHEIGHT 5
#define TABLECENTER (Vector2){ 0.0f, 97.0f }
#define NET3DPOSY   93.0f

#define BALLCOLOR (Color){ 235, 235, 235, 255 }

#define PADDLETEXDIM     48.0f
#define PADDLEMAXFRAME   9
#define PADDLEFRAMEDELAY 1
#define PADDLEHITFRAME   3
#define PLAYERHIT        (Rectangle){ 10.0f, 16.0f, 25.0f, 20.0f }
#define PLAYERHITHALFW   PLAYERHIT.width/2

#define WINSCORE 21

#define ENTERBLINKSPEED 20

// Hit line
// origin ----------- extend offset

#define PADDLEACC 0.1f

struct {
    u8 screen;     // 0 = menu, 1 = game, 2 = end
    u8 difficulty; // 0 = easy, 1 = normal, 2 = hard
    u8 bounces, bounces_com;
    u8 illegal_bounce, end;
    u8 score, score_com;
    int wait, ball_wait;
    u8 crt;
    int time;
} state;

typedef struct {
    Texture2D tex;
    int frame, frametick;
    u8 animate;
    Vector2 pos, vel;
} paddle;

// Globals
RenderTexture2D screen;
Texture2D background, table, table_mask, menu;
Sound sfx_bounce, sfx_bounce_table, sfx_hit, sfx_select, sfx_click;
Music game_music;
Shader shadow_shader, crt_shader;
int table_mask_loc, time_loc;
paddle player, com;

int enter_blink_tick;
u8  enter_blink;

// Ball
struct {
    Vector3 pos, vel;
    Vector2 project;
    float floor_z, radius;
} ball;



Vector2 ball_project(u8 shadow, u8 shadow_offset) {
    return (Vector2){
        fabsf( 0.28f*ball.pos.y+24 ) * ball.pos.x * 0.03f + HALFWIDTH,
        ball.pos.y - (shadow ?  shadow_offset : ball.pos.z)
    };
}

Vector2 project(Vector3 p) {
    return (Vector2){
        (fabsf( 0.28f*p.y+24 ) * p.x * 0.03f + HALFWIDTH),
        p.y
    };
}

void ball_init() {
    ball.pos = XYTOXYZ(TABLECENTER, 10.0f);
    ball.vel = (Vector3){ 0.0f, 1.0f, 0.0f };
    ball.project = ball_project(1, 0);

    ball.floor_z = 0.0f;
    ball.radius = 3.0f;
}


int sfx_hit_timer = 0;

void ball_hit_paddle(paddle p, int dir) {
    Rectangle hit = RECPLUSXY(PLAYERHIT, p.pos);
    if ( p.frame >= PADDLEHITFRAME && p.frame <= PADDLEMAXFRAME ) { // If can hit
        if ( CheckCollisionPointRec( ball.project, hit ) ) {
            ball.vel.x = ( ball.project.x - PLAYERHITHALFW - hit.x ) / ( hit.width );
            // printf("%f\n", ball.vel.x);
            ball.vel.y = ( ball.project.y - hit.y ) / ( hit.height * 5.0f ) + 1.0f;
            ball.vel.y *= dir;
            if ( !sfx_hit_timer ) { PlaySound(sfx_hit); sfx_hit_timer = 30; }

            state.bounces = state.bounces_com = 0;
        }
    }
    if (sfx_hit_timer) sfx_hit_timer--;
}

void ball_move() {
    ball.project = ball_project(0, 0);
    if ( ball.pos.x < 25.0f && ball.pos.x > -23.5f && ball.pos.y < 129.0f && ball.pos.y > 65.5f )
         ball.floor_z = TABLEHEIGHT;
    else ball.floor_z = 0.0f;

    // Floor Bounce
    if ( ball.pos.z <= ball.floor_z ) {
        ball.pos.z = ball.floor_z; ball.vel.z = -ball.vel.z;
        if ( ball.floor_z ) {
            PlaySound(sfx_bounce_table);
            if ( ball.pos.y > NET3DPOSY ) state.bounces++;
            else                          state.bounces_com++;
        }
        else {
            PlaySound(sfx_bounce);
            state.illegal_bounce = 1;
        }
    }
    else if ( ball.vel.z > -3.0f ) ball.vel.z -= 0.05;

    // Ceiling
    if ( ball.pos.z > 15.0f ) ball.pos.z = 15.0f;

    // Wall Bounce
    if ( fabsf(ball.pos.x) >= 45.0f )                  { ball.vel.x = -ball.vel.x; PlaySound(sfx_bounce); state.illegal_bounce = 1; }
    if ( ball.pos.y <= 45.0f || ball.pos.y >= HEIGHT ) { ball.vel.y = -ball.vel.y; PlaySound(sfx_bounce); state.illegal_bounce = 1; }

    // Paddle Bounce
    ball_hit_paddle(player, -1);
    ball_hit_paddle(com, 1);

    ball.pos = Vector3Add(ball.pos, ball.vel);
    // Vector2MultiplyValue(ball.vel, 0.99f);
}



// Paddles
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

void paddle_move(paddle* p) {
    p->vel.x *= 0.98f;
    p->pos = Vector2Add( p->pos, p->vel );

    // printf("%f\n", p->pos.x);
    if      ( p->pos.x > 165.0f ) { p->vel.x = -p->vel.x*0.5f; p->pos.x = 165.0f; }
    else if ( p->pos.x < -15.0f ) { p->vel.x = -p->vel.x*0.5f; p->pos.x = -15.0f; }
}

void paddle_unload(paddle p) {
    UnloadTexture(p.tex);
}

// Player
void player_init() {
    player = paddle_init(
        "graphics/player_paddle.png",
        (Vector2){ 100.0f, 100.0f }
    );
}

void player_move() {
    if ( IsKeyDown(KEY_D) ) player.vel.x += PADDLEACC;
    if ( IsKeyDown(KEY_A) ) player.vel.x -= PADDLEACC;

    paddle_move(&player);

    if ( IsKeyPressed(KEY_SPACE) ) {
        player.animate = 1;
    }
}

// Com
void com_init() {
    com = paddle_init(
        "graphics/com_paddle.png",
        (Vector2){ 100.0f, 47.0f }
    );
}

void com_move() {
    Rectangle hit = RECPLUSXY(PLAYERHIT, com.pos);
    switch ( state.difficulty ) {
        case 0: { // Easy
            if ( ball.vel.y < 0.0f ) {
                float rmag = Q_rsqrt( ball.vel.x*ball.vel.x + ball.vel.y*ball.vel.y );
                Vector3
                    // target      = { 25.0f, 128.0f, 5.0f },
                    balldir     = { ball.vel.x*rmag, ball.vel.y*rmag, 0.0f },
                    ball_impact = Vector3Add( Vector3MultiplyValue( balldir, ball.pos.y-com.pos.y ), ball.pos );

                Vector2 projected_ball_impact = project(ball_impact);
                // DrawCircleV( (Vector2){ projected_ball_impact.x, com.pos.y }, 3.0f, RED );

                if ( fabsf(com.vel.x) < 0.5f ) {
                    if ( hit.x+hit.width-2 < projected_ball_impact.x ) com.vel.x += PADDLEACC;
                    if ( hit.x+2 > projected_ball_impact.x )           com.vel.x -= PADDLEACC;
                }
                else com.vel.x += copysignf(PADDLEACC, -com.vel.x);

                if ( ball.project.y - hit.y+hit.height < 40.0f && !com.animate ) com.animate = 1;
            }
            else {
                if ( hit.x+hit.width-1 < 100.0f ) com.vel.x += PADDLEACC;
                if ( hit.x+1 > 100.0f )           com.vel.x -= PADDLEACC;
            }
        }
        case 1: { // Normal
            if ( hit.x+hit.width-1 < ball.project.x ) com.vel.x += PADDLEACC;
            if ( hit.x+1 > ball.project.x )           com.vel.x -= PADDLEACC;

            if ( ball.project.y - hit.y+hit.height < 35.0f && !com.animate ) com.animate = 1;
            break;
        }
        case 2: { // Hard
            if ( ball.vel.y < 0.0f ) {
                float rmag = Q_rsqrt( ball.vel.x*ball.vel.x + ball.vel.y*ball.vel.y );
                Vector3
                    target      = { 25.0f, 128.0f, 5.0f },
                    balldir     = { ball.vel.x*rmag, ball.vel.y*rmag, 0.0f },
                    ball_impact = Vector3Add( Vector3MultiplyValue( balldir, ball.pos.y-com.pos.y ), ball.pos );

                Vector2 projected_ball_impact = project(ball_impact);
                // DrawCircleV( (Vector2){ projected_ball_impact.x, com.pos.y }, 3.0f, RED );

                Vector3 aimdir = Vector3Subtract( target, ball_impact );
                rmag = Q_rsqrt( aimdir.x*aimdir.x + aimdir.y*aimdir.y );
                aimdir = Vector3MultiplyValue( aimdir, rmag );

                // float aim_x = ball.pos.x - (hit.width*aimdir.x) - PLAYERHITHALFW;
                // printf("%f\n", aim_x);
                // DrawCircleV( (Vector2){ aimdir.x+ball.project.x, com.pos.y+20 }, 3.0f, BLUE );

                if ( fabsf(com.vel.x) < 0.5f ) {
                    if ( hit.x+hit.width-2 < projected_ball_impact.x ) com.vel.x += PADDLEACC;
                    if ( hit.x+2 > projected_ball_impact.x )           com.vel.x -= PADDLEACC;
                }
                else com.vel.x += copysignf(PADDLEACC, -com.vel.x);

                if ( ball.project.y - hit.y+hit.height < 40.0f && !com.animate ) com.animate = 1;
            }
            else {
                if ( hit.x+hit.width-1 < 100.0f ) com.vel.x += PADDLEACC;
                if ( hit.x+1 > 100.0f )           com.vel.x -= PADDLEACC;
            }
        }
    }
    paddle_move(&com);
}

// Scoring
void score_draw() {
    DrawText(TextFormat("YOU %02d  COM %02d", state.score, state.score_com), 60, 13, 10, WHITE);
}

void score_update() {
    if ( state.illegal_bounce ) {
        int dir = 1;
        if ( state.bounces && state.bounces_com ) {
            if ( ball.pos.y > NET3DPOSY ) { state.score_com++; dir = 1; }
            else                          { state.score++; dir = -1; }
        }
        if      ( state.bounces && !state.bounces_com ) { state.score_com++; dir = 1; }
        else if ( !state.bounces && state.bounces_com ) { state.score++; dir = -1; }

        
        // Reset
        if ( dir == 1 ) { state.bounces = 0; state.bounces_com = 1; }
        else            { state.bounces = 1; state.bounces_com = 0; }
        state.illegal_bounce = 0;

        ball.pos = XYTOXYZ(TABLECENTER, 10.0f);
        ball.vel = (Vector3){ 0.0f, 1.0f*dir, 0.0f };

        ball_move();
        state.ball_wait = 30;
    }

    if ( !state.end && (state.score >= WINSCORE || state.score_com >= WINSCORE) ) {
        if ( !state.wait ) state.wait = 120;
        state.end = 1;
    }
    if ( state.end && !state.wait ) state.screen = 2;
}

void reset() {
    state.screen = 0;

    ball.pos   = XYTOXYZ(TABLECENTER, 10.0f);
    ball.vel   = (Vector3){ 0.0f, 1.0f, 0.0f };
    player.pos = (Vector2){ 100.0f, 100.0f };
    player.vel = (Vector2){ 0.0f, 0.0f };
    com.pos    = (Vector2){ 100.0f, 47.0f };
    com.vel    = (Vector2){ 0.0f, 0.0f };

    state.ball_wait   =
    state.end         =
    state.score       =
    state.score_com   =
    state.bounces     =
    state.bounces_com = 0;

    StopMusicStream(game_music);
    PlayMusicStream(game_music);
}

// Vector3 cursor;

void Init() {
    InitWindow(WINWIDTH, WINHEIGHT, "Table Tennis!");
    InitAudioDevice();
    SetTargetFPS(FPS);
    HideCursor();

    // cursor = (Vector3){ 0.0f, 100.0f, 5.0f };

    // State
    state.screen         = 0;
    state.difficulty     = 1;
    state.score          =   // Yes, I probably could use memset
    state.score_com      =
    state.illegal_bounce =
    state.end            =
    state.bounces        =
    state.bounces_com    =
    state.wait           = 
    state.ball_wait      =
    state.crt            =
    state.time           = 0;

    // Background graphics
    screen     = LoadRenderTexture(WIDTH, HEIGHT);
    background = LoadTexture("graphics/background.png");
    table      = LoadTexture("graphics/table.png");
    table_mask = LoadTexture("graphics/table_mask.png");
    menu       = LoadTexture("graphics/menu.png");

    shadow_shader  = LoadShader(0, "shaders/shadow.glsl");
    table_mask_loc = GetShaderLocation(shadow_shader, "mask");

    crt_shader  = LoadShader(0, "shaders/crt.glsl");
    time_loc    = GetShaderLocation(crt_shader, "time");

    // Sounds
    sfx_bounce       = LoadSound("audio/bounce.wav");
    sfx_bounce_table = LoadSound("audio/bounce_table.wav");
    sfx_hit          = LoadSound("audio/hit.wav");
    sfx_select       = LoadSound("audio/select.wav");
    sfx_click        = LoadSound("audio/click.wav");
    game_music       = LoadMusicStream("audio/music.wav");

    enter_blink_tick = enter_blink = 0;

    // Game objects
    ball_init();
    player_init();
    com_init();

    // a = 0.0f;
}

void Unload() {
    UnloadRenderTexture(screen);
    UnloadTexture(background);
    UnloadTexture(table);
    UnloadTexture(table_mask);
    UnloadTexture(menu);

    UnloadShader(shadow_shader);
    UnloadShader(crt_shader);

    UnloadSound(sfx_bounce);
    UnloadSound(sfx_bounce_table);
    UnloadSound(sfx_hit);
    UnloadSound(sfx_select);
    UnloadSound(sfx_click);
    UnloadMusicStream(game_music);

    paddle_unload(player);
    paddle_unload(com);

    CloseAudioDevice();
    CloseWindow();
}

void Update() {
    if ( IsKeyPressed(KEY_C) ) state.crt = !state.crt;
    if ( state.time < 3600 ) state.time++;
    else                     state.time = 0;

    if ( state.wait ) state.wait--;
    else {
        switch ( state.screen ) {
            case 0: { // Menu
                if ( IsKeyPressed(KEY_SPACE) ) {
                    state.difficulty++;
                    state.difficulty %= 3;
                    PlaySound(sfx_select);
                }
                if ( IsKeyPressed(KEY_ENTER) ) {
                    state.screen = 1;
                    state.wait   = 10;
                    PlaySound(sfx_click);
                    PlayMusicStream(game_music);
                }
                break;
            }
            case 1: { // Game
                if (!state.end) UpdateMusicStream(game_music);

                if ( state.ball_wait ) state.ball_wait--;
                else ball_move();

                player_move();
                com_move();

                score_update();

                // if ( IsKeyDown(KEY_RIGHT) ) cursor.x++;
                // if ( IsKeyDown(KEY_LEFT) ) cursor.x--;
                // if ( IsKeyDown(KEY_DOWN) ) cursor.y++;
                // if ( IsKeyDown(KEY_UP) ) cursor.y--;
                // printf("CURSOR %f %f\n", cursor.x, cursor.y);
                break;
            }
            case 2: { // End
                if ( enter_blink_tick < ENTERBLINKSPEED ) enter_blink_tick++;
                else { enter_blink_tick = 0; enter_blink = !enter_blink; }

                if ( IsKeyPressed(KEY_ENTER) ) {
                    PlaySound(sfx_click);
                    // Reset
                    reset();
                }
            }
        }
    }
    
    // printf("%f\n", ball.pos.x);

    // if (IsKeyDown(KEY_RIGHT)) a+=0.5f;
    // if (IsKeyDown(KEY_LEFT)) a-=0.5f;
    // printf("%f\n", a);
}

void Draw() {
    switch ( state.screen ) {
        case 0: { // Menu
            DrawTexture(menu, 0, 0, WHITE);

            DrawText("Difficulty", 70, 90, 10, WHITE);
                DrawText("Easy", 80, 100, 10, state.difficulty == 0 ? UISELECT : WHITE);
                DrawText("Normal", 80, 110, 10, state.difficulty == 1 ? UISELECT : WHITE);
                DrawText("Hard", 80, 120, 10, state.difficulty == 2 ? UISELECT : WHITE);
            
            break;
        }
        case 1: { // Game
            DrawTexture(background, 0, 0, WHITE);
            score_draw();

            DrawCircleV(ball_project(1, 0), ball.radius, BLACK);
            DrawTexture(table, 0, 0, WHITE);
            
            BeginShaderMode(shadow_shader);
                SetShaderValueTexture(shadow_shader, table_mask_loc, table_mask);
                DrawCircleV(ball_project(1, 5), ball.radius, BLACK);
                // DrawRectangle(0, 0, HALFWIDTH, HEIGHT, WHITE);
            EndShaderMode();

            paddle_animate(&com);
            DrawCircleV(ball.project, ball.radius, BALLCOLOR);
            // Vector2 a = ball_project(0);
            // printf("%f\n", a.y);

            // DrawLineV( project((Vector3){a, 45.0f, 0.0f}), project((Vector3){a, WIDTH, 0.0f}), GREEN );
            // DrawCircleV( project((Vector3){0.0f, a, 5.0f}), 2.0f, GREEN );

            paddle_animate(&player);

            // com_move(); // TEMP

            // DrawCircleV( project(cursor), 3.0f, GREEN );
            
            // DrawRectangleLinesEx(RECPLUSXY(PLAYERHIT, player.pos), 1.0f, GREEN);
            break;
        }
        case 2: {
            if ( state.score > state.score_com ) DrawText("YOU WIN!", 55, 70, 20, WHITE);
            if ( state.score < state.score_com ) DrawText("COM WIN!", 55, 70, 20, WHITE);

            DrawText(TextFormat("YOU %02d  COM %02d", state.score, state.score_com), 60, 95, 10, WHITE);

            if ( enter_blink ) DrawText( "[ENTER]", 80, 105, 10, WHITE );
            break;
        }
    }
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
            if ( state.crt ) { BeginShaderMode(crt_shader); SetShaderValue(crt_shader, time_loc, &state.time, SHADER_UNIFORM_INT); }
            DrawTexturePro(
                screen.texture,
                (Rectangle){ 0.0f, 0.0f, WIDTH, -HEIGHT },
                (Rectangle){ 0.0f, 0.0f, WINWIDTH, WINHEIGHT },
                (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE
            );
            if ( state.crt ) EndShaderMode();
        EndDrawing();
    }

    Unload();
}