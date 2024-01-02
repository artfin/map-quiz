#include <stdio.h>
#include <time.h>

// implementation added to raylib.h
#include "stb_image.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include "raylib.h"
#include "raymath.h"

#define FONT_SIZE 64

#define HUD_LIFETIME 1.0
#define IMAGE_SCALE 0.5

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

    int screen_width = 16*80; 
    int screen_height = 9*80;
    InitWindow(screen_width, screen_height, "Map quiz");
    SetTargetFPS(60);

    Font font = LoadFontEx("./resources/Alegreya-Regular.ttf", FONT_SIZE, NULL, 0);
    GenTextureMipmaps(&font.texture);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    
    const char* color_map_filename = "resources/mexico-colored.png"; 
    const char* black_white_map_filename = "resources/mexico-black-white.png";

    Image color_image = LoadImage(color_map_filename);
    Image black_white_image = LoadImage(black_white_map_filename);
    
    printf("(color_image) width = %d, height = %d\n", color_image.width, color_image.height);
    printf("(bw_image) width = %d, height = %d\n", black_white_image.width, black_white_image.height);

    Texture2D texture = LoadTextureFromImage(black_white_image);

    Province* hidden_province = select_random_province();
    
    const size_t BUFFER_SIZE = 100;
    char buffer[BUFFER_SIZE];

    float lifetime_wrong_msg = HUD_LIFETIME; 
    bool draw_wrong_msg = false;
    bool draw_victory_msg = false;

    size_t error_counter = 0;

    Camera2D cam = {0};
    cam.zoom = 1.0; //IMAGE_SCALE;

    while (!WindowShouldClose()) {

        if (IsKeyDown(KEY_Q)) {
            UnloadTexture(texture); 
            UnloadImage(color_image);
            UnloadImage(black_white_image);
            CloseWindow();
            break;
        }

        if (IsKeyDown(KEY_S)) {
            printf("cam.target.x: %.5lf; cam.target.y: %.5lf\n", cam.target.x, cam.target.y);
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f / cam.zoom);
            cam.target = Vector2Add(cam.target, delta);
        }

        /*
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), cam);
            cam.offset = GetMousePosition();
            cam.zoom += wheel * 0.125f;
            cam.target = mouseWorldPos;
            if (cam.zoom < 0.125f) cam.zoom = 0.125f; 
        }
        */

        screen_width = GetScreenWidth();
        screen_height = GetScreenHeight();

        BeginDrawing(); 
        BeginMode2D(cam);

        ClearBackground(GetColor(BACKGROUND_COLOR));
       
        // fixed texture position on the screen 
        float posx = screen_width/2 - IMAGE_SCALE * texture.width/2;
        float posy = screen_height/2 - IMAGE_SCALE * texture.height/2;
        DrawTextureEx(texture, CLITERAL(Vector2){posx, posy}, 0.0, IMAGE_SCALE, WHITE);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = Vector2Add(GetMousePosition(), cam.target);

            if ((mouse.x < screen_width/2 - IMAGE_SCALE*texture.width/2)   || 
                (mouse.x > screen_width/2 + IMAGE_SCALE*texture.width/2)   || 
                (mouse.y < screen_height/2 - IMAGE_SCALE*texture.height/2) || 
                (mouse.y > screen_height/2 + IMAGE_SCALE*texture.height/2)) 
            {
                printf("Click is outside the image!\n");
            } 
            else {
                int x = (int) ( (mouse.x - (screen_width/2 - IMAGE_SCALE*texture.width/2)) /  IMAGE_SCALE);
                int y = (int) ( (mouse.y - (screen_height/2 - IMAGE_SCALE*texture.height/2)) / IMAGE_SCALE);
                Color c = GetImageColor(color_image, x, y);
                unsigned long hex = rgb_to_hex(c.r, c.g, c.b);
                
                printf("mousex = %.5lf; mousey = %.5lf; x = %d; y = %d; color: (%d, %d, %d, %d) => #%06lx\n", 
                        mouse.x, mouse.y, x, y, c.r, c.g, c.b, c.a, hex);

                char* clicked_province = hmget(PROVINCES, hex);
                if (clicked_province != NULL) {
                    if (strcmp(clicked_province, hidden_province->value) == 0) {
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

                        UnloadTexture(texture);
                        texture = LoadTextureFromImage(black_white_image);

                        hidden_province->guessed = true;
                        hidden_province = select_random_province();

                        if (hidden_province == NULL) {
                            draw_victory_msg = true;
                        }

                    } else {
                        error_counter += 1;
                        draw_wrong_msg = true;
                    }
                } else {
                    printf("Province name unknown! Possibly a border has been clicked. \n");
                }
            }
        }

        if (hidden_province != NULL) {
            memset(buffer, 0, BUFFER_SIZE);

            int n = snprintf(buffer, BUFFER_SIZE, "Find '%s'", hidden_province->value);
            assert(n == 7 + strlen(hidden_province->value));

            DrawTextEx(font, buffer, CLITERAL(Vector2){0.2*screen_width, 0.1*screen_height}, 40, 0, WHITE);
            
            memset(buffer, 0, BUFFER_SIZE);
            n = snprintf(buffer, BUFFER_SIZE, "Error counter: %ld", error_counter);
            // TODO: error check `snprintf`

            DrawTextEx(font, buffer, CLITERAL(Vector2){0.65*screen_width, 0.1*screen_height}, 40, 0, WHITE);
        } 

        if (draw_wrong_msg) {
            float dt = GetFrameTime();
            lifetime_wrong_msg -= dt;       

            if (lifetime_wrong_msg > 0) {   
                DrawTextEx(font, "Wrong!", CLITERAL(Vector2){screen_width/2, screen_height/2}, 75, 0, RED);
            } else {
                draw_wrong_msg = false;
                lifetime_wrong_msg = HUD_LIFETIME;
            }
        }

        if (draw_victory_msg) {
            DrawTextEx(font, "Victory!", CLITERAL(Vector2){screen_width/2, screen_height/2}, 75, 0, GREEN);
        }

        EndMode2D();
        EndDrawing();
    } 

    return 0;
}