#pragma once

#include "render/RenderBackend.h"

#include <map>
#include <string>

struct SDL_Texture;
struct SDL_Window;
struct SDL_Renderer;
struct TTF_Font;

class SDLRenderBackend : public RenderBackend
{
    SDL_Renderer* m_renderer = nullptr;
    std::map<std::string, SDL_Texture*> m_textures;
    std::map<std::string, TTF_Font*> m_fonts;

    SDL_Texture* getTexture(const TextureHandle& texture) const;
    TTF_Font* getFont(const std::string& name) const;

public:
    explicit SDLRenderBackend(SDL_Window* window);
    ~SDLRenderBackend() override;

    void loadTexture(const std::string& name, const std::string& path) override;
    TextureSize textureSize(const TextureHandle& texture) const override;
    void loadFont(const std::string& name, const std::string& path, int size) override;

    void beginFrame(Color clearColor) override;
    void endFrame() override;
    void drawSprite(const SpriteDrawCommand& command) override;
    void drawRect(const RectF& rect, Color color) override;
    void fillRect(const RectF& rect, Color color) override;
    void drawText(const TextDrawCommand& command) override;
};
