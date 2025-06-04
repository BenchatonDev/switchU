#ifndef TITLE_EXTRACTOR_HPP
#define TITLE_EXTRACTOR_HPP

#include <SDL2/SDL.h>
#include <string>
#include <vector>

struct App {
    std::string title;
    std::string launch_path;
    SDL_Texture* icon;
};

extern std::vector<App> apps;
extern int cur_selected_tile;

SDL_Texture* load_texture(const char* path, SDL_Renderer* renderer);

// Parses and returns the <name> from a given meta.xml path
std::string get_title_from_meta(const char* path);

// Fills the apps vector with valid launchable apps (populates icon, launch_path, etc.)
void scan_apps(SDL_Renderer* renderer);

// Returns the selected app's path to pass into RPXLoader_LaunchHomebrew()
const char* get_selected_app_path();

#endif // TITLE_EXTRACTOR_HPP
