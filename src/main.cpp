#include <SDL.h>
#include <SDL_image.h>

#include <rpxloader/rpxloader.h>
#include <coreinit/debug.h>
#include <coreinit/title.h>
#include <padscore/kpad.h>
#include <sndcore2/core.h>
#include <sysapp/launch.h>
#include <whb/proc.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <cstdint>

#include "input/CombinedInput.h"
#include "input/VPADInput.h"
#include "input/WPADInput.h"

#include "render.hpp"
#include "util.hpp"
#include "font.hpp"
#include "common.hpp"
#include "title_extractor.hpp"

enum RowSelection {
    ROW_TOP = 0,
    ROW_MIDDLE = 1,
    ROW_BOTTOM = 2
};

enum Menu {
    MENU_MAIN = 0,
    MENU_USER = 1,
    MENU_MORE = 2,
    MENU_SETTINGS = 3,
    MENU_SCREENSHOT = 4
};

BitmapFont font;

static Uint32 left_hold_time = 0;
static Uint32 right_hold_time = 0;
static Uint32 up_hold_time = 0;
static Uint32 down_hold_time = 0;
static bool left_scrolling = false;
static bool right_scrolling = false;
static bool up_scrolling = false;
static bool down_scrolling = false;
static bool game_launched = false;
bool quit = false;

int seperation_space = 264;
int target_camera_offset_x = 0;
int camera_offset_x = 0;
int cur_menu = MENU_MAIN;

int cur_selected_tile = 0;
int cur_selected_row = ROW_MIDDLE;
int cur_selected_subtile = 0;
int cur_selected_subrow = 0;

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

int tiles_x = WINDOW_WIDTH / 6;
int tiles_y = WINDOW_HEIGHT / 2;

const int SCROLL_INITIAL_DELAY = 500;
const int SCROLL_REPEAT_INTERVAL = 125;
const int TILE_COUNT_MIDDLE = 12;
const int TILE_COUNT_BOTTOM = 6;

const int circle_diameter = 75;
const int circle_radius = circle_diameter / 2;
const int spawn_box_size = 256;
const int visible_tile_count = WINDOW_WIDTH / seperation_space;
const int settings_row_count = 4;

SDL_Window *main_window;
SDL_Renderer *main_renderer;
SDL_Event event;
SDL_Texture* circle = NULL;
SDL_Texture* circle_selection = NULL;

