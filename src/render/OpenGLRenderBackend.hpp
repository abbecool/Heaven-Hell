#pragma once

#include "render/RenderBackend.hpp"

#include <map>
#include <string>
#include <vector>

struct SDL_GLContextState;
using SDL_GLContext = SDL_GLContextState*;
struct SDL_Window;
struct TTF_Font;


class OpenGLRenderBackend : public RenderBackend
{
    static constexpr int MaxSpritesPerBatch = 8192;
    static constexpr int IndicesPerSprite = 6;

    struct OpenGLTexture {
        unsigned int id = 0;
        TextureSize size;
    };
    
    struct QuadVertex {
        float x, y;
    };

    struct SpriteInstance {
        float dstX, dstY, dstW, dstH;
        float srcU0, srcV0, srcU1, srcV1;
        float angle;
        float r, g, b, a;
    };

    SDL_Window* m_window = nullptr;
    SDL_GLContext m_context = nullptr;
    int m_width = 1;
    int m_height = 1;
    unsigned int m_spriteProgram = 0;
    int m_screenSizeUniform = -1;
    unsigned int m_spriteVertexArray = 0;
    unsigned int m_spriteVertexBuffer = 0;
    unsigned int m_spriteIndexBuffer = 0;
    unsigned int m_instanceBuffer = 0;
    unsigned int m_whiteTexture = 0;
    std::map<std::string, OpenGLTexture> m_textures;
    std::map<std::string, TTF_Font*> m_fonts;
    
    std::vector<SpriteInstance> m_spriteInstances;
    unsigned int m_currentBatchTexture = 0;
    int m_spriteBatchCount = 0;

    const OpenGLTexture& getTexture(const TextureHandle& texture) const;
    TTF_Font* getFont(const std::string& name) const;
    void createSpriteRenderer();
    void flushSpriteBatch();
    void drawTexturedQuad(
        unsigned int textureId,
        TextureSize textureSize,
        const RectF& src,
        const RectF& dst,
        float angle,
        Color color);

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
