#include <stdio.h>
#include <time.h>

// implementation added to raylib.h
#include "stb_image.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include "raylib.h"
#include "raymath.h"

#if defined(PLATFORM_DESKTOP)
   #define GLSL_VERSION 330
#else 
   #define GLSL_VERSION 100
#endif

#define FONT_SIZE_LOAD 160 

#define HUD_LIFETIME 0.7 
#define HUD_DEFAULT_FONTSIZE 40
#define HUD_LARGE_FONTSIZE 75
#define DEFAULT_IMAGE_SCALE 0.5

#define BACKGROUND_COLOR 0x181818AA
#define PROVINCE_CORRECT_COLOR 0xE9E9E9AA

unsigned long rgb_to_hex(int r, int g, int b) {   
    return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}

typedef struct {
    unsigned long key;
    char* value;
    bool guessed;
} Province;
    
Province *PROVINCES = NULL;

Province* select_random_province()
{
    bool found = false;
    Province *p;

    bool any_not_guessed = false;
    for (int i = 0; i < hmlen(PROVINCES); ++i) {
        p = &PROVINCES[i];
        if (!p->guessed) {
            any_not_guessed = true;
        }
    }

    if (!any_not_guessed) { 
        printf("There are no provinces to guess!\n");
        return NULL;
    }

    while (!found) {
        int i = rand() % hmlen(PROVINCES);
        p = &PROVINCES[i];
        found = !p->guessed;
    } 

    return p;
}

typedef enum {
    QUIZ = 0,
    VICTORY,
    LEARN,
} GameState; 

Font font = {0};
Shader shader = {0};
Image color_image = {0};
Image black_white_image = {0}; 

typedef struct {
    Vector2 ul; 
    Vector2 lr;
    float width;
    float height;
} Rec;
    
static size_t error_counter = 0;

void quiz(GameState *state, Rec *rec, Texture *texture, Camera2D *camera, Province **hidden_province)
{
    static bool draw_wrong_msg = false;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), *camera);

        printf("mouse.x: %.5lf; mouse.y: %.5lf\n", mouse.x, mouse.y);
        DrawCircle(mouse.x, mouse.y, 10.0, RED);

        printf("ul_corner: (%.5lf, %.5lf); lr_corner: (%.5lf, %.5lf)\n", rec->ul.x, rec->ul.y, rec->lr.x, rec->lr.y);

        if ( (mouse.x < rec->ul.x) || (mouse.x > rec->lr.x) || (mouse.y < rec->ul.y) || (mouse.y > rec->lr.y)) {
            printf("Click is outside the image!\n");
        } 
        else {
            int imgx = (int) ( (mouse.x - (GetScreenWidth()/2 - DEFAULT_IMAGE_SCALE*texture->width/2)) / DEFAULT_IMAGE_SCALE);
            int imgy = (int) ( (mouse.y - (GetScreenHeight()/2 - DEFAULT_IMAGE_SCALE*texture->height/2)) / DEFAULT_IMAGE_SCALE);
            Color c = GetImageColor(color_image, imgx, imgy);
            unsigned long hex = rgb_to_hex(c.r, c.g, c.b);

            printf("Click is inside! imgx = %d; imgy = %d; color: (%d, %d, %d, %d) => #%06lx\n", 
                    imgx, imgy, c.r, c.g, c.b, c.a, hex);

            char* clicked_province = hmget(PROVINCES, hex);
            if (clicked_province != NULL) {
                if (strcmp(clicked_province, (*hidden_province)->value) == 0) {
                    //printf("Province name = %s\n", province_name);

                    for (int px = 0; px < color_image.width; ++px) {
                        for (int py = 0; py < color_image.height; ++py) {
                            Color current_color = GetImageColor(color_image, px, py);
                            if ((current_color.r == c.r) && 
                                    (current_color.g == c.g) && 
                                    (current_color.b == c.b) && 
                                    (current_color.a == c.a)) {
                                ImageDrawPixel(&black_white_image, px, py, GetColor(PROVINCE_CORRECT_COLOR));
                            }
                        }
                    } 

                    UnloadTexture(*texture);
                    *texture = LoadTextureFromImage(black_white_image);

                    (*hidden_province)->guessed = true;
                    *hidden_province = select_random_province();

                    if (*hidden_province == NULL) {
                        *state = VICTORY;
                    }
                } else {
                    error_counter += 1;
                    draw_wrong_msg = true;
                }
            } else {
                printf("Province name unknown! Possibly a border has been clicked. \n\n");
            }
        }
    }

    if (*hidden_province != NULL) {
        Vector2 pos_find_str = CLITERAL(Vector2) {
            rec->ul.x + 0.03 * DEFAULT_IMAGE_SCALE*texture->width,
            rec->ul.y + 0.01 * DEFAULT_IMAGE_SCALE*texture->height
        };

        Vector2 pos_errors_str = CLITERAL(Vector2) {
            rec->ul.x + 0.75 * DEFAULT_IMAGE_SCALE*texture->width, 
            rec->ul.y + 0.01 * DEFAULT_IMAGE_SCALE*texture->height
        };

        BeginShaderMode(shader);
        DrawTextEx(font, TextFormat("Find '%s'", (*hidden_province)->value), 
                   pos_find_str, HUD_DEFAULT_FONTSIZE, 0, WHITE);
        DrawTextEx(font, TextFormat("Error counter: %ld", error_counter), 
                   pos_errors_str, HUD_DEFAULT_FONTSIZE, 0, WHITE);
        EndShaderMode();
    } 
   
    static float lifetime_wrong_msg = HUD_LIFETIME; 
    if (draw_wrong_msg) {
        float dt = GetFrameTime();
        lifetime_wrong_msg -= dt;       

        if (lifetime_wrong_msg > 0) {   
            Vector2 pos = CLITERAL(Vector2) {
                rec->ul.x + 0.4 * DEFAULT_IMAGE_SCALE*texture->width,
                rec->ul.y + 0.5 * DEFAULT_IMAGE_SCALE*texture->height 
            };

            BeginShaderMode(shader);
            DrawTextEx(font, "Wrong!", pos, HUD_LARGE_FONTSIZE, 0, RED);
            EndShaderMode();
        } else {
            draw_wrong_msg = false;
            lifetime_wrong_msg = HUD_LIFETIME;
        }
    }
}

