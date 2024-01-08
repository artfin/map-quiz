// TODO: support for utf-8 (brazilian accents)?
// TODO: try cross-platform compilation

#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <stdint.h>

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
                    
#define DEBUG_SAVE_MAP_TO_PNG    
#undef DEBUG_SAVE_MAP_TO_PNG    

#define FONT_SIZE_LOAD 160 

#define MIN_CAMERA_ZOOM 0.6f
#define MAX_CAMERA_ZOOM 3.0f

#define HUD_LIFETIME 0.7 
#define HUD_DEFAULT_FONTSIZE 50
#define HUD_LARGE_FONTSIZE 90 
#define DEFAULT_IMAGE_SCALE 0.5

#define COLOR_TEXT_DEFAULT GetColor(0xDF2935FF)

#define COUNTRIES_PANEL_WIDTH  0.15
#define COUNTRIES_PANEL_HEIGHT 0.86

#define COLOR_PANEL_BUTTON           GetColor(0xAD343EFF) 
#define COLOR_PANEL_BUTTON_SELECTED  GetColor(0xF2AF29FF)
#define COLOR_PANEL_BUTTON_HOVEROVER ColorBrightness(COLOR_PANEL_BUTTON, 0.2)

#define MAX_ERRORS_CURRENT_ROUND 3

#define COLOR_BACKGROUND                 GetColor(0x181818FF)
#define COLOR_COUNTRIES_PANEL_BACKGROUND GetColor(0xDDDBCBFF) 
#define COLOR_CONTROL_PANEL_BACKGROUND   GetColor(0xDDDBCBFF)
#define COLOR_GUESSED_PERFECT_PROVINCE   GetColor(0xE9E9E9FF)
#define COLOR_GUESSED_WERRORS_PROVINCE   GetColor(0xFFF370FF)
#define COLOR_INCORRECT_PROVINCE         GetColor(0xB52A2AFF) 
#define COLOR_LEARN_PROVINCE             COLOR_GUESSED_PERFECT_PROVINCE 
#define COLOR_VICTORY                    ColorBrightness(GetColor(0x7DD181FF), -0.3)

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
        if (!p->guessed) any_not_guessed = true;
    }

    return any_not_guessed;
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
    MAP_PHILLIPINES_ISLANDS,
    MAP_MALAYSIA,
} ActiveMap;

Font font = {0};
Shader shader = {0};
Texture2D map_texture = {0};
Camera2D camera = {0};
GameState state = QUIZ;
ActiveMap active_map = MAP_MEXICO; // counter in the COUNTRIES array 

typedef struct {
    char *name;
    char *display_name;

    char* color_map_filename;
    Image color_map;

    char* bw_map_filename;
    Image bw_map; 
} Country;

typedef struct {
    Country *items;
    size_t count;
    size_t capacity;
} Countries;

Countries COUNTRIES = {0};

