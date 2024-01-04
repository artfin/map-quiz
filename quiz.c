// TODO: support for utf-8 (brazilian accents)?
// TODO: try cross-platform compilation

#include <stdio.h>
#include <time.h>
#include <limits.h>

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

#define MIN_CAMERA_ZOOM 0.9f
#define MAX_CAMERA_ZOOM 2.5f

#define HUD_LIFETIME 0.7 
#define HUD_DEFAULT_FONTSIZE 50
#define HUD_LARGE_FONTSIZE 90 
#define DEFAULT_IMAGE_SCALE 0.5

#define COLOR_TEXT_DEFAULT GetColor(0xDF2935FF)

#define PANEL_WIDTH 0.15
#define COLOR_PANEL_BUTTON           GetColor(0xAD343EFF) 
#define COLOR_PANEL_BUTTON_SELECTED  GetColor(0xF2AF29FF)
#define COLOR_PANEL_BUTTON_HOVEROVER ColorBrightness(COLOR_PANEL_BUTTON, 0.2)

#define COLOR_BACKGROUND GetColor(0x181818FF)
#define COLOR_COUNTRIES_PANEL_BACKGROUND GetColor(0xDDDBCBFF) 
#define COLOR_CORRECT_PROVINCE GetColor(0xE9E9E9FF)
#define COLOR_LEARN_PROVINCE GetColor(0xE9E9E9FF)
#define COLOR_VICTORY ColorBrightness(GetColor(0x7DD181FF), -0.3)

// -------------------------------------------------------------------------------------------
// Copyright 2023 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// Initial capacity of a dynamic array
#define DA_INIT_CAP 256

