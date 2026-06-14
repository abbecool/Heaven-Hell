#include "render/SDLRenderBackend.h"

#include "assets/Assets.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace {
SDL_FRect toSDLRect(const RectF& rect)
{
    return SDL_FRect{rect.x, rect.y, rect.w, rect.h};
}
}

SDLRenderBackend::SDLRenderBackend(SDL_Renderer* renderer, Assets& assets)
    : m_renderer(renderer)
    , m_assets(assets)
{
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetDefaultTextureScaleMode(m_renderer, SDL_SCALEMODE_NEAREST);
}

void SDLRenderBackend::beginFrame(Color clearColor)
{
    SDL_SetRenderDrawColor(m_renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    SDL_RenderClear(m_renderer);
}

void SDLRenderBackend::endFrame()
{
    SDL_RenderPresent(m_renderer);
}

void SDLRenderBackend::drawSprite(const SpriteDrawCommand& command)
{
    SDL_Texture* texture = m_assets.getTexture(command.texture.name);
    SDL_FRect src = toSDLRect(command.src);
    SDL_FRect dst = toSDLRect(command.dst);
    SDL_RenderTextureRotated(
        m_renderer,
        texture,
        &src,
        &dst,
        command.angle,
        nullptr,
        SDL_FLIP_NONE
    );
}

void SDLRenderBackend::drawRect(const RectF& rect, Color color)
{
    SDL_FRect sdlRect = toSDLRect(rect);
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderRect(m_renderer, &sdlRect);
}

void SDLRenderBackend::fillRect(const RectF& rect, Color color)
{
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    if (rect.w <= 0.0f || rect.h <= 0.0f) {
        SDL_RenderFillRect(m_renderer, nullptr);
        return;
    }

    SDL_FRect sdlRect = toSDLRect(rect);
    SDL_RenderFillRect(m_renderer, &sdlRect);
}

void SDLRenderBackend::drawText(const TextDrawCommand& command)
{
    SDL_Color color = {command.color.r, command.color.g, command.color.b, command.color.a};
    SDL_Surface* surface = TTF_RenderText_Blended(
        m_assets.getFont(command.fontName),
        command.text.c_str(),
        0,
        color
    );
    if (!surface) {
        SDL_Log("TTF_RenderText_Blended error: %s", SDL_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        SDL_Log("SDL_CreateTextureFromSurface error: %s", SDL_GetError());
        return;
    }

    SDL_FRect dst = toSDLRect(command.dst);
    SDL_RenderTextureRotated(m_renderer, texture, nullptr, &dst, 0.0, nullptr, SDL_FLIP_NONE);
    SDL_DestroyTexture(texture);
}
