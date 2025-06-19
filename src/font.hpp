#pragma once
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

enum class TextAlign {
    Left,
    Center,
    Right
};

class TTFText {
public:
    TTFText(SDL_Renderer* renderer);
    ~TTFText();

    bool loadFont(const std::string& path, int size, bool bold = false);
    void renderTextAt(const std::string& message, SDL_Color color, int x, int y, TextAlign align);

private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    TextAlign alignment = TextAlign::Left;

    SDL_Texture* renderText(const std::string& message, SDL_Color color);
};
