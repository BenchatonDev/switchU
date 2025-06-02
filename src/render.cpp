#include <SDL_ttf.h>

#include "render.hpp"

void render_set_color(SDL_Renderer *renderer, RenderColor color) {
    SDL_SetRenderDrawColor(renderer, color.rr, color.gg, color.bb, color.aa);
}

void render_rectangle(SDL_Renderer *renderer, int xx, int yy, int ww, int hh, bool filled) {
    SDL_Rect rectangle = {xx, yy, ww, hh};
    if (filled) {
        SDL_RenderFillRect(renderer, &rectangle);
    } else {
        SDL_RenderDrawRect(renderer, &rectangle);
    }
}

void render_circle(SDL_Renderer *renderer, int32_t centreX, int32_t centreY, int32_t radius, bool fill) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                if (fill || (dx * dx + dy * dy) >= ((radius - 2) * (radius - 2))) {
                    SDL_RenderDrawPoint(renderer, centreX + dx, centreY + dy);
                }
            }
        }
    }
}

void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dst = { x, y, surface->w, surface->h };

    SDL_FreeSurface(surface);
    if (!texture) return;

    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
}
