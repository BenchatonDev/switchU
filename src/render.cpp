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

void render_icon_with_background(SDL_Renderer* renderer, SDL_Texture* icon, int x, int y, int box_size) {
    if (!icon) return;

    int tex_w, tex_h;
    if (SDL_QueryTexture(icon, nullptr, nullptr, &tex_w, &tex_h) != 0 || tex_w == 0 || tex_h == 0)
        return;

    // Create a surface to read one pixel from the original texture safely
    SDL_Surface* icon_surface = nullptr; {
        SDL_Texture* tmp = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, tex_w, tex_h);
        if (!tmp) return;

        SDL_SetRenderTarget(renderer, tmp);
        SDL_RenderCopy(renderer, icon, nullptr, nullptr);

        icon_surface = SDL_CreateRGBSurfaceWithFormat(0, tex_w, tex_h, 32, SDL_PIXELFORMAT_RGBA8888);
        if (!icon_surface) {
            SDL_DestroyTexture(tmp);
            SDL_SetRenderTarget(renderer, nullptr);
            return;
        }

        if (SDL_RenderReadPixels(renderer, nullptr, icon_surface->format->format, icon_surface->pixels, icon_surface->pitch) != 0) {
            SDL_FreeSurface(icon_surface);
            SDL_DestroyTexture(tmp);
            SDL_SetRenderTarget(renderer, nullptr);
            return;
        }

        SDL_DestroyTexture(tmp);
        SDL_SetRenderTarget(renderer, nullptr);
    }

    Uint32 first_pixel = ((Uint32*)icon_surface->pixels)[0];
    Uint8 r, g, b, a;
    SDL_GetRGBA(first_pixel, icon_surface->format, &r, &g, &b, &a);
    SDL_FreeSurface(icon_surface);

    // Fill background
    SDL_Rect box_rect = { x, y, box_size, box_size };
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &box_rect);

    // Aspect-ratio scale the icon to fit vertically
    float aspect_ratio = (float)tex_h / tex_w;
    int new_height = static_cast<int>(box_size * aspect_ratio);
    if (new_height > box_size) new_height = box_size;

    SDL_Rect dst_rect = {
        x,
        y + (box_size - new_height) / 2,
        box_size,
        new_height
    };

    SDL_RenderCopy(renderer, icon, nullptr, &dst_rect);
}
