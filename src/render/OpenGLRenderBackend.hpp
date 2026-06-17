#pragma once

#include "render/RenderBackend.hpp"

#include <map>
#include <string>

struct SDL_GLContextState;
using SDL_GLContext = SDL_GLContextState*;
struct SDL_Window;

class OpenGLRenderBackend : public RenderBackend
{
    struct OpenGLTexture {
        unsigned int id = 0;
        TextureSize size;
    };

    SDL_Window* m_window = nullptr;
    SDL_GLContext m_context = nullptr;
    int m_width = 1;
    int m_height = 1;
    unsigned int m_triangleProgram = 0;
    unsigned int m_triangleVertexArray = 0;
    unsigned int m_triangleVertexBuffer = 0;
    std::map<std::string, OpenGLTexture> m_textures;

    const OpenGLTexture& getTexture(const TextureHandle& texture) const;
    void createDebugTriangle();

public:
    explicit OpenGLRenderBackend(SDL_Window* window);
    ~OpenGLRenderBackend() override;

    void loadTexture(const std::string& name, const std::string& path) override;
    TextureSize textureSize(const TextureHandle& texture) const override;
    void loadFont(const std::string& name, const std::string& path, int size) override;

    void onWindowResized(int width, int height) override;
    void beginFrame(Color clearColor) override;
    void endFrame() override;
    void drawSprite(const SpriteDrawCommand& command) override;
    void drawRect(const RectF& rect, Color color) override;
    void fillRect(const RectF& rect, Color color) override;
    void drawText(const TextDrawCommand& command) override;
};
