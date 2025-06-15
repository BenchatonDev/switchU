
// texture_utils.hpp
#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <unordered_map>
#include <string>

inline SDL_Texture* load_texture(const char* path, SDL_Renderer* renderer) {
    SDL_RWops* rw = SDL_RWFromFile(path, "rb");
    if (!rw) {
        printf("SDL_RWFromFile failed: %s\n", SDL_GetError());
        return nullptr;
    }

    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) {
        printf("IMG_Load_RW failed: %s\n", IMG_GetError());
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        printf("SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
    }

    return texture;
}

inline void load_icon_set(std::unordered_map<std::string, SDL_Texture*>& iconMap, SDL_Renderer* renderer, const std::unordered_map<std::string, std::string>& paths) {
    for (const auto& [key, file] : paths) {
        SDL_Texture* tex = load_texture(file.c_str(), renderer);
        if (!tex) {
            printf("Failed to load texture: %s\n", file.c_str());
        }
        iconMap[key] = tex;
    }
}

inline void destroy_icon_set(std::unordered_map<std::string, SDL_Texture*>& iconMap) {
    for (auto& [key, tex] : iconMap) {
        if (tex) SDL_DestroyTexture(tex);
    }
    iconMap.clear();
}
