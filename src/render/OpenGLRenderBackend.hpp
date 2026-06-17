#pragma once

#include "render/RenderBackend.hpp"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

struct SDL_GLContextState;
using SDL_GLContext = SDL_GLContextState*;
struct SDL_Window;

class OpenGLRenderBackend : public RenderBackend
{
    struct OpenGLTexture {
        unsigned int id = 0;
        TextureSize size;
    };

    struct SpriteVertex {
        float x = 0.0f;
        float y = 0.0f;
        float u = 0.0f;
        float v = 0.0f;
    };

    struct SpriteBatch {
        unsigned int textureId = 0;
        size_t firstVertex = 0;
        size_t vertexCount = 0;
    };

    SDL_Window* m_window = nullptr;
    SDL_GLContext m_context = nullptr;
    int m_width = 1;
    int m_height = 1;
    unsigned int m_spriteProgram = 0;
    unsigned int m_spriteVertexArray = 0;
    unsigned int m_spriteVertexBuffer = 0;
    size_t m_spriteVertexBufferCapacity = 0;
    size_t m_spriteCount = 0;
    size_t m_spriteBatchCount = 0;
    size_t m_spriteDrawCallCount = 0;
    size_t m_spriteTextureBindCount = 0;
    std::map<std::string, OpenGLTexture> m_textures;
    std::vector<SpriteVertex> m_spriteVertices;
    std::vector<SpriteBatch> m_spriteBatches;

    const OpenGLTexture& getTexture(const TextureHandle& texture) const;
    void createSpriteRenderer();
    void ensureSpriteVertexBufferCapacity(size_t vertexCount);
    void appendSpriteBatchVertices(unsigned int textureId, const SpriteVertex* vertices, size_t vertexCount);
    void flushSpriteBatches();

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
