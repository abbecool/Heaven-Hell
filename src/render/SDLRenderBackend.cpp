#include "render/SDLRenderBackend.hpp"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
SDL_FRect toSDLRect(const RectF& rect)
{
    return SDL_FRect{rect.x, rect.y, rect.w, rect.h};
}
}

SDLRenderBackend::SDLRenderBackend(SDL_Window* window)
{
    m_renderer = SDL_CreateRenderer(window, nullptr);
    if (!m_renderer) {
        throw std::runtime_error(std::string("Renderer creation failed: ") + SDL_GetError());
    }

    if (!TTF_Init()) {
        std::string error = SDL_GetError();
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
        throw std::runtime_error("TTF_Init failed: " + error);
    }

    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetDefaultTextureScaleMode(m_renderer, SDL_SCALEMODE_NEAREST);
}

SDLRenderBackend::~SDLRenderBackend()
{
    for (auto& [name, texture] : m_textures) {
        SDL_DestroyTexture(texture);
    }
    m_textures.clear();

    for (auto& [name, font] : m_fonts) {
        TTF_CloseFont(font);
    }
    m_fonts.clear();

    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    TTF_Quit();
}

void SDLRenderBackend::loadTexture(const std::string& name, const std::string& path)
{
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        throw std::runtime_error("Failed to load image " + name + " from " + path + ": " + SDL_GetError());
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        throw std::runtime_error("Failed to create texture " + name + " from " + path + ": " + SDL_GetError());
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    if (auto it = m_textures.find(name); it != m_textures.end()) {
        SDL_DestroyTexture(it->second);
    }
    m_textures[name] = texture;
}

TextureSize SDLRenderBackend::textureSize(const TextureHandle& texture) const
{
    float width = 0.0f;
    float height = 0.0f;
    if (!SDL_GetTextureSize(getTexture(texture), &width, &height)) {
        throw std::runtime_error("Failed to read texture size for " + texture.name + ": " + SDL_GetError());
    }
    return TextureSize{static_cast<int>(width), static_cast<int>(height)};
}

void SDLRenderBackend::loadFont(const std::string& name, const std::string& path, int size)
{
    TTF_Font* font = TTF_OpenFont(path.c_str(), size);
    if (!font) {
        throw std::runtime_error("Failed to load font " + name + " from " + path + ": " + SDL_GetError());
    }

    if (auto it = m_fonts.find(name); it != m_fonts.end()) {
        TTF_CloseFont(it->second);
    }
    m_fonts[name] = font;
}

void SDLRenderBackend::beginFrame(Color clearColor)
{
    SDL_SetRenderDrawColor(
        m_renderer, 
        clearColor.r, 
        clearColor.g, 
        clearColor.b, 
        clearColor.a
    );
    SDL_RenderClear(m_renderer);
}

void SDLRenderBackend::endFrame()
{
    SDL_RenderPresent(m_renderer);
}

void SDLRenderBackend::drawSprite(const SpriteDrawCommand& command)
{
    SDL_Texture* texture = getTexture(command.texture);
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
    SDL_Color color = {
        command.color.r, 
        command.color.g, 
        command.color.b, 
        command.color.a
    };
    SDL_Surface* surface = TTF_RenderText_Blended(
        getFont(command.fontName),
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
    SDL_RenderTextureRotated(
        m_renderer, 
        texture, 
        nullptr, 
        &dst, 
        0.0, 
        nullptr, 
        SDL_FLIP_NONE
    );
    SDL_DestroyTexture(texture);
}

SDL_Texture* SDLRenderBackend::getTexture(const TextureHandle& texture) const
{
    try {
        return m_textures.at(texture.name);
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Texture not found: " + texture.name);
    }
}

TTF_Font* SDLRenderBackend::getFont(const std::string& name) const
{
    try {
        return m_fonts.at(name);
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Font not found: " + name);
    }
}
