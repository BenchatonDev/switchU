#pragma once

#include <SDL.h>
#include <string>
#include <unordered_map>

enum class TextAlign {
    LEFT,
    CENTER,
    RIGHT
};

class BitmapFont {
public:
    bool load(SDL_Renderer* renderer, const std::string& path, int gridW, int gridH, int charsPerRow);
    void renderText(SDL_Renderer* renderer, const std::string& text, int x, int y,
                            TextAlign align, int letterSpacing, SDL_Color color);

private:
    SDL_Texture* fontTexture = nullptr;
    int charWidth = 0, charHeight = 0;
    int charsPerRow = 19;
};