// Append an item to a dynamic array
#define da_append(da, item)                                                              \
    do {                                                                                 \
        if ((da)->count >= (da)->capacity) {                                             \
            (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2;   \
            (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Buy more RAM lol");                       \
        }                                                                                \
                                                                                         \
        (da)->items[(da)->count++] = (item);                                             \
    } while (0)
// -------------------------------------------------------------------------------------------

unsigned long rgb_to_hex(int r, int g, int b) {   
    return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}

typedef struct {
    unsigned long key;
    char* value;
    Vector2 center;
    bool guessed;
} Province;
    
Province *PROVINCES = NULL;

bool any_provinces_left_to_guess()
{
    bool any_not_guessed = false;
    Province *p;

    for (int i = 0; i < hmlen(PROVINCES); ++i) {
        p = &PROVINCES[i];
        if (!p->guessed) {
            any_not_guessed = true;
        }
    }

    return !any_not_guessed;
}

Province* select_random_province()
{
    bool found = false;
    Province *p;

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

typedef enum {
    MAP_MEXICO = 0,
    MAP_BRAZIL,
    MAP_JAPAN,
} ActiveMap;

Font font = {0};
Shader shader = {0};
Texture2D map_texture = {0};
Camera2D camera = {0};
GameState state = QUIZ;
ActiveMap active_map = MAP_MEXICO; // counter in the COUNTRIES array 

typedef struct {
    const char *name;

    const char* color_map_filename;
    Image color_map;

    const char* bw_map_filename;
    Image bw_map; 
} Country;

typedef struct {
    Country *items;
    size_t count;
    size_t capacity;
} Countries;

Countries COUNTRIES = {0};

void fill_provinces(int country_counter) {
    hmfree(PROVINCES);

    switch (country_counter) {
        case MAP_MEXICO: {
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
            break;
        };

        case MAP_BRAZIL: {
            hmput(PROVINCES, 0x808080, "Acre");
            hmput(PROVINCES, 0x008000, "Rondonia");
            hmput(PROVINCES, 0x800000, "Amazonas");
            hmput(PROVINCES, 0xff0000, "Roraima");
            hmput(PROVINCES, 0x808000, "Para");
            hmput(PROVINCES, 0xffff00, "Amapa");
            hmput(PROVINCES, 0x00ff00, "Mato Grosso");
            hmput(PROVINCES, 0xff8040, "Mato Grosso Do Sul");
            hmput(PROVINCES, 0x00ffff, "Maranhao");
            hmput(PROVINCES, 0x008080, "Tocantins");
            hmput(PROVINCES, 0x000080, "Goias");
            hmput(PROVINCES, 0x800080, "Piaui");
            hmput(PROVINCES, 0xff00ff, "Ceara");
            hmput(PROVINCES, 0x808040, "Rio Grande do Norte");
            hmput(PROVINCES, 0xffff80, "Paraiba");
            hmput(PROVINCES, 0x004040, "Pernambuco");
            hmput(PROVINCES, 0x80ffff, "Alagoas");
            hmput(PROVINCES, 0x004080, "Sergipe");
            hmput(PROVINCES, 0x8080ff, "Bahia");
            hmput(PROVINCES, 0x4000ff, "Minas Gerais");
            hmput(PROVINCES, 0x00272b, "Espirito Santo");
            hmput(PROVINCES, 0xff665b, "Rio de Janeiro");
            hmput(PROVINCES, 0x804000, "Sao Paulo");
            hmput(PROVINCES, 0xd5c619, "Parana");
            hmput(PROVINCES, 0x192a51, "Santa Catarina");
            hmput(PROVINCES, 0xe3dc95, "Rio Grande do Sul");
            hmput(PROVINCES, 0xff0080, "Federal District");
            assert(hmlen(PROVINCES) == 27);
            break;
        }

        case MAP_JAPAN: {
            hmput(PROVINCES, 0xed1c24, "Hokkaido"); 
            hmput(PROVINCES, 0xff7f27, "Aomori"); 
            hmput(PROVINCES, 0x22b14c, "Iwate"); 
            hmput(PROVINCES, 0xfff200, "Akita"); 
            hmput(PROVINCES, 0xa349a4, "Miyagi"); 
            hmput(PROVINCES, 0x3f48cc, "Yamagata"); 
            hmput(PROVINCES, 0x00ff00, "Fukushima"); 
            hmput(PROVINCES, 0xffc90e, "Ibaraki"); 
            hmput(PROVINCES, 0xb5e61d, "Tochigi"); 
            hmput(PROVINCES, 0x99d9ea, "Gunma"); 
            hmput(PROVINCES, 0x8000ff, "Saitama"); 
            hmput(PROVINCES, 0xff00ff, "Chiba");
            hmput(PROVINCES, 0xff0080, "Tokyo"); 
            hmput(PROVINCES, 0x808000, "Kanagawa");
            hmput(PROVINCES, 0xffaec9, "Niigata"); 
            hmput(PROVINCES, 0xc8bfe7, "Toyama");
            hmput(PROVINCES, 0x312893, "Ishikawa"); 
            hmput(PROVINCES, 0x4ffdfd, "Fukui");
            hmput(PROVINCES, 0x4bc6d3, "Yamanashi"); 
            hmput(PROVINCES, 0x7092be, "Nagano");
            hmput(PROVINCES, 0xfb717b, "Gifu"); 
            hmput(PROVINCES, 0x4f803c, "Shizuoka");
            hmput(PROVINCES, 0x9a7dee, "Aichi"); 
            hmput(PROVINCES, 0x9ad1a2, "Mie"); 
            hmput(PROVINCES, 0xf0e65b, "Shiga"); 
            hmput(PROVINCES, 0x05e437, "Kyoto");
            hmput(PROVINCES, 0xa06849, "Osaka"); 
            hmput(PROVINCES, 0x8dd812, "Hyogo");
            hmput(PROVINCES, 0xc487c5, "Nara"); 
            hmput(PROVINCES, 0xd713d1, "Wakayama"); 
            hmput(PROVINCES, 0xb5d2b3, "Tottori"); 
            hmput(PROVINCES, 0xfafa8b, "Shimane");
            hmput(PROVINCES, 0xf7948e, "Okayama"); 
            hmput(PROVINCES, 0xdbaab8, "Hiroshima");
            hmput(PROVINCES, 0xd89ee7, "Yamaguchi"); 
            hmput(PROVINCES, 0xdec6a7, "Tokushima"); 
            hmput(PROVINCES, 0x88cefd, "Kagawa"); 
            hmput(PROVINCES, 0x97ee9d, "Ehime");
            hmput(PROVINCES, 0xadaed8, "Kochi"); 
            hmput(PROVINCES, 0xc4bacb, "Fukuoka");
            hmput(PROVINCES, 0x98edc9, "Saga"); 
            hmput(PROVINCES, 0x91fefc, "Nagasaki"); 
            hmput(PROVINCES, 0xbf9ee7, "Kumamoto"); 
            hmput(PROVINCES, 0xf88dad, "Oita");
            hmput(PROVINCES, 0xabdad1, "Miyazaki"); 
            hmput(PROVINCES, 0xfab88b, "Kagoshima");
            hmput(PROVINCES, 0x0000ff, "Okinawa");
            assert(hmlen(PROVINCES) == 47);
            break;
        }
    } 

    for (int i = 0; i < hmlen(PROVINCES); ++i) {
        PROVINCES[i].guessed = false;
    }
}

typedef struct {
    Vector2 ul; // upper-left 
    Vector2 lr; // lower-right
    float width;
    float height;
} Rec;
    
static size_t error_counter = 0;

void reset_provinces() {
    for (int i = 0; i < hmlen(PROVINCES); ++i) {
        PROVINCES[i].guessed = false;
    }
}

Rectangle project_rectangle(Rectangle r_abs)
{
    Vector3 ul = CLITERAL(Vector3) { r_abs.x, r_abs.y, 0.0 };
    Vector3 lr = CLITERAL(Vector3) { r_abs.x + r_abs.width, r_abs.y + r_abs.height, 0.0 };
    Matrix invMatCamera = MatrixInvert(GetCameraMatrix2D(camera));
    Vector3 ul_t = Vector3Transform(ul, invMatCamera);
    Vector3 lr_t = Vector3Transform(lr, invMatCamera);

    return CLITERAL(Rectangle){ul_t.x, ul_t.y, lr_t.x - ul_t.x, lr_t.y - ul_t.y }; 
}

void quiz(Rec *rec, Province **hidden_province)
{
    static bool draw_wrong_msg = false;
    Province *p;

    if (any_provinces_left_to_guess()) {
        state = VICTORY;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (GetMouseX() < GetScreenWidth() * PANEL_WIDTH) goto skip_if;

        Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);

        printf("mouse.x: %.5lf; mouse.y: %.5lf\n", mouse.x, mouse.y);
        DrawCircle(mouse.x, mouse.y, 10.0, RED);

        printf("ul_corner: (%.5lf, %.5lf); lr_corner: (%.5lf, %.5lf)\n", rec->ul.x, rec->ul.y, rec->lr.x, rec->lr.y);

        if ( (mouse.x < rec->ul.x) || (mouse.x > rec->lr.x) || (mouse.y < rec->ul.y) || (mouse.y > rec->lr.y)) {
            printf("Click is outside the image!\n");
        } else {
            int imgx = (int) ( (mouse.x - (GetScreenWidth()/2 - DEFAULT_IMAGE_SCALE*map_texture.width/2)) / DEFAULT_IMAGE_SCALE);
            int imgy = (int) ( (mouse.y - (GetScreenHeight()/2 - DEFAULT_IMAGE_SCALE*map_texture.height/2)) / DEFAULT_IMAGE_SCALE);

            Image color_map = COUNTRIES.items[active_map].color_map;
            Image bw_map    = COUNTRIES.items[active_map].bw_map;

            Color c = GetImageColor(color_map, imgx, imgy);
            unsigned long hex = rgb_to_hex(c.r, c.g, c.b);

            printf("Click is inside! imgx = %d; imgy = %d; color: (%d, %d, %d, %d) => #%06lx\n", 
                    imgx, imgy, c.r, c.g, c.b, c.a, hex);

            int i = hmgeti(PROVINCES, hex);
            if (i != -1) {
                p = &PROVINCES[i];

                if (strcmp(p->value, (*hidden_province)->value) == 0) {
                    //printf("Province name = %s\n", province_name);

                    for (int px = 0; px < color_map.width; ++px) {
                        for (int py = 0; py < color_map.height; ++py) {
                            Color current_color = GetImageColor(color_map, px, py);
                            if ((current_color.r == c.r) && 
                                    (current_color.g == c.g) && 
                                    (current_color.b == c.b) && 
                                    (current_color.a == c.a)) {
                                ImageDrawPixel(&bw_map, px, py, COLOR_CORRECT_PROVINCE);
                            }
                        }
                    } 

                    UnloadTexture(map_texture);
                    map_texture = LoadTextureFromImage(bw_map);

                    (*hidden_province)->guessed = true;
                    if (any_provinces_left_to_guess()) {
                        state = VICTORY;
                        return;
                    }

                    *hidden_province = select_random_province();

                } else {
                    error_counter += 1;
                    draw_wrong_msg = true;
                }
            } else {
                printf("Province name unknown! Possibly a border has been clicked. \n\n");
            }
        }
    }

skip_if:
    Rectangle status_bar = project_rectangle(CLITERAL(Rectangle) {
        .x = GetScreenWidth() * PANEL_WIDTH,
        .y = 0.0,
        .width = GetScreenWidth() * (1.0 - PANEL_WIDTH),
        .height = 65.0});

    DrawRectangleRec(status_bar, ColorBrightness(COLOR_BACKGROUND, 0.2));

    float padding = 0.01 * GetScreenHeight();
    Vector2 pos_find_str = GetScreenToWorld2D(CLITERAL(Vector2) {PANEL_WIDTH*GetScreenWidth() + 3*padding, padding}, camera);

    Vector2 pos_errors_str = GetScreenToWorld2D(CLITERAL(Vector2) {
            GetScreenWidth() - 270.0, padding 
            }, camera);

    BeginShaderMode(shader);
    DrawTextEx(font, TextFormat("Find '%s'", (*hidden_province)->value), 
            pos_find_str, HUD_DEFAULT_FONTSIZE / camera.zoom, 0, COLOR_TEXT_DEFAULT);
    DrawTextEx(font, TextFormat("Error counter: %ld", error_counter), 
            pos_errors_str, HUD_DEFAULT_FONTSIZE / camera.zoom, 0, COLOR_TEXT_DEFAULT);
    EndShaderMode();
   
    static float lifetime_wrong_msg = HUD_LIFETIME; 
    if (draw_wrong_msg) {
        float dt = GetFrameTime();
        lifetime_wrong_msg -= dt;       

        if (lifetime_wrong_msg > 0) {   
            Vector2 pos = CLITERAL(Vector2) {
                rec->ul.x + 0.4 * DEFAULT_IMAGE_SCALE*map_texture.width,
                rec->ul.y + 0.5 * DEFAULT_IMAGE_SCALE*map_texture.height 
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

void victory(Rec *rec) 
{
    Rectangle status_bar = project_rectangle(CLITERAL(Rectangle) {
        .x = GetScreenWidth() * PANEL_WIDTH,
        .y = 0.0,
        .width = GetScreenWidth() * (1.0 - PANEL_WIDTH),
        .height = 65.0});

    DrawRectangleRec(status_bar, ColorBrightness(COLOR_BACKGROUND, 0.2));

    float padding = 0.01 * GetScreenHeight();

    BeginShaderMode(shader);
    DrawTextEx(font, TextFormat("Error counter: %ld", error_counter), GetScreenToWorld2D(CLITERAL(Vector2) {
        GetScreenWidth() - 270.0, padding}, camera),
        HUD_DEFAULT_FONTSIZE / camera.zoom, 0, COLOR_TEXT_DEFAULT);
    EndShaderMode();

    Vector2 victory_text_pos = GetScreenToWorld2D(CLITERAL(Vector2) {GetScreenWidth()/2, GetScreenHeight()/2}, camera);

    BeginShaderMode(shader);
    DrawTextEx(font, "Victory!", victory_text_pos, HUD_LARGE_FONTSIZE / camera.zoom, 0, COLOR_VICTORY);
    EndShaderMode();
}

void learn(Rec *rec)
{
    const char* text = "Learn provinces"; 
    Vector2 text_len = MeasureTextEx(font, text, HUD_DEFAULT_FONTSIZE, 0);
    
    Vector2 title_pos = CLITERAL(Vector2) {
        rec->ul.x + 0.5 * DEFAULT_IMAGE_SCALE*rec->width - 0.5*text_len.x, 
        rec->ul.y + 0.01 * DEFAULT_IMAGE_SCALE*rec->height
    };

    BeginShaderMode(shader);
    DrawTextEx(font, text, title_pos, HUD_DEFAULT_FONTSIZE, 0, WHITE);
    EndShaderMode();
   
    static bool show_warning_msg = false;
    static bool show_province_name = false;
    
    static Province *p = NULL;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);

        printf("mouse.x: %.5lf; mouse.y: %.5lf\n", mouse.x, mouse.y);
        DrawCircle(mouse.x, mouse.y, 10.0, RED);

        printf("ul_corner: (%.5lf, %.5lf); lr_corner: (%.5lf, %.5lf)\n", rec->ul.x, rec->ul.y, rec->lr.x, rec->lr.y);

        if ( (mouse.x < rec->ul.x) || (mouse.x > rec->lr.x) || (mouse.y < rec->ul.y) || (mouse.y > rec->lr.y)) {
            printf("Click is outside the image!\n");
        } else {
            int imgx = (int) ( (mouse.x - (GetScreenWidth()/2 - DEFAULT_IMAGE_SCALE*rec->width/2)) / DEFAULT_IMAGE_SCALE);
            int imgy = (int) ( (mouse.y - (GetScreenHeight()/2 - DEFAULT_IMAGE_SCALE*rec->height/2)) / DEFAULT_IMAGE_SCALE);

            Image color_map = COUNTRIES.items[active_map].color_map;
            Image bw_map    = COUNTRIES.items[active_map].bw_map;

            Color c = GetImageColor(color_map, imgx, imgy);
            unsigned long hex = rgb_to_hex(c.r, c.g, c.b);

            printf("Click is inside! imgx = %d; imgy = %d; color: (%d, %d, %d, %d) => #%06lx\n", 
                    imgx, imgy, c.r, c.g, c.b, c.a, hex);

            int i = hmgeti(PROVINCES, hex);

            if (i != -1) {
                p = &PROVINCES[i];

                int min_x = INT_MAX;
                int max_x = INT_MIN;
                int min_y = INT_MAX;
                int max_y = INT_MIN;

                for (int px = 0; px < color_map.width; ++px) {
                    for (int py = 0; py < color_map.height; ++py) {
                        Color current_color = GetImageColor(color_map, px, py);

                        if ((current_color.r == c.r) && 
                            (current_color.g == c.g) && 
                            (current_color.b == c.b) && 
                            (current_color.a == c.a)) {

                            ImageDrawPixel(&bw_map, px, py, COLOR_LEARN_PROVINCE);
                            if (px > max_x) max_x = px;
                            if (px < min_x) min_x = px;
                            if (py > max_y) max_y = py;
                            if (py < min_y) min_y = py;
                        }
                    }
                }
                
                UnloadTexture(map_texture);
                map_texture = LoadTextureFromImage(bw_map);

                p->center = CLITERAL(Vector2) {
                    0.5 * (min_x + max_x) * DEFAULT_IMAGE_SCALE,
                    0.5 * (min_y + max_y) * DEFAULT_IMAGE_SCALE
                };

                show_province_name = true;

                printf("min_x: %d; max_x: %d\n", min_x, max_x);
                printf("min_y: %d; max_y: %d\n", min_y, max_y);
                
            } else {
                show_warning_msg = true;
            }
        }
    }

    static float warning_msg_lifetime = HUD_LIFETIME;

    if (show_warning_msg) {
        warning_msg_lifetime -= GetFrameTime();

        if (warning_msg_lifetime > 0) {
            const char* text = "Click a province"; 
            Vector2 text_len = MeasureTextEx(font, text, HUD_LARGE_FONTSIZE, 0);

            Vector2 title_pos = CLITERAL(Vector2) {
                rec->ul.x + 0.5 * DEFAULT_IMAGE_SCALE*rec->width - 0.5*text_len.x, 
                rec->ul.y + 0.5 * DEFAULT_IMAGE_SCALE*rec->height - 0.5*text_len.y
            };

            BeginShaderMode(shader);
            DrawTextEx(font, text, title_pos, HUD_LARGE_FONTSIZE, 0, RED);
            EndShaderMode();
        } else {
            warning_msg_lifetime = HUD_LIFETIME;
            show_warning_msg = false;
        }
    }

    static float province_name_lifetime = HUD_LIFETIME;

    if (show_province_name) {
        province_name_lifetime -= GetFrameTime();

        if (province_name_lifetime > 0) {
            BeginShaderMode(shader);
            DrawTextEx(font, TextFormat("%s", p->value), p->center, 60, 0, RED);
            EndShaderMode();
        } else {
            province_name_lifetime = HUD_LIFETIME;
            show_province_name = false;
        }
    }
}

typedef enum {
    BS_NONE      = 0, // 00
    BS_HOVEROVER = 1, // 01
    BS_CLICKED   = 2, // 10
} Button_State;

int button(Rectangle boundary) 
{
    Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);
    int hoverover = CheckCollisionPointRec(mouse, boundary);
    int clicked = 0;

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (hoverover) clicked = 1;
    }

    return (clicked << 1) | hoverover;
}

void panel_countries(Rectangle panel_boundary, Province **hidden_province)
{
    DrawRectangleRec(project_rectangle(panel_boundary), COLOR_COUNTRIES_PANEL_BACKGROUND);

    //float scroll_bar_width = 0.03 * panel_boundary.width;

    float panel_padding = 0.03 * panel_boundary.width;
    float entry_size = 80.0;

    for (size_t i = 0; i < COUNTRIES.count; ++i) {
        Country *c = &COUNTRIES.items[i];

        Rectangle menu_entry = project_rectangle(
            CLITERAL(Rectangle) {
                .x = panel_boundary.x + panel_padding,
                .y = panel_boundary.y + panel_padding + i * entry_size,
                .width = panel_boundary.width - panel_padding * 2,
                .height = entry_size - panel_padding * 2});
        
        Color color;
        if ((int) i != active_map) {
            int button_state = button(menu_entry);
            if (button_state & BS_HOVEROVER) {
                color = COLOR_PANEL_BUTTON_HOVEROVER;
            } else {
                color = COLOR_PANEL_BUTTON;
            }

            if (button_state & BS_CLICKED) {
                Country *country = &COUNTRIES.items[active_map];
                country->bw_map = LoadImage(country->bw_map_filename);
                map_texture = LoadTextureFromImage(country->bw_map);

                active_map = i;
                camera.zoom = 1.0;

                map_texture = LoadTextureFromImage(COUNTRIES.items[i].bw_map);
            
                fill_provinces(i);

                *hidden_province = select_random_province();

                error_counter = 0; 

                state = QUIZ; 
            }
        } else {
            color = COLOR_PANEL_BUTTON_SELECTED;
        }

        DrawRectangleRounded(menu_entry, 0.5, 10, color);
        float fontsize = 50 / camera.zoom;
        Vector2 name_len = MeasureTextEx(font, c->name, fontsize, 0);
    
        Vector2 name_pos = CLITERAL(Vector2) {
            menu_entry.x + 0.5 * menu_entry.width - 0.5 * name_len.x, 
            menu_entry.y + 0.5 * menu_entry.height - 0.5 * name_len.y
        };

        BeginShaderMode(shader);
        DrawTextEx(font, c->name, name_pos, fontsize, 0, WHITE);
        EndShaderMode();
    }
}

int main()
{
    srand(time(NULL));
    stbds_rand_seed(time(NULL));

    Country country_item            = {0};
    country_item.name               = "Mexico";
    country_item.color_map_filename = "resources/mexico-colored.png";
    country_item.bw_map_filename    = "resources/mexico-black-white.png";
    country_item.color_map          = LoadImage(country_item.color_map_filename);
    country_item.bw_map             = LoadImage(country_item.bw_map_filename);
    assert((country_item.color_map.width == country_item.bw_map.width) && 
           (country_item.color_map.height == country_item.bw_map.height));
    da_append(&COUNTRIES, country_item);

    country_item.name               = "Brazil";
    country_item.color_map_filename = "resources/brazil-colored.png";
    country_item.bw_map_filename    = "resources/brazil-black-white.png";
    country_item.color_map          = LoadImage(country_item.color_map_filename);
    country_item.bw_map             = LoadImage(country_item.bw_map_filename);
    assert((country_item.color_map.width == country_item.bw_map.width) && 
           (country_item.color_map.height == country_item.bw_map.height));
    da_append(&COUNTRIES, country_item);

    country_item.name               = "Japan";
    country_item.color_map_filename = "resources/japan-colored.png";
    country_item.bw_map_filename    = "resources/japan-black-white.png";
    country_item.color_map          = LoadImage(country_item.color_map_filename);
    country_item.bw_map             = LoadImage(country_item.bw_map_filename);
    assert((country_item.color_map.width == country_item.bw_map.width) && 
           (country_item.color_map.height == country_item.bw_map.height));
    da_append(&COUNTRIES, country_item);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    camera.zoom = 1.0; 
    
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

    fill_provinces(active_map);
    map_texture = LoadTextureFromImage(COUNTRIES.items[active_map].bw_map);
    SetTextureFilter(map_texture, TEXTURE_FILTER_BILINEAR);

    Province* hidden_province = select_random_province();
    

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_R)) {
            Country *country = &COUNTRIES.items[active_map];
            country->bw_map = LoadImage(country->bw_map_filename);
            map_texture = LoadTextureFromImage(country->bw_map);
    
            reset_provinces();

            hidden_province = select_random_province();
            error_counter = 0; 

            state = QUIZ;
        }
           
        if (IsKeyDown(KEY_L)) {
            Country *country = &COUNTRIES.items[active_map];
            country->bw_map = LoadImage(country->bw_map_filename);
            map_texture = LoadTextureFromImage(country->bw_map);

            reset_provinces();
            error_counter = 0;
            hidden_province = NULL;
        
            state = LEARN;
        }

        if (IsKeyDown(KEY_S)) {
            printf("cam.offset.x: %.5lf; cam.offset.y: %.5lf\n", camera.offset.x, camera.offset.y);
            printf("cam.target.x: %.5lf; cam.target.y: %.5lf\n", camera.target.x, camera.target.y);
            printf("cam.zoom: %.5lf\n", camera.zoom);
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f / camera.zoom);
            
            //Vector2 new_camera_target = Vector2Add(camera.target, delta);
            //printf("new_camera_target: %.5lf, %.5lf\n", new_camera_target.x, new_camera_target.y);
            
            camera.target = Vector2Add(camera.target, delta);
            //if (new_camera_target.x > 800.0) camera.target.x = 800.0; 
        }

        float wheel = GetMouseWheelMove();

        if (wheel != 0) {
            Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
            camera.offset = GetMousePosition();
            camera.zoom += wheel * 0.2f;
            camera.target = mouseWorldPos;
            if (camera.zoom < MIN_CAMERA_ZOOM) camera.zoom = MIN_CAMERA_ZOOM; 
            if (camera.zoom > MAX_CAMERA_ZOOM) camera.zoom = MAX_CAMERA_ZOOM; 
        }

        if (IsWindowResized()) {
            UnloadRenderTexture(canvas);
            canvas = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        }

        BeginTextureMode(canvas); 
        BeginMode2D(camera);

        ClearBackground(COLOR_BACKGROUND);

        float posx = GetScreenWidth()/2 - DEFAULT_IMAGE_SCALE * map_texture.width/2;
        float posy = GetScreenHeight()/2 - DEFAULT_IMAGE_SCALE * map_texture.height/2;
        DrawTextureEx(map_texture, CLITERAL(Vector2){posx, posy}, 0.0, DEFAULT_IMAGE_SCALE, WHITE);
            
        /*
        Rectangle map_rectangle = CLITERAL(Rectangle) {
            .x = PANEL_WIDTH, 
            .y = 0.0, 
            .width = GetScreenWidth() - PANEL_WIDTH,
            .height = GetScreenHeight()
        };
 
        DrawTexturePro(map_texture, CLITERAL(Rectangle) { 0.0f, 0.0f, map_texture.width, map_texture.height }, 
            CLITERAL(Rectangle) { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f, map_texture.width, map_texture.height }, 
            CLITERAL(Vector2) {map_texture.width / 2, map_texture.height / 2}, 0.0f, WHITE);
        */

        panel_countries(CLITERAL(Rectangle) {
            .x = 0, 
            .y = 0,
            .width = PANEL_WIDTH * GetScreenWidth(),
            .height = GetScreenHeight()
        }, &hidden_province); 
        //printf("main loop state = %d\n", state);
        
        Rec rec = (Rec) {
            .ul = CLITERAL(Vector2) {posx, posy},
            .lr = CLITERAL(Vector2) { 
                posx + DEFAULT_IMAGE_SCALE * map_texture.width,
                posy + DEFAULT_IMAGE_SCALE * map_texture.height
            },
            .width = map_texture.width,
            .height = map_texture.height
        }; 

        switch (state) {
            case QUIZ: {
                quiz(&rec, &hidden_province);
                break;
            }

            case LEARN: {
                learn(&rec); 
                break;
            }
            
            case VICTORY: {
                victory(&rec);
                break;
            }

            default: {
                assert(false);
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
    UnloadRenderTexture(canvas);
    UnloadShader(shader);

    // TODO: unload COUNTRIES

    CloseWindow();

    return 0;
}
