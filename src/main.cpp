#include <SDL.h>
#include <SDL_image.h>

#include <coreinit/debug.h>
#include <coreinit/title.h>
#include <padscore/kpad.h>
#include <sndcore2/core.h>
#include <sysapp/launch.h>
#include <whb/proc.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "input/CombinedInput.h"
#include "input/VPADInput.h"
#include "input/WPADInput.h"

#include "render.hpp"
#include "util.hpp"
#include "font.hpp"
#include "title_extractor.hpp"

enum RowSelection {
    ROW_TOP = 0,
    ROW_MIDDLE = 1,
    ROW_BOTTOM = 2
};

BitmapFont font;

// Constants
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

const int circle_diameter = 64;
const int circle_radius = circle_diameter / 2;
const int seperation_space = 264;
const int spawn_box_size = 256;
const int visible_tile_count = WINDOW_WIDTH / seperation_space;
const int bottom_row_circle_count = 6;

// Global controls
bool quit = false;
int tiles_x = WINDOW_WIDTH / 6;
int tiles_y = WINDOW_HEIGHT / 2;
int target_camera_offset_x = 0;
int camera_offset_x = 0;
int cur_selected_tile = 0;
int cur_selected_row = ROW_MIDDLE;

// Global data
SDL_Window *main_window;
SDL_Renderer *main_renderer;
SDL_Event event;

SDL_Texture* load_texture(const char* path, SDL_Renderer* renderer) {
    SDL_RWops* rw = SDL_RWFromFile(path, "rb");
    if (!rw) {
        OSReport("SDL_RWFromFile failed: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) {
        OSReport("IMG_Load_RW failed: %s\n", IMG_GetError());
        return NULL;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        OSReport("SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
    }

    return texture;
}

int initialise() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        OSReport("SDL_Init failed with error: ", SDL_GetError(), "\n");
        return EXIT_FAILURE;
    }

    // Handle window creation
    main_window = SDL_CreateWindow(
        "Switch UI",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0);

    if (!main_window) {
        OSReport("SDL_CreateWindow failed with error: ", SDL_GetError(), "\n");
        SDL_Quit();
        return EXIT_FAILURE;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        OSReport("Failed to initialise SDL_image for PNG files: %s\n", IMG_GetError());
    }

    // Handle renderer creation
    main_renderer = SDL_CreateRenderer(main_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    font.load(main_renderer, "/vol/external01/switchU/assets/font.png", 30, 30, 19);

    return EXIT_SUCCESS;
}

void shutdown() {
    SDL_DestroyWindow(main_window);
    SDL_DestroyRenderer(main_renderer);
    SDL_Quit();
}

void input(Input &input) {
    if (input.data.buttons_d & Input::STICK_L_UP) {
        if (cur_selected_row > 0) cur_selected_row--;
        cur_selected_tile = 0;
    }

    if (input.data.buttons_d & Input::STICK_L_DOWN) {
        if (cur_selected_row < 2) cur_selected_row++;
        cur_selected_tile = 0;
    }

    if (input.data.buttons_d & Input::STICK_L_LEFT) {
        if (cur_selected_row == ROW_MIDDLE) {
            cur_selected_tile = (cur_selected_tile - 1 + 12) % 12;
        } else if (cur_selected_row == ROW_BOTTOM) {
            cur_selected_tile = (cur_selected_tile - 1 + bottom_row_circle_count) % bottom_row_circle_count;
        }
    }

    if (input.data.buttons_d & Input::STICK_L_RIGHT) {
        if (cur_selected_row == ROW_MIDDLE) {
            cur_selected_tile = (cur_selected_tile + 1) % 12;
        } else if (cur_selected_row == ROW_BOTTOM) {
            cur_selected_tile = (cur_selected_tile + 1) % bottom_row_circle_count;
        }
    }

    if (cur_selected_row == ROW_TOP) {
        cur_selected_tile = 0;
    }

    // Only update camera if middle row is selected
    if (cur_selected_row == ROW_MIDDLE) {
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
        int max_camera_offset = seperation_space * 12 - WINDOW_WIDTH;
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
    const int base_y = tiles_y - 160;

    for (int i = 0; i < 12; ++i) {
        int x = base_x + seperation_space * i - camera_offset_x;

        render_set_color(main_renderer, COLOR_UI_BOX);
        render_rectangle(main_renderer, x, base_y, spawn_box_size, spawn_box_size, false);

        if (i == cur_selected_tile && cur_selected_row == 1) {
            const int outline_padding = 3;
            const int outline_thickness = 4;

            SDL_Rect outline_rect = {
                x - outline_padding,
                base_y - outline_padding,
                spawn_box_size + 2 * outline_padding,
                spawn_box_size + 2 * outline_padding
            };

            render_set_color(main_renderer, COLOR_SELECTED_OUTLINE);
            if (i < game_titles.size()) {
                font.renderText(main_renderer, game_titles[i].c_str(), x + 120, base_y - 34, TextAlign::CENTER, -12);
            } else {
                font.renderText(main_renderer, "No Title", x + 120, base_y - 34, TextAlign::CENTER, -12);
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

    // === Bottom Row (Fixed Position, 6 centered circles) ===
    int bottom_y = WINDOW_HEIGHT - 128;
    int total_width = (bottom_row_circle_count * circle_diameter) + ((bottom_row_circle_count - 1) * 32);
    int start_x = (WINDOW_WIDTH - total_width) / 2;

    for (int i = 0; i < bottom_row_circle_count; ++i) {
        int cx = start_x + i * (circle_diameter + 32) + circle_radius;
        int cy = bottom_y;

        render_set_color(main_renderer, COLOR_UI_BOX);
        //render_circle(main_renderer, cx, cy, circle_radius, true);

        if (i == cur_selected_tile && cur_selected_row == 2) {
            render_set_color(main_renderer, COLOR_SELECTED_OUTLINE);
            //render_circle(main_renderer, cx, cy, circle_radius + 6, false);
        }
    }

    // === Top Row (Fixed, 1 circle in top-right) ===
    int top_x = 64;
    int top_y = 64;

    render_set_color(main_renderer, COLOR_UI_BOX);
    //render_circle(main_renderer, top_x + circle_radius, top_y, circle_radius, false);

    if (cur_selected_tile == 0 && cur_selected_row == 0) {
        render_set_color(main_renderer, COLOR_SELECTED_OUTLINE);
        //render_circle(main_renderer, top_x + circle_radius, top_y, circle_radius + 6, false);
    }

    render_set_color(main_renderer, COLOR_WHITE);
    SDL_RenderDrawLine(main_renderer, WINDOW_WIDTH / 28, WINDOW_HEIGHT - 64, WINDOW_WIDTH / 1.035, WINDOW_HEIGHT - 64);

    SDL_RenderPresent(main_renderer);
}

inline bool RunningFromMiiMaker() {
    return (OSGetTitleID() & 0xFFFFFFFFFFFFF0FFull) == 0x000500101004A000ull;
}

int main(int argc, char const *argv[]) {
    if (initialise() != EXIT_SUCCESS) {
        shutdown();
    }

    load_titles_from_apps();

    WHBProcInit();

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
