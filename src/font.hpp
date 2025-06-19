#pragma once
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class TTFText {
public:
    TTFText(SDL_Renderer* renderer);
    ~TTFText();

    bool loadFont(const std::string& path, int size);
    SDL_Texture* renderText(const std::string& message, SDL_Color color);
    void renderTextAt(const std::string& message, SDL_Color color, int x, int y);

private:
    SDL_Renderer* renderer;
    TTF_Font* font;
};
