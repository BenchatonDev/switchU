#include <SDL_image.h>

#include "font.hpp"

bool BitmapFont::load(SDL_Renderer* renderer, const std::string& path, int gridW, int gridH, int rowChars) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) return false;

    fontTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!fontTexture) return false;

    charWidth = gridW;
    charHeight = gridH;
    charsPerRow = rowChars;
    return true;
}

void BitmapFont::renderText(SDL_Renderer* renderer, const std::string& text, int x, int y,
                            TextAlign align, int letterSpacing, SDL_Color color) {
    if (!fontTexture) return;

    SDL_SetTextureColorMod(fontTexture, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(fontTexture, color.a);

    SDL_Rect src = { 0, 0, charWidth, charHeight };
    SDL_Rect dst = { x, y, charWidth, charHeight };

    int spacing = charWidth + letterSpacing;
    int textWidth = spacing * static_cast<int>(text.length());

    // Adjust x based on alignment
    switch (align) {
        case TextAlign::CENTER:
            dst.x -= textWidth / 2;
            break;
        case TextAlign::RIGHT:
            dst.x -= textWidth;
            break;
        case TextAlign::LEFT:
        default:
            break;
    }

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (c < 32 || c > 126) continue;

        int index = c - 32;
        src.x = (index % charsPerRow) * charWidth;
        src.y = (index / charsPerRow) * charHeight;

        SDL_RenderCopy(renderer, fontTexture, &src, &dst);
        dst.x += spacing;
    }
}
