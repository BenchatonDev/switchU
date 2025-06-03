#ifndef TITLE_EXTRACTOR_HPP
#define TITLE_EXTRACTOR_HPP

#include <SDL2/SDL.h>

#include <string>
#include <vector>

struct App {
    std::string title;
    SDL_Texture* icon;
};

extern std::vector<App> apps;

SDL_Texture* load_texture(const char* path, SDL_Renderer* renderer);

// Parses and returns the <name> from a given meta.xml path
std::string get_title_from_meta(const char* path);

// Loads all game titles from the /wiiu/apps/*/meta.xml directory
void load_apps(SDL_Renderer* renderer);

#endif // TITLE_EXTRACTOR_HPP
