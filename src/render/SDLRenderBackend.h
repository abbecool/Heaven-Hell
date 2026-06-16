#pragma once

#include "render/RenderBackend.h"

class Assets;
struct SDL_Window;
struct SDL_Renderer;

class SDLRenderBackend : public RenderBackend
{
    SDL_Renderer* m_renderer = nullptr;
    Assets& m_assets;

public:
    explicit SDLRenderBackend(SDL_Window* window, Assets& assets);
    ~SDLRenderBackend() override;

    void beginFrame(Color clearColor) override;
    void endFrame() override;
    void drawSprite(const SpriteDrawCommand& command) override;
    void drawRect(const RectF& rect, Color color) override;
    void fillRect(const RectF& rect, Color color) override;
    void drawText(const TextDrawCommand& command) override;
    SDL_Renderer* getRenderer();
};