SDL_Texture* load_texture(const char* path, SDL_Renderer* renderer) {
    SDL_RWops* rw = SDL_RWFromFile(path, "rb");
    if (!rw) {
        printf("SDL_RWFromFile failed: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) {
        printf("IMG_Load_RW failed: %s\n", IMG_GetError());
        return NULL;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        printf("SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
    }

    return texture;
}

int initialise() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed with error: ", SDL_GetError(), "\n");
        return EXIT_FAILURE;
    }

    if (RPXLoader_InitLibrary() != RPX_LOADER_RESULT_SUCCESS) {
        printf("RPX_LOADER failed with an error");
    }

    // Handle window creation
    main_window = SDL_CreateWindow(
        "SwitchU",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0);

    if (!main_window) {
        printf("SDL_CreateWindow failed with error: ", SDL_GetError(), "\n");
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("Failed to initialise SDL_image for PNG files: %s\n", IMG_GetError());
    }

    // Handle renderer creation
    main_renderer = SDL_CreateRenderer(main_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    circle = load_texture(SD_CARD_PATH "switchU/assets/ui_button.png", main_renderer);
    if (!circle) {
        printf("Failed to load UI Button texture\n");
    }

    circle_selection = load_texture(SD_CARD_PATH "switchU/assets/ui_button_selected.png", main_renderer);
    if (!circle_selection) {
        printf("Failed to load UI Button Selection texture\n");
    }

    font.load(main_renderer, SD_CARD_PATH "switchU/assets/font.png", 30, 30, 19);

    return EXIT_SUCCESS;
}

void shutdown() {
    if (circle) {
        SDL_DestroyTexture(circle);
        circle = NULL;
    }
    if (circle_selection) {
        SDL_DestroyTexture(circle_selection);
        circle_selection = NULL;
    }
    for (auto& app : apps) {
        if (app.icon) SDL_DestroyTexture(app.icon);
    }
    apps.clear();

    RPXLoader_DeInitLibrary();

    SDL_DestroyWindow(main_window);
    SDL_DestroyRenderer(main_renderer);
    SDL_Quit();
}

void input(Input &input) {
    Uint32 now = SDL_GetTicks();

    bool holding_left = (input.data.buttons_h & Input::STICK_L_LEFT || input.data.buttons_h & Input::BUTTON_LEFT);
    bool holding_right = (input.data.buttons_h & Input::STICK_L_RIGHT || input.data.buttons_h & Input::BUTTON_RIGHT);
    bool holding_up = (input.data.buttons_h & Input::STICK_L_UP || input.data.buttons_h & Input::BUTTON_UP);
    bool holding_down = (input.data.buttons_h & Input::STICK_L_DOWN || input.data.buttons_h & Input::BUTTON_DOWN);

    bool pressed_left = (input.data.buttons_d & Input::STICK_L_LEFT || input.data.buttons_d & Input::BUTTON_LEFT);
    bool pressed_right = (input.data.buttons_d & Input::STICK_L_RIGHT || input.data.buttons_d & Input::BUTTON_RIGHT);
    bool pressed_up = (input.data.buttons_d & Input::STICK_L_UP || input.data.buttons_d & Input::BUTTON_UP);
    bool pressed_down = (input.data.buttons_d & Input::STICK_L_DOWN || input.data.buttons_d & Input::BUTTON_DOWN);

    bool is_main_menu = (cur_menu == MENU_MAIN);
    bool is_middle_row = (cur_selected_row == ROW_MIDDLE);
    bool is_bottom_row = (cur_selected_row == ROW_BOTTOM);

    if (is_main_menu && (is_middle_row || is_bottom_row)) {
        int tile_count = is_middle_row ? TILE_COUNT_MIDDLE : TILE_COUNT_BOTTOM;

        if (pressed_left) {
            if (cur_selected_tile > 0) {
                cur_selected_tile--;
            } else if (is_middle_row) {
                cur_selected_tile = tile_count - 1;
            }
            left_hold_time = now;
            left_scrolling = true;
        } else if (holding_left && left_scrolling) {
            if (now - left_hold_time >= SCROLL_INITIAL_DELAY) {
                if (cur_selected_tile > 0) {
                    cur_selected_tile--;
                    left_hold_time = now - (SCROLL_INITIAL_DELAY - SCROLL_REPEAT_INTERVAL);
                }
            }
        } else {
            left_scrolling = false;
        }

        if (pressed_right) {
            if (cur_selected_tile < tile_count - 1) {
                cur_selected_tile++;
            } else if (is_middle_row) {
                cur_selected_tile = 0;
            }
            right_hold_time = now;
            right_scrolling = true;
        } else if (holding_right && right_scrolling) {
            if (now - right_hold_time >= SCROLL_INITIAL_DELAY) {
                if (cur_selected_tile < tile_count - 1) {
                    cur_selected_tile++;
                    right_hold_time = now - (SCROLL_INITIAL_DELAY - SCROLL_REPEAT_INTERVAL);
                }
            }
        } else {
            right_scrolling = false;
        }
    }

    if (pressed_up) {
        if (cur_menu == MENU_MAIN) {
            if (cur_selected_row > 0) cur_selected_row--;
            cur_selected_tile = 0;
        } else if (cur_menu == MENU_USER) {
            if (cur_selected_subrow > 0) cur_selected_subrow--;
        }
        up_hold_time = now;
        up_scrolling = true;
    } else if (holding_up && up_scrolling && cur_menu != MENU_MAIN) {
        if (now - up_hold_time >= SCROLL_INITIAL_DELAY) {
            if (cur_menu == MENU_USER && cur_selected_subrow > 0) {
                cur_selected_subrow--;
                up_hold_time = now - (SCROLL_INITIAL_DELAY - SCROLL_REPEAT_INTERVAL);
            }
        }
    } else {
        up_scrolling = false;
    }

    if (pressed_down) {
        if (cur_menu == MENU_MAIN) {
            if (cur_selected_row < 2) cur_selected_row++;
            cur_selected_tile = 0;
        } else if (cur_menu == MENU_USER) {
            if (cur_selected_subrow < settings_row_count - 1) cur_selected_subrow++;
        }
        down_hold_time = now;
        down_scrolling = true;
    } else if (holding_down && down_scrolling && cur_menu != MENU_MAIN) {
        if (now - down_hold_time >= SCROLL_INITIAL_DELAY) {
            if (cur_menu == MENU_USER && cur_selected_subrow < settings_row_count - 1) {
                cur_selected_subrow++;
                down_hold_time = now - (SCROLL_INITIAL_DELAY - SCROLL_REPEAT_INTERVAL);
            }
        }
    } else {
        down_scrolling = false;
    }

    if (input.data.buttons_d & Input::BUTTON_A) {
        if ((cur_selected_row == ROW_TOP)) {
            if (cur_menu == MENU_MAIN) {
                cur_menu = MENU_USER;
                cur_selected_row = ROW_MIDDLE;
            }
        } else if (cur_selected_row == ROW_MIDDLE) {
            if (cur_menu == MENU_MAIN) {
                const char* launch_path = get_selected_app_path();
                printf("Launching app with path: %s\n", launch_path);

                RPXLoaderStatus st = RPXLoader_LaunchHomebrew(launch_path);
                printf("Launch status: %s\n", RPXLoader_GetStatusStr(st));
            } else if (cur_menu == MENU_USER) {
                if (cur_selected_subrow == 0) {
                    // Insert a profile subsubmenu thing
                }
            }
        } else {
            if (cur_selected_tile == 4) {
                cur_menu = MENU_SETTINGS;
            }
        }
    }

    if (input.data.buttons_d & Input::BUTTON_B) {
        if (cur_menu != MENU_MAIN) {
            cur_menu = MENU_MAIN;
        }
    }

    if (cur_selected_row == ROW_TOP) {
        cur_selected_tile = 0;
    }

    // Only update camera if middle row is selected
    if ((cur_selected_row == ROW_MIDDLE) && (cur_menu == MENU_MAIN)) {
        const int outline_padding = 6;
        int selected_tile_x = cur_selected_tile * seperation_space;

        int tile_left = selected_tile_x - outline_padding;
        int tile_right = selected_tile_x + spawn_box_size + outline_padding;

        if (tile_left < target_camera_offset_x) {
            target_camera_offset_x = tile_left;
        } else if (tile_right > target_camera_offset_x + WINDOW_WIDTH) {
            target_camera_offset_x = tile_right - WINDOW_WIDTH + 220;
        }

        // Clamp camera within bounds
        if (target_camera_offset_x < 0) target_camera_offset_x = 0;
        int max_camera_offset = seperation_space * 24 - WINDOW_WIDTH;
        if (target_camera_offset_x > max_camera_offset) target_camera_offset_x = max_camera_offset;
    }

    if (event.type == SDL_QUIT) {
        quit = true;
    }
}

void update() {
    render_set_color(main_renderer, COLOR_BACKGROUND);
    SDL_RenderClear(main_renderer);

    // Smooth camera movement
    const float camera_speed = 0.2f;
    camera_offset_x += (int)((target_camera_offset_x - camera_offset_x) * camera_speed);

    // === Middle Row (Camera-dependent) ===
    const int base_x = tiles_x - (spawn_box_size / 2);
    const int base_y = tiles_y - 185;
    const int sub_base_y = tiles_y - 200;

    if (cur_menu == MENU_MAIN) {
        seperation_space = 264;

        for (int i = 0; i < 12; ++i) {
            int x = base_x + seperation_space * i - camera_offset_x;

            SDL_Rect icon_rect = { x, base_y, spawn_box_size, spawn_box_size };

            if (i < (int)apps.size() && apps[i].icon) {
                render_icon_with_background(main_renderer, apps[i].icon, x, base_y, spawn_box_size);
            } else {
                render_set_color(main_renderer, COLOR_UI_BOX);
                SDL_RenderDrawRect(main_renderer, &icon_rect);
            }

            if (i == cur_selected_tile && cur_selected_row == ROW_MIDDLE) {
                const int outline_padding = 3;
                const int outline_thickness = 4;

                SDL_Rect outline_rect = {
                    x - outline_padding,
                    base_y - outline_padding,
                    spawn_box_size + 2 * outline_padding,
                    spawn_box_size + 2 * outline_padding
                };

                render_set_color(main_renderer, COLOR_CYAN);

                if (i < (int)apps.size()) {
                    font.renderText(main_renderer, apps[i].title, x + 120, base_y - 34, TextAlign::CENTER, -12, {0, 225, 255, 255});
                }

                for (int t = 0; t < outline_thickness; ++t) {
                    SDL_Rect thick_rect = {
                        outline_rect.x - t,
                        outline_rect.y - t,
                        outline_rect.w + 2 * t,
                        outline_rect.h + 2 * t
                    };
                    SDL_RenderDrawRect(main_renderer, &thick_rect);
                }
            }
        }
    } else if (cur_menu == MENU_USER) {
        seperation_space = 80;

        for (int i = 0; i < settings_row_count; ++i) {
            int y = sub_base_y + seperation_space * i;

            SDL_Rect setting_rect = { base_x, y, 256, 64 };

            render_set_color(main_renderer, COLOR_WHITE);

            if (i == cur_selected_subrow) {
                const int outline_padding = 2;
                const int outline_thickness = 3;

                SDL_Rect setting_outline_rect = {
                    base_x - outline_padding,
                    y - outline_padding,
                    128 * outline_padding,
                    32 * outline_padding
                };

                render_set_color(main_renderer, COLOR_BLUE);
                font.renderText(main_renderer, "None"/**/, base_x + 8, y + 16, TextAlign::LEFT, -13, {15, 206, 185, 255});

                for (int t = 0; t < outline_thickness; ++t) {
                    SDL_Rect thick_setting_rect = {
                        setting_outline_rect.x - t,
                        setting_outline_rect.y - t,
                        setting_outline_rect.w + 2 * t,
                        setting_outline_rect.h + 2 * t
                    };
                    SDL_RenderDrawRect(main_renderer, &thick_setting_rect);
                }
            } else {
                font.renderText(main_renderer, "None"/**/, base_x + 8, y + 16, TextAlign::LEFT, -13, {255, 255, 255, 255});
            }
        }
    }

    // === Bottom Row (Fixed Position, 6 centered circles) ===
    int bottom_y = WINDOW_HEIGHT - 250;
    int total_width = (TILE_COUNT_BOTTOM * circle_diameter) + ((TILE_COUNT_BOTTOM - 1) * 32);
    int start_x = (WINDOW_WIDTH - total_width) / 2;

    if (cur_menu == MENU_MAIN) {
        for (int i = 0; i < TILE_COUNT_BOTTOM; ++i) {
            int cx = start_x + i * (circle_diameter + 32);
            int cy = bottom_y;

            SDL_Rect dst_rect = { cx, cy, circle_diameter * 2, circle_diameter * 2 };

            SDL_RenderCopy(main_renderer, circle, NULL, &dst_rect);

            if (i == cur_selected_tile && cur_selected_row == ROW_BOTTOM) {
                SDL_RenderCopy(main_renderer, circle_selection, NULL, &dst_rect);
            }
        }
    }

    // === Top Row (Fixed, 1 circle in top-right) ===
    int top_x = 32;
    int top_y = 0;

    SDL_Rect dst_rect_top = { top_x, top_y, 100, 100 };
    if ((cur_menu == MENU_MAIN) || (cur_menu == MENU_USER)) {
        SDL_RenderCopy(main_renderer, circle, NULL, &dst_rect_top);

        if (cur_selected_tile == 0 && cur_selected_row == ROW_TOP) {
            SDL_RenderCopy(main_renderer, circle_selection, NULL, &dst_rect_top);
        }
    }

    render_set_color(main_renderer, COLOR_WHITE);
    SDL_RenderDrawLine(main_renderer, WINDOW_WIDTH / 28, WINDOW_HEIGHT - 90, WINDOW_WIDTH / 1.035, WINDOW_HEIGHT - 90);
    if (cur_menu != MENU_MAIN) {
        SDL_RenderDrawLine(main_renderer, WINDOW_WIDTH / 28, 90, WINDOW_WIDTH / 1.035, 90);
    }
    if (cur_menu == MENU_SETTINGS) {
        font.renderText(main_renderer, "System Settings", 128, 32, TextAlign::LEFT, -13, {255, 255, 255, 255});
    } else if (cur_menu == MENU_USER) {
        font.renderText(main_renderer, /*Insert Wii U User's Name*/ "'s Page", 128, 32, TextAlign::LEFT, -13, {255, 255, 255, 255});
    }

    SDL_RenderPresent(main_renderer);
}

inline bool RunningFromMiiMaker() {
    return (OSGetTitleID() & 0xFFFFFFFFFFFFF0FFull) == 0x000500101004A000ull;
}

int main(int argc, char const *argv[]) {
    if (initialise() != EXIT_SUCCESS) {
        shutdown();
    }

    scan_apps(main_renderer);

    WHBProcInit();

    RPXLoader_InitLibrary();

    AXInit();
    AXQuit();

    KPADInit();
    WPADEnableURCC(TRUE);

    CombinedInput baseInput;
    VPadInput vpadInput;
    WPADInput wpadInputs[4] = {
            WPAD_CHAN_0,
            WPAD_CHAN_1,
            WPAD_CHAN_2,
            WPAD_CHAN_3};

    while (WHBProcIsRunning()) {
        baseInput.reset();
        if (vpadInput.update(1280, 720)) {
            baseInput.combine(vpadInput);
        }
        for (auto &wpadInput : wpadInputs) {
            if (wpadInput.update(1280, 720)) {
                baseInput.combine(wpadInput);
            }
        }
        baseInput.process();

        input(baseInput);

        if (quit) {
           if (RunningFromMiiMaker()) {
                // Legacy way, just quit
                break;
            } else {
                // Launch menu otherwise
                SYSLaunchMenu();
            }
        }

        update();
    }

    shutdown();

    WHBProcShutdown();
    return EXIT_SUCCESS;
}