Country* load_country(const char* country_name, const char* display_name)
{
    // TODO: implement arena allocator to hold all these random strings
    //       instead of malloc-ing
    Country country_item = {0};
   
    country_item.name = malloc(TextLength(country_name) + 1);
    TextCopy(country_item.name, country_name);

    country_item.display_name = malloc(TextLength(display_name) + 1);
    TextCopy(country_item.display_name, display_name); 

    const char *tmp;    
    tmp = TextFormat("resources/%s-colored.png", TextToLower(country_item.name));
    country_item.color_map_filename = malloc(TextLength(tmp) + 1);
    TextCopy(country_item.color_map_filename, tmp);

    tmp = TextFormat("resources/%s-black-white.png", TextToLower(country_item.name)); 
    country_item.bw_map_filename = malloc(TextLength(tmp) + 1);
    TextCopy(country_item.bw_map_filename, tmp);

    country_item.color_map = LoadImage(country_item.color_map_filename);
    country_item.bw_map    = LoadImage(country_item.bw_map_filename);
    ImageFormat(&country_item.bw_map, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    assert((country_item.color_map.width   == country_item.bw_map.width) &&
            (country_item.color_map.height == country_item.bw_map.height));
    da_append(&COUNTRIES, country_item);

    return &COUNTRIES.items[COUNTRIES.count - 1];
}

Country* reload_country(size_t i)
{
    assert(i < COUNTRIES.count);

    Country* c = &COUNTRIES.items[i];
    c->bw_map = LoadImage(c->bw_map_filename);
    ImageFormat(&c->bw_map, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    return c;
}

void fill_provinces(int country_counter) {
    hmfree(PROVINCES);

    switch (country_counter) {
        case MAP_MEXICO: {
            hmput(PROVINCES, 0x00ffffff, "Baja California");
            hmput(PROVINCES, 0x808080ff, "Baja California Sur"); 
            hmput(PROVINCES, 0x800000ff, "Sonora");
            hmput(PROVINCES, 0x808000ff, "Chihuahua");
            hmput(PROVINCES, 0x008000ff, "Coahuila");
            hmput(PROVINCES, 0x000080ff, "Nuevo Leon");
            hmput(PROVINCES, 0xff00ffff, "Tamaulipas");
            hmput(PROVINCES, 0xff0000ff, "Sinaloa");
            hmput(PROVINCES, 0xffff00ff, "Durango");
            hmput(PROVINCES, 0x00ff00ff, "Zacatecas");
            hmput(PROVINCES, 0x0000ffff, "San Luis Potosi");
            hmput(PROVINCES, 0x6bd4bfff, "Veracruz");
            hmput(PROVINCES, 0x008080ff, "Nayarit");
            hmput(PROVINCES, 0xe94f37ff, "Jalisco");
            hmput(PROVINCES, 0x004040ff, "Colima");
            hmput(PROVINCES, 0x808040ff, "Michoacan");
            hmput(PROVINCES, 0x80ffffff, "Guerrero");
            hmput(PROVINCES, 0xb04f89ff, "Oaxaca"); 
            hmput(PROVINCES, 0x2d534eff, "Chiapas"); 
            hmput(PROVINCES, 0x9a83bcff, "Tabasco"); 
            hmput(PROVINCES, 0x804000ff, "Puebla"); 
            hmput(PROVINCES, 0xb18e93ff, "Campeche"); 
            hmput(PROVINCES, 0xd2beadff, "Yucatan"); 
            hmput(PROVINCES, 0xf4948bff, "Quintana Roo"); 
            hmput(PROVINCES, 0xff0080ff, "Mexico City"); 
            hmput(PROVINCES, 0xffff80ff, "Aguascalientes"); 
            hmput(PROVINCES, 0x800080ff, "Guanajuato"); 
            hmput(PROVINCES, 0x0080ffff, "Queretaro"); 
            hmput(PROVINCES, 0x004080ff, "Hidalgo"); 
            hmput(PROVINCES, 0x00ff80ff, "State of Mexico"); 
            hmput(PROVINCES, 0x4000ffff, "Morelos"); 
            hmput(PROVINCES, 0xff8040ff, "Tlaxcala"); 
            assert(hmlen(PROVINCES) == 32);
            break;
        };

        case MAP_BRAZIL: {
            hmput(PROVINCES, 0x808080ff, "Acre");
            hmput(PROVINCES, 0x008000ff, "Rondonia");
            hmput(PROVINCES, 0x800000ff, "Amazonas");
            hmput(PROVINCES, 0xff0000ff, "Roraima");
            hmput(PROVINCES, 0x808000ff, "Para");
            hmput(PROVINCES, 0xffff00ff, "Amapa");
            hmput(PROVINCES, 0x00ff00ff, "Mato Grosso");
            hmput(PROVINCES, 0xff8040ff, "Mato Grosso Do Sul");
            hmput(PROVINCES, 0x00ffffff, "Maranhao");
            hmput(PROVINCES, 0x008080ff, "Tocantins");
            hmput(PROVINCES, 0x000080ff, "Goias");
            hmput(PROVINCES, 0x800080ff, "Piaui");
            hmput(PROVINCES, 0xff00ffff, "Ceara");
            hmput(PROVINCES, 0x808040ff, "Rio Grande do Norte");
            hmput(PROVINCES, 0xffff80ff, "Paraiba");
            hmput(PROVINCES, 0x004040ff, "Pernambuco");
            hmput(PROVINCES, 0x80ffffff, "Alagoas");
            hmput(PROVINCES, 0x004080ff, "Sergipe");
            hmput(PROVINCES, 0x8080ffff, "Bahia");
            hmput(PROVINCES, 0x4000ffff, "Minas Gerais");
            hmput(PROVINCES, 0x00272bff, "Espirito Santo");
            hmput(PROVINCES, 0xff665bff, "Rio de Janeiro");
            hmput(PROVINCES, 0x804000ff, "Sao Paulo");
            hmput(PROVINCES, 0xd5c619ff, "Parana");
            hmput(PROVINCES, 0x192a51ff, "Santa Catarina");
            hmput(PROVINCES, 0xe3dc95ff, "Rio Grande do Sul");
            hmput(PROVINCES, 0xff0080ff, "Federal District");
            assert(hmlen(PROVINCES) == 27);
            break;
        }

        case MAP_JAPAN: {
            hmput(PROVINCES, 0xed1c24ff, "Hokkaido"); 
            hmput(PROVINCES, 0xff7f27ff, "Aomori"); 
            hmput(PROVINCES, 0x22b14cff, "Iwate"); 
            hmput(PROVINCES, 0xfff200ff, "Akita"); 
            hmput(PROVINCES, 0xa349a4ff, "Miyagi"); 
            hmput(PROVINCES, 0x3f48ccff, "Yamagata"); 
            hmput(PROVINCES, 0x00ff00ff, "Fukushima"); 
            hmput(PROVINCES, 0xffc90eff, "Ibaraki"); 
            hmput(PROVINCES, 0xb5e61dff, "Tochigi"); 
            hmput(PROVINCES, 0x99d9eaff, "Gunma"); 
            hmput(PROVINCES, 0x8000ffff, "Saitama"); 
            hmput(PROVINCES, 0xff00ffff, "Chiba");
            hmput(PROVINCES, 0xff0080ff, "Tokyo"); 
            hmput(PROVINCES, 0x808000ff, "Kanagawa");
            hmput(PROVINCES, 0xffaec9ff, "Niigata"); 
            hmput(PROVINCES, 0xc8bfe7ff, "Toyama");
            hmput(PROVINCES, 0x312893ff, "Ishikawa"); 
            hmput(PROVINCES, 0x4ffdfdff, "Fukui");
            hmput(PROVINCES, 0x4bc6d3ff, "Yamanashi"); 
            hmput(PROVINCES, 0x7092beff, "Nagano");
            hmput(PROVINCES, 0xfb717bff, "Gifu"); 
            hmput(PROVINCES, 0x4f803cff, "Shizuoka");
            hmput(PROVINCES, 0x9a7deeff, "Aichi"); 
            hmput(PROVINCES, 0x9ad1a2ff, "Mie"); 
            hmput(PROVINCES, 0xf0e65bff, "Shiga"); 
            hmput(PROVINCES, 0x05e437ff, "Kyoto");
            hmput(PROVINCES, 0xa06849ff, "Osaka"); 
            hmput(PROVINCES, 0x8dd812ff, "Hyogo");
            hmput(PROVINCES, 0xc487c5ff, "Nara"); 
            hmput(PROVINCES, 0xd713d1ff, "Wakayama"); 
            hmput(PROVINCES, 0xb5d2b3ff, "Tottori"); 
            hmput(PROVINCES, 0xfafa8bff, "Shimane");
            hmput(PROVINCES, 0xf7948eff, "Okayama"); 
            hmput(PROVINCES, 0xdbaab8ff, "Hiroshima");
            hmput(PROVINCES, 0xd89ee7ff, "Yamaguchi"); 
            hmput(PROVINCES, 0xdec6a7ff, "Tokushima"); 
            hmput(PROVINCES, 0x88cefdff, "Kagawa"); 
            hmput(PROVINCES, 0x97ee9dff, "Ehime");
            hmput(PROVINCES, 0xadaed8ff, "Kochi"); 
            hmput(PROVINCES, 0xc4bacbff, "Fukuoka");
            hmput(PROVINCES, 0x98edc9ff, "Saga"); 
            hmput(PROVINCES, 0x91fefcff, "Nagasaki"); 
            hmput(PROVINCES, 0xbf9ee7ff, "Kumamoto"); 
            hmput(PROVINCES, 0xf88dadff, "Oita");
            hmput(PROVINCES, 0xabdad1ff, "Miyazaki"); 
            hmput(PROVINCES, 0xfab88bff, "Kagoshima");
            hmput(PROVINCES, 0x0000ffff, "Okinawa");
            assert(hmlen(PROVINCES) == 47);
            break;
        }

        case MAP_PHILLIPINES_ISLANDS: {
            hmput(PROVINCES, 0x0000ffff, "Luzon");
            hmput(PROVINCES, 0x000080ff, "Mindoro");
            hmput(PROVINCES, 0x008000ff, "Masbate");
            hmput(PROVINCES, 0x800000ff, "Samar");
            hmput(PROVINCES, 0x800080ff, "Panay");
            hmput(PROVINCES, 0x804000ff, "Palawan");
            hmput(PROVINCES, 0x00ffffff, "Negros");
            hmput(PROVINCES, 0xff0000ff, "Cebu");
            hmput(PROVINCES, 0xffff00ff, "Bohol");
            hmput(PROVINCES, 0x808000ff, "Leyte");
            hmput(PROVINCES, 0x008080ff, "Mindanao");
            assert(hmlen(PROVINCES) == 11);
            break;
        }

        case MAP_MALAYSIA: {
            hmput(PROVINCES, 0x808080ff, "Perlis");
            hmput(PROVINCES, 0xff0000ff, "Penang");
            hmput(PROVINCES, 0x800000ff, "Kedah");
            hmput(PROVINCES, 0x808000ff, "Perak");
            hmput(PROVINCES, 0xffff00ff, "Kelantan");
            hmput(PROVINCES, 0x008000ff, "Teregganu");
            hmput(PROVINCES, 0x00ff00ff, "Pahang");
            hmput(PROVINCES, 0x008080ff, "Selangor");
            hmput(PROVINCES, 0x00ffffff, "Negeri Sembilan");
            hmput(PROVINCES, 0x000080ff, "Malacca");
            hmput(PROVINCES, 0x0000ffff, "Johor");
            hmput(PROVINCES, 0x800080ff, "Sarawak");
            hmput(PROVINCES, 0xff00ffff, "Sabah");
            hmput(PROVINCES, 0x4000ffff, "Kuala Lumpur");
            hmput(PROVINCES, 0xff0080ff, "Putrajaya"); 
            hmput(PROVINCES, 0x804000ff, "Labuan"); 
            assert(hmlen(PROVINCES) == 16);
            break;
        }

        default: {
            assert(false);
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

void mark_province(Image bw_map, Image color_map, Color target_color, Color mark_color)
{
    for (int px = 0; px < color_map.width; ++px) {
        for (int py = 0; py < color_map.height; ++py) {
            Color current_color = GetImageColor(color_map, px, py);
            if ((current_color.r == target_color.r) && 
                (current_color.g == target_color.g) && 
                (current_color.b == target_color.b) && 
                (current_color.a == target_color.a)) {
                ImageDrawPixel(&bw_map, px, py, mark_color);
            }
        }
    } 
}

int mark_province_by_name(Image bw_map, Image color_map, const char *name, Color mark_color)
{
    Province* p = NULL;
    for (int i = 0; i < hmlen(PROVINCES); ++i) {
        if (strcmp(name, PROVINCES[i].value) == 0) {
            p = &PROVINCES[i];
        } 
    }

    if (p == NULL) return -1;

    printf("(mark_province_by_name) FOUND p!\n");
    mark_province(bw_map, color_map, GetColor(p->key), mark_color);

    return 0; 
}

void quiz(Rec *rec, Province **hidden_province)
{
    static bool draw_wrong_msg = false;
    Province *p;

    static size_t errors_current_round = 0;

    if (!any_provinces_left_to_guess()) {
        state = VICTORY;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (GetMouseX() < GetScreenWidth() * COUNTRIES_PANEL_WIDTH) goto skip_if;

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
            unsigned int hex = ColorToInt(c); 

            printf("Click is inside! imgx = %d; imgy = %d; color: (%d, %d, %d, %d) => #%08x\n", 
                    imgx, imgy, c.r, c.g, c.b, c.a, hex);

            int i = hmgeti(PROVINCES, hex);
            if (i != -1) {
                p = &PROVINCES[i];

                if (strcmp(p->value, (*hidden_province)->value) == 0) {
                    //printf("Province name = %s\n", province_name);
                    
                    if (errors_current_round == 0) {
                        mark_province(bw_map, color_map, c, COLOR_GUESSED_PERFECT_PROVINCE);
                       
                    #ifdef DEBUG_SAVE_MAP_TO_PNG    
                        printf("Writing colored map to temp file!\n");
                        ExportImage(bw_map, "temp-map.png");
                    #endif
                    } else {
                        mark_province(bw_map, color_map, c, COLOR_GUESSED_WERRORS_PROVINCE);
                        errors_current_round = 0;
                    }

                    UnloadTexture(map_texture);
                    map_texture = LoadTextureFromImage(bw_map);

                    (*hidden_province)->guessed = true;

                    if (any_provinces_left_to_guess()) {
                        *hidden_province = select_random_province();
                    } else {
                        state = VICTORY;
                        *hidden_province = NULL;
                        return;
                    }

                } else {
                    error_counter += 1;
                    draw_wrong_msg = true;

                    errors_current_round += 1;
                    if (errors_current_round >= MAX_ERRORS_CURRENT_ROUND) {
                        unsigned int key = (*hidden_province)->key; 
                        Color target_color = GetColor(key);
                        printf("Marking PROVINCE=`%s`; target_color=(%d,%d,%d,%d) = #%06x with INCORRECT_COLOR\n", (*hidden_province)->value,
                               target_color.r, target_color.g, target_color.b, target_color.a, 
                               key);

                        mark_province(bw_map, color_map, target_color, COLOR_INCORRECT_PROVINCE);
                        //mark_province_by_name(bw_map, color_map, "Veracruz", YELLOW);
                        UnloadTexture(map_texture);
                        map_texture = LoadTextureFromImage(bw_map);

                        (*hidden_province)->guessed = true;
                        if (any_provinces_left_to_guess()) {
                            *hidden_province = select_random_province();
                        } else {
                            state = VICTORY;
                            *hidden_province = NULL;
                            return;
                        }
                        
                        errors_current_round = 0;
                    }
                }
            } else {
                printf("Province name unknown! Possibly a border has been clicked. \n\n");
            }
        }
    }

skip_if:
    Rectangle status_bar = project_rectangle(CLITERAL(Rectangle) {
        .x = GetScreenWidth() * COUNTRIES_PANEL_WIDTH,
        .y = 0.0,
        .width = GetScreenWidth() * (1.0 - COUNTRIES_PANEL_WIDTH),
        .height = 65.0});

    DrawRectangleRec(status_bar, ColorBrightness(COLOR_BACKGROUND, 0.2));

    float padding = 0.01 * GetScreenHeight();
    Vector2 pos_find_str = GetScreenToWorld2D(CLITERAL(Vector2) {COUNTRIES_PANEL_WIDTH*GetScreenWidth() + 3*padding, padding}, camera);

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
            const char* text = "Wrong!"; 
            float fontsize = HUD_LARGE_FONTSIZE / camera.zoom;
            Vector2 text_len = MeasureTextEx(font, text, fontsize, 0);

            Vector2 text_pos = GetScreenToWorld2D(CLITERAL(Vector2) {
                GetScreenWidth()/2 - text_len.x/2, 
                GetScreenHeight()/2 - text_len.y/2 
            }, camera);

            BeginShaderMode(shader);
            DrawTextEx(font, text, text_pos, fontsize, 0, RED);
            EndShaderMode();
        } else {
            draw_wrong_msg = false;
            lifetime_wrong_msg = HUD_LIFETIME;
        }
    }
}

void victory() 
{
    Rectangle status_bar = project_rectangle(CLITERAL(Rectangle) {
        .x = GetScreenWidth() * COUNTRIES_PANEL_WIDTH,
        .y = 0.0,
        .width = GetScreenWidth() * (1.0 - COUNTRIES_PANEL_WIDTH),
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
    static bool show_warning_msg = false;
    static bool show_province_name = false;
 
    /*
     * The name of the clicked province is displayed using the pointer `p`.
     * The pointer p is outdated if the `active_map` has changed between the `learn` calls. 
     * We must first check for this before utilizing the pointer.
     */
    static ActiveMap local_active_map = 0; 
    static Province *p = NULL;

    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (GetMouseX() < GetScreenWidth() * COUNTRIES_PANEL_WIDTH) goto skip_if;

        Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);

        printf("mouse.x: %.5lf; mouse.y: %.5lf\n", mouse.x, mouse.y);
        DrawCircle(mouse.x, mouse.y, 10.0, RED);

        printf("ul_corner: (%.5lf, %.5lf); lr_corner: (%.5lf, %.5lf)\n", rec->ul.x, rec->ul.y, rec->lr.x, rec->lr.y);

        if ( (mouse.x < rec->ul.x) || (mouse.x > rec->lr.x) || (mouse.y < rec->ul.y) || (mouse.y > rec->lr.y)) {
            printf("Click is outside the image!\n");
        } else {
            int imgx = (int) ( (mouse.x - (screen_width/2 - DEFAULT_IMAGE_SCALE*rec->width/2)) / DEFAULT_IMAGE_SCALE);
            int imgy = (int) ( (mouse.y - (screen_height/2 - DEFAULT_IMAGE_SCALE*rec->height/2)) / DEFAULT_IMAGE_SCALE);

            Image color_map = COUNTRIES.items[active_map].color_map;
            Image bw_map    = COUNTRIES.items[active_map].bw_map;

            Color c = GetImageColor(color_map, imgx, imgy);
            unsigned int hex = ColorToInt(c);

            printf("Click is inside! imgx = %d; imgy = %d; color: (%d, %d, %d, %d) => #%08x\n", 
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
                    (min_x + max_x)/2*DEFAULT_IMAGE_SCALE + (screen_width/2 - DEFAULT_IMAGE_SCALE*rec->width/2),
                    (min_y + max_y)/2*DEFAULT_IMAGE_SCALE + (screen_height/2 - DEFAULT_IMAGE_SCALE*rec->height/2)
                };

                local_active_map = active_map;
                show_province_name = true;

            } else {
                show_warning_msg = true;
            }
        }
    }

skip_if:
    static float warning_msg_lifetime = HUD_LIFETIME;

    if (show_warning_msg) {
        warning_msg_lifetime -= GetFrameTime();

        if (warning_msg_lifetime > 0) {
            const char* text = "Click a province"; 
            float fontsize = HUD_LARGE_FONTSIZE / camera.zoom;
            Vector2 text_len = MeasureTextEx(font, text, fontsize, 0);

            Vector2 text_pos = GetScreenToWorld2D(CLITERAL(Vector2) {
                GetScreenWidth()/2 - text_len.x/2, 
                GetScreenHeight()/2 - text_len.y/2 
            }, camera);

            BeginShaderMode(shader);
            DrawTextEx(font, text, text_pos, fontsize, 0, RED);
            EndShaderMode();
        } else {
            warning_msg_lifetime = HUD_LIFETIME;
            show_warning_msg = false;
        }
    }

    static float province_name_lifetime = 3*HUD_LIFETIME;

    if (show_province_name) {
        if (active_map != local_active_map) return;

        province_name_lifetime -= GetFrameTime();

        if (province_name_lifetime > 0) {
            float fontsize = 40 / camera.zoom;
            Vector2 name_len = MeasureTextEx(font, p->value, fontsize, 0);

            Vector2 name_pos = CLITERAL(Vector2) {
                p->center.x - name_len.x/2,
                p->center.y - name_len.y/2
            };

            BeginShaderMode(shader);
            DrawTextEx(font, p->value, name_pos, fontsize, 0, RED);
            EndShaderMode();
        } else {
            province_name_lifetime = 3*HUD_LIFETIME;
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

void countries_panel(Rectangle panel_boundary, Province **hidden_province)
{
    DrawRectangleRounded(project_rectangle(panel_boundary), 0.1, 4, COLOR_COUNTRIES_PANEL_BACKGROUND);

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
        if ((int) i != (int) active_map) {
            int button_state = button(menu_entry);
            if (button_state & BS_HOVEROVER) {
                color = COLOR_PANEL_BUTTON_HOVEROVER;
            } else {
                color = COLOR_PANEL_BUTTON;
            }

            if (button_state & BS_CLICKED) {
                active_map = i;
                camera.zoom = 1.0;

                Country* c = reload_country((size_t) active_map);
                map_texture = LoadTextureFromImage(c->bw_map);
       
                fill_provinces(active_map);

                if (state == VICTORY) {
                    state = QUIZ;
                }

                *hidden_province = select_random_province();

                error_counter = 0; 
            }
        } else {
            color = COLOR_PANEL_BUTTON_SELECTED;
        }

        DrawRectangleRounded(menu_entry, 0.5, 10, color);
        
        float fontsize = 50 / camera.zoom;
        
        float line_spacing = 0;
        // TODO: check for multiple newlines
        if (TextFindIndex(c->display_name, "\n") > 0) line_spacing = 0.55 * fontsize;
        SetTextLineSpacing(line_spacing);

        Vector2 name_len = MeasureTextEx(font, c->display_name, fontsize, 0);
        name_len.y += line_spacing;
   
        // TODO: the button label is jerky when zooming. No idea why..

        int it = 0;
        while ((name_len.y > menu_entry.height) || (name_len.x > menu_entry.width)) {
            fontsize -= 1.0;

            name_len = MeasureTextEx(font, c->display_name, fontsize, 0);
            name_len.y += line_spacing;     
            
            if (it > 10) break;
            it++;
        }

        Vector2 name_pos = CLITERAL(Vector2) {
            menu_entry.x + menu_entry.width/2 - name_len.x/2, 
            menu_entry.y + menu_entry.height/2 - name_len.y/2
        };

        BeginShaderMode(shader);
        DrawTextEx(font, c->display_name, name_pos, fontsize, 0, WHITE);
        EndShaderMode();
    }
}

void control_panel(Rectangle panel_boundary)
{
    DrawRectangleRounded(project_rectangle(panel_boundary), 0.2, 7, COLOR_CONTROL_PANEL_BACKGROUND);
    
    float panel_padding = 0.02 * panel_boundary.width;
    float entry_size = 85.0;

    Rectangle quiz_button = project_rectangle(CLITERAL(Rectangle) {
        .x = panel_boundary.x + panel_padding,
        .y = panel_boundary.y + panel_padding,
        .width = panel_boundary.width/2 - 2*panel_padding,
        .height = entry_size - 2*panel_padding,
    });
    {
        int button_state = button(quiz_button);

        Color color;
        if (state == QUIZ) {
            color = COLOR_PANEL_BUTTON_SELECTED;
        } else {
            color = COLOR_PANEL_BUTTON;
        }

        if (button_state & BS_CLICKED) {
            Country *c = reload_country((size_t) active_map);

            UnloadTexture(map_texture);
            map_texture = LoadTextureFromImage(c->bw_map);

            state = QUIZ;
        }
        
        DrawRectangleRounded(quiz_button, 0.5, 10, color);
        float fontsize = 40 / camera.zoom;
        const char* text = "Quiz";
        Vector2 text_len = MeasureTextEx(font, text, fontsize, 0);

        Vector2 text_pos = CLITERAL(Vector2) {
            quiz_button.x + 0.5 * quiz_button.width - 0.5 * text_len.x, 
            quiz_button.y + 0.5 * quiz_button.height - 0.5 * text_len.y
        };

        BeginShaderMode(shader);
        DrawTextEx(font, text, text_pos, fontsize, 0, WHITE);
        EndShaderMode();
    }

    Rectangle learn_button = project_rectangle(CLITERAL(Rectangle) {
       .x = panel_boundary.x + panel_boundary.width/2 + panel_padding,
       .y = panel_boundary.y + panel_padding,
       .width = panel_boundary.width/2 - 2*panel_padding, 
       .height = entry_size - 2*panel_padding 
    });
    {
        int button_state = button(learn_button);

        Color color;
        if (state == LEARN) {
            color = COLOR_PANEL_BUTTON_SELECTED;
        } else {
            if (button_state & BS_HOVEROVER) {
                color = COLOR_PANEL_BUTTON_HOVEROVER;
            } else {
                color = COLOR_PANEL_BUTTON;
            }

            if (button_state & BS_CLICKED) {
                Country *c = reload_country((size_t) active_map);

                UnloadTexture(map_texture);
                map_texture = LoadTextureFromImage(c->bw_map);

                state = LEARN;
            } 
        }

        DrawRectangleRounded(learn_button, 0.5, 10, color);
        float fontsize = 40 / camera.zoom;
        const char* text = "Learn";
        Vector2 text_len = MeasureTextEx(font, text, fontsize, 0);

        Vector2 text_pos = CLITERAL(Vector2) {
            learn_button.x + 0.5 * learn_button.width - 0.5 * text_len.x, 
            learn_button.y + 0.5 * learn_button.height - 0.5 * text_len.y
        };

        BeginShaderMode(shader);
        DrawTextEx(font, text, text_pos, fontsize, 0, WHITE);
        EndShaderMode();
    }
}

int main()
{
    srand(time(NULL));
    stbds_rand_seed(time(NULL));

    load_country("Mexico", "Mexico");
    load_country("Brazil", "Brazil");
    load_country("Japan", "Japan");
    load_country("Phillipines-islands", "Phillipines\nIslands");
    load_country("Malaysia", "Malaysia");

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

    for (int i = 0; i < hmlen(PROVINCES); ++i) {
        Province *p = &PROVINCES[i];
        Color c = GetColor(p->key);
        printf("#%08lx :: Color=(%d,%d,%d,%d) => %s\n", p->key, c.r, c.g, c.b, c.a, p->value); 
    }

    map_texture = LoadTextureFromImage(COUNTRIES.items[active_map].bw_map);
    SetTextureFilter(map_texture, TEXTURE_FILTER_BILINEAR);

    Province* hidden_province = select_random_province();
    

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_R)) {
            Country* c = reload_country(active_map);
            map_texture = LoadTextureFromImage(c->bw_map);
    
            reset_provinces();

            hidden_province = select_random_province();
            error_counter = 0; 
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

        countries_panel(CLITERAL(Rectangle) {
            .x = 0, 
            .y = 0,
            .width = COUNTRIES_PANEL_WIDTH * GetScreenWidth(),
            .height = COUNTRIES_PANEL_HEIGHT * GetScreenHeight()
        }, &hidden_province); 
       
        float padding = 0.01;
        control_panel(CLITERAL(Rectangle) {
            .x = 0,
            .y = (COUNTRIES_PANEL_HEIGHT + padding) * GetScreenHeight(),
            .width = COUNTRIES_PANEL_WIDTH * GetScreenWidth(),
            .height = (1.0 - COUNTRIES_PANEL_HEIGHT - 2*padding) * GetScreenHeight()
        }); 

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
                victory();
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

    for (size_t i = 0; i < COUNTRIES.count; ++i) {
        Country *c = &COUNTRIES.items[i];
        UnloadImage(c->bw_map);
        UnloadImage(c->color_map);
        free(c->name);
        free(c->display_name);
        free(c->color_map_filename);
        free(c->bw_map_filename);
    }
    free(COUNTRIES.items);


    CloseWindow();

    return 0;
}
