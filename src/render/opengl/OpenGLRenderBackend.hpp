#pragma once

#include "render/RenderBackend.hpp"
#include "render/opengl/OpenGLGlyphAtlas.hpp"
#include "render/opengl/OpenGLSpriteBatch.hpp"

#include <map>
#include <string>

struct SDL_GLContextState;
using SDL_GLContext = SDL_GLContextState*;
struct SDL_Window;
struct TTF_Font;

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
    RenderView m_worldView;
    OpenGLSpriteBatch m_spriteBatch;
    unsigned int m_overlayProgram = 0;
    unsigned int m_overlayVertexArray = 0;
    unsigned int m_overlayVertexBuffer = 0;
    int m_overlayScreenSizeUniform = -1;
    int m_overlayCenterUniform = -1;
    int m_overlayColorUniform = -1;
    int m_overlayCenterAlphaUniform = -1;
    int m_overlayEdgeAlphaUniform = -1;
    int m_overlayPulseUniform = -1;
    std::map<std::string, OpenGLTexture> m_textures;
    std::map<std::string, TTF_Font*> m_fonts;
    std::map<std::string, OpenGLGlyphAtlas> m_fontAtlases;

    const OpenGLTexture& getTexture(const TextureHandle& texture) const;
    TTF_Font* getFont(const std::string& name) const;
    const OpenGLGlyphAtlas& getFontAtlas(const std::string& name) const;
    void drawTextImpl(
        const std::string& text,
        const std::string& fontName,
        const RectF& dst,
        Color color,
        OpenGLRenderSpace renderSpace);

public:
    explicit OpenGLRenderBackend(SDL_Window* window);
    ~OpenGLRenderBackend() override;

    void loadTexture(const std::string& name, const std::string& path) override;
    TextureSize textureSize(const TextureHandle& texture) const override;
    void loadFont(const std::string& name, const std::string& path, int size) override;

    void onWindowResized(int width, int height) override;
    void beginFrame(Color clearColor) override;
    void endFrame() override;

    void setWorldView(const RenderView& view) override;
    void drawSprite(const SpriteDrawCommand& command) override;
    void drawWorldSprite(const WorldSpriteDrawCommand& command) override;
    void drawRect(const RectF& rect, Color color) override;
    void drawWorldRect(const RectF& rect, Color color) override;
    void fillRect(const RectF& rect, Color color) override;
    void fillWorldRect(const RectF& rect, Color color) override;
    void drawText(const TextDrawCommand& command) override;
    void drawWorldText(const WorldTextDrawCommand& command) override;
    void drawScreenRadialGradient(
        Color color,
        float centerAlpha,
        float edgeAlpha,
        float pulse,
        float centerXRatio,
        float centerYRatio) override;
};