void victory(GameState *state, Rec *rec) 
{
    Vector2 pos = CLITERAL(Vector2) {
        rec->ul.x + 0.4 * DEFAULT_IMAGE_SCALE*rec->width,
        rec->ul.y + 0.5 * DEFAULT_IMAGE_SCALE*rec->height 
    };

    BeginShaderMode(shader);
    DrawTextEx(font, "Victory!", pos, HUD_LARGE_FONTSIZE, 0, GREEN);
    EndShaderMode();
}

int main()
{
    srand(time(NULL));
    stbds_rand_seed(time(NULL));

    hmput(PROVINCES, 0x00ffff, "Baja California");
    hmput(PROVINCES, 0x808080, "Baja California Sur"); 
    hmput(PROVINCES, 0x800000, "Sonora");
    hmput(PROVINCES, 0x808000, "Chihuahua");
    hmput(PROVINCES, 0x008000, "Coahuila");
    hmput(PROVINCES, 0x000080, "Nuevo Leon");
    hmput(PROVINCES, 0xff00ff, "Tamaulipas");
    hmput(PROVINCES, 0xff0000, "Sinaloa");
    hmput(PROVINCES, 0xffff00, "Durango");
    hmput(PROVINCES, 0x00ff00, "Zacatecas");
    hmput(PROVINCES, 0x0000ff, "San Luis Potosi");
    hmput(PROVINCES, 0x6bd4bf, "Veracruz");
    hmput(PROVINCES, 0x008080, "Nayarit");
    hmput(PROVINCES, 0xe94f37, "Jalisco");
    hmput(PROVINCES, 0x004040, "Colima");
    hmput(PROVINCES, 0x808040, "Michoacan");
    hmput(PROVINCES, 0x80ffff, "Guerrero");
    hmput(PROVINCES, 0xb04f89, "Oaxaca"); 
    hmput(PROVINCES, 0x2d534e, "Chiapas"); 
    hmput(PROVINCES, 0x9a83bc, "Tabasco"); 
    hmput(PROVINCES, 0x804000, "Puebla"); 
    hmput(PROVINCES, 0xb18e93, "Campeche"); 
    hmput(PROVINCES, 0xd2bead, "Yucatan"); 
    hmput(PROVINCES, 0xf4948b, "Quintana Roo"); 
    hmput(PROVINCES, 0xff0080, "Mexico City"); 
    hmput(PROVINCES, 0xffff80, "Aguascalientes"); 
    hmput(PROVINCES, 0x800080, "Guanajuato"); 
    hmput(PROVINCES, 0x0080ff, "Queretaro"); 
    hmput(PROVINCES, 0x004080, "Hidalgo"); 
    hmput(PROVINCES, 0x00ff80, "State of Mexico"); 
    hmput(PROVINCES, 0x4000ff, "Morelos"); 
    hmput(PROVINCES, 0xff8040, "Tlaxcala"); 
    assert(hmlen(PROVINCES) == 32);

    for (int i = 0; i < hmlen(PROVINCES); ++i) {
        PROVINCES[i].guessed = false;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    
    size_t factor = 80;
    InitWindow(16*factor, 9*factor, "Map quiz");
    SetTargetFPS(60);
    SetExitKey(KEY_Q);

    int fileSize = 0;
    unsigned char* fileData = LoadFileData("resources/Alegreya-Regular.ttf", &fileSize);

    font.baseSize = FONT_SIZE_LOAD;
    font.glyphCount = 95;
    font.glyphs = LoadFontData(fileData, fileSize, FONT_SIZE_LOAD, 0, 95, FONT_SDF);
    Image atlas = GenImageFontAtlas(font.glyphs, &font.recs, 95, FONT_SIZE_LOAD, 4, 0);
    font.texture = LoadTextureFromImage(atlas);
    UnloadImage(atlas);

    UnloadFileData(fileData);
    
    shader = LoadShader(0, TextFormat("resources/shaders/glsl%i/sdf.fs", GLSL_VERSION));
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    RenderTexture2D canvas = LoadRenderTexture(16*factor, 9*factor);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);
    
    const char* color_map_filename = "resources/mexico-colored.png"; 
    const char* black_white_map_filename = "resources/mexico-black-white.png";

    color_image = LoadImage(color_map_filename);
    black_white_image = LoadImage(black_white_map_filename);
    
    printf("(color_image) width = %d, height = %d\n", color_image.width, color_image.height);
    printf("(bw_image) width = %d, height = %d\n", black_white_image.width, black_white_image.height);

    Texture2D map_texture = LoadTextureFromImage(black_white_image);

    Province* hidden_province = select_random_province();
    
    Camera2D camera = {0};
    camera.zoom = 1.0; 

    GameState state = QUIZ;

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_R)) {
            black_white_image = LoadImage(black_white_map_filename);
            map_texture = LoadTextureFromImage(black_white_image);
    
            for (int i = 0; i < hmlen(PROVINCES); ++i) {
                PROVINCES[i].guessed = false;
            }
   
            error_counter = 0; 
            hidden_province = select_random_province();

            state = QUIZ;
        }
            
        if (IsKeyDown(KEY_S)) {
            printf("cam.offset.x: %.5lf; cam.offset.y: %.5lf\n", camera.offset.x, camera.offset.y);
            printf("cam.target.x: %.5lf; cam.target.y: %.5lf\n", camera.target.x, camera.target.y);
            printf("cam.zoom: %.5lf\n", camera.zoom);
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f / camera.zoom);
            camera.target = Vector2Add(camera.target, delta);
        }

        float wheel = GetMouseWheelMove();

        if (wheel != 0) {
            Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
            camera.offset = GetMousePosition();
            camera.zoom += wheel * 0.2f;
            camera.target = mouseWorldPos;
            if (camera.zoom < 0.125f) camera.zoom = 0.125f;
        }

        if (IsWindowResized()) {
            UnloadRenderTexture(canvas);
            canvas = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        }

        BeginTextureMode(canvas); 
        BeginMode2D(camera);

        ClearBackground(GetColor(BACKGROUND_COLOR));

        // fixed texture position on the screen 
        float posx = GetScreenWidth()/2 - DEFAULT_IMAGE_SCALE * map_texture.width/2;
        float posy = GetScreenHeight()/2 - DEFAULT_IMAGE_SCALE * map_texture.height/2;
        DrawTextureEx(map_texture, CLITERAL(Vector2){posx, posy}, 0.0, DEFAULT_IMAGE_SCALE, WHITE);

        Rec rec = (Rec) {
            .ul = CLITERAL(Vector2) {posx, posy},
            .lr = CLITERAL(Vector2) { 
                posx + DEFAULT_IMAGE_SCALE * map_texture.width,
                posy + DEFAULT_IMAGE_SCALE * map_texture.height
            },
            .width = map_texture.width,
            .height = map_texture.height
        }; 
 
        switch (state) 
        {
            case QUIZ: {
                quiz(&state, &rec, &map_texture, &camera, &hidden_province);
                break;
            }

            case VICTORY: {
                victory(&state, &rec);
                break;
            }

            case LEARN: {
                assert(false);
                break;
            }
        }

        EndMode2D();
        EndTextureMode();

        BeginDrawing();
        // flip texture 
        DrawTexturePro(canvas.texture,
            CLITERAL(Rectangle){0, 0, canvas.texture.width, -canvas.texture.height },
            CLITERAL(Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()}, 
            CLITERAL(Vector2) {0, 0},
            0, WHITE);
                
        EndDrawing();
    } 

    UnloadFont(font);    
    UnloadTexture(map_texture); 
    UnloadImage(color_image);
    UnloadImage(black_white_image);
    UnloadRenderTexture(canvas);
    UnloadShader(shader);

    CloseWindow();

    return 0;
}
