#include "render/opengl/OpenGLRenderBackend.hpp"

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

OpenGLRenderBackend::OpenGLRenderBackend(SDL_Window* window)
    : m_window(window)
{
    m_context = SDL_GL_CreateContext(m_window);
    if (!m_context) {
        throw std::runtime_error(std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());
    }

    if (!SDL_GL_MakeCurrent(m_window, m_context)) {
        std::string error = SDL_GetError();
        SDL_GL_DestroyContext(m_context);
        m_context = nullptr;
        throw std::runtime_error("SDL_GL_MakeCurrent failed: " + error);
    }

    auto loadOpenGLFunction = [](const char* name) -> void* {
        return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name));
    };
    if (!gladLoadGLLoader(loadOpenGLFunction)) {
        SDL_GL_DestroyContext(m_context);
        m_context = nullptr;
        throw std::runtime_error("Failed to initialize GLAD");
    }

    SDL_GL_SetSwapInterval(1);
    SDL_GetWindowSize(m_window, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);
    m_spriteBatch.setScreenSize(m_width, m_height);

    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL version: "
              << (version ? reinterpret_cast<const char*>(version) : "unknown")
              << std::endl;

    if (!TTF_Init()) {
        std::string error = SDL_GetError();
        SDL_GL_DestroyContext(m_context);
        m_context = nullptr;
        throw std::runtime_error("TTF_Init failed: " + error);
    }

    m_spriteBatch.create();

    const char* overlayVertexSource = R"(
        #version 330 core
        layout(location = 0) in vec2 aPosition;
        void main()
        {
            gl_Position = vec4(aPosition, 0.0, 1.0);
        }
    )";

    const char* overlayFragmentSource = R"(
        #version 330 core
        out vec4 fragColor;
        uniform vec2 uScreenSize;
        uniform vec2 uCenter;
        uniform vec4 uColor;
        uniform float uCenterAlpha;
        uniform float uEdgeAlpha;
        uniform float uPulse;

        void main()
        {
            vec2 pixel = gl_FragCoord.xy;
            vec2 center = uCenter;
            vec2 halfScreen = uScreenSize * 0.5;
            vec2 normalized = (pixel - center) / max(halfScreen, vec2(1.0));
            float aspect = halfScreen.x / max(halfScreen.y, 1.0);
            float dist = length(vec2(normalized.x * aspect, normalized.y));
            float t = clamp(dist, 0.0, 1.0);
            float alpha = mix(uCenterAlpha, uEdgeAlpha, t) * uPulse;
            fragColor = vec4(uColor.rgb, alpha * uColor.a);
        }
    )";

    unsigned int overlayVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(overlayVertexShader, 1, &overlayVertexSource, nullptr);
    glCompileShader(overlayVertexShader);

    unsigned int overlayFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(overlayFragmentShader, 1, &overlayFragmentSource, nullptr);
    glCompileShader(overlayFragmentShader);

    m_overlayProgram = glCreateProgram();
    glAttachShader(m_overlayProgram, overlayVertexShader);
    glAttachShader(m_overlayProgram, overlayFragmentShader);
    glLinkProgram(m_overlayProgram);

    glDeleteShader(overlayVertexShader);
    glDeleteShader(overlayFragmentShader);

    glGenVertexArrays(1, &m_overlayVertexArray);
    glGenBuffers(1, &m_overlayVertexBuffer);
    glBindVertexArray(m_overlayVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_overlayVertexBuffer);

    const float quadVertices[] = {
        -1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
         1.0f, -1.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_overlayScreenSizeUniform = glGetUniformLocation(m_overlayProgram, "uScreenSize");
    m_overlayCenterUniform = glGetUniformLocation(m_overlayProgram, "uCenter");
    m_overlayColorUniform = glGetUniformLocation(m_overlayProgram, "uColor");
    m_overlayCenterAlphaUniform = glGetUniformLocation(m_overlayProgram, "uCenterAlpha");
    m_overlayEdgeAlphaUniform = glGetUniformLocation(m_overlayProgram, "uEdgeAlpha");
    m_overlayPulseUniform = glGetUniformLocation(m_overlayProgram, "uPulse");
}

OpenGLRenderBackend::~OpenGLRenderBackend()
{
    if (!m_context) {
        return;
    }

    SDL_GL_MakeCurrent(m_window, m_context);
    m_spriteBatch.destroy();

    if (m_overlayVertexBuffer != 0) {
        glDeleteBuffers(1, &m_overlayVertexBuffer);
        m_overlayVertexBuffer = 0;
    }
    if (m_overlayVertexArray != 0) {
        glDeleteVertexArrays(1, &m_overlayVertexArray);
        m_overlayVertexArray = 0;
    }
    if (m_overlayProgram != 0) {
        glDeleteProgram(m_overlayProgram);
        m_overlayProgram = 0;
    }

    for (auto& [name, texture] : m_textures) {
        if (texture.id != 0) {
            glDeleteTextures(1, &texture.id);
        }
    }
    m_textures.clear();

    for (auto& [name, atlas] : m_fontAtlases) {
        atlas.destroy();
    }
    m_fontAtlases.clear();

    for (auto& [name, font] : m_fonts) {
        TTF_CloseFont(font);
    }
    m_fonts.clear();
    TTF_Quit();

    SDL_GL_DestroyContext(m_context);
    m_context = nullptr;
}

void OpenGLRenderBackend::loadTexture(const std::string& name, const std::string& path)
{
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (!loadedSurface) {
        throw std::runtime_error("Failed to load image " + name + " from " + path + ": " + SDL_GetError());
    }

    SDL_Surface* convertedSurface = SDL_ConvertSurface(loadedSurface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loadedSurface);
    if (!convertedSurface) {
        throw std::runtime_error("Failed to convert image " + name + " from " + path + ": " + SDL_GetError());
    }

    if (!SDL_LockSurface(convertedSurface)) {
        std::string error = SDL_GetError();
        SDL_DestroySurface(convertedSurface);
        throw std::runtime_error("Failed to lock image " + name + " from " + path + ": " + error);
    }

    unsigned int textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        convertedSurface->w,
        convertedSurface->h,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        convertedSurface->pixels
    );
    glBindTexture(GL_TEXTURE_2D, 0);

    SDL_UnlockSurface(convertedSurface);

    if (auto it = m_textures.find(name); it != m_textures.end() && it->second.id != 0) {
        glDeleteTextures(1, &it->second.id);
    }
    m_textures[name] = OpenGLTexture{
        textureId,
        TextureSize{convertedSurface->w, convertedSurface->h}
    };

    SDL_DestroySurface(convertedSurface);
}

TextureSize OpenGLRenderBackend::textureSize(const TextureHandle& texture) const
{
    return getTexture(texture).size;
}

void OpenGLRenderBackend::loadFont(const std::string& name, const std::string& path, int size)
{
    TTF_Font* font = TTF_OpenFont(path.c_str(), size);
    if (!font) {
        throw std::runtime_error("Failed to load font " + name + " from " + path + ": " + SDL_GetError());
    }

    OpenGLGlyphAtlas atlas;
    try {
        atlas = OpenGLGlyphAtlas::build(font, name);
    } catch (...) {
        TTF_CloseFont(font);
        throw;
    }

    if (auto it = m_fonts.find(name); it != m_fonts.end()) {
        TTF_CloseFont(it->second);
    }
    if (auto it = m_fontAtlases.find(name); it != m_fontAtlases.end()) {
        it->second.destroy();
    }

    m_fonts[name] = font;
    m_fontAtlases[name] = std::move(atlas);
}

void OpenGLRenderBackend::onWindowResized(int width, int height)
{
    m_width = width;
    m_height = height;
    glViewport(0, 0, m_width, m_height);
    m_spriteBatch.setScreenSize(m_width, m_height);
}

void OpenGLRenderBackend::beginFrame(Color clearColor)
{
    glViewport(0, 0, m_width, m_height);
    glClearColor(
        static_cast<float>(clearColor.r) / 255.0f,
        static_cast<float>(clearColor.g) / 255.0f,
        static_cast<float>(clearColor.b) / 255.0f,
        static_cast<float>(clearColor.a) / 255.0f
    );
    glClear(GL_COLOR_BUFFER_BIT);

    m_spriteBatch.setScreenSize(m_width, m_height);
    m_spriteBatch.beginFrame();
}

void OpenGLRenderBackend::endFrame()
{
    m_spriteBatch.flush();
    SDL_GL_SwapWindow(m_window);
}

void OpenGLRenderBackend::setWorldView(const RenderView& view)
{
    m_worldView = view;
    m_spriteBatch.setWorldView(view);
}

void OpenGLRenderBackend::drawSprite(const SpriteDrawCommand& command)
{
    const OpenGLTexture& texture = getTexture(command.texture);
    m_spriteBatch.drawTexturedQuad(
        texture.id,
        texture.size,
        command.src,
        command.dst,
        command.angle,
        {255, 255, 255, 255},
        OpenGLRenderSpace::Screen,
        command.whiteTint
    );
}

void OpenGLRenderBackend::drawWorldSprite(const WorldSpriteDrawCommand& command)
{
    const OpenGLTexture& texture = getTexture(command.texture);
    m_spriteBatch.drawTexturedQuad(
        texture.id,
        texture.size,
        command.src,
        command.dst,
        command.angle,
        {255, 255, 255, 255},
        OpenGLRenderSpace::World,
        command.whiteTint
    );
}

void OpenGLRenderBackend::drawRect(const RectF& rect, Color color)
{
    if (rect.w <= 0.0f || rect.h <= 0.0f) {
        return;
    }

    constexpr float Thickness = 1.0f;
    fillRect({rect.x, rect.y, rect.w, Thickness}, color);
    fillRect({rect.x, rect.y + rect.h - Thickness, rect.w, Thickness}, color);
    fillRect({rect.x, rect.y, Thickness, rect.h}, color);
    fillRect({rect.x + rect.w - Thickness, rect.y, Thickness, rect.h}, color);
}

void OpenGLRenderBackend::drawWorldRect(const RectF& rect, Color color)
{
    if (rect.w <= 0.0f || rect.h <= 0.0f) {
        return;
    }

    const float thickness = m_worldView.scale > 0.0f ? 1.0f / m_worldView.scale : 1.0f;
    fillWorldRect({rect.x, rect.y, rect.w, thickness}, color);
    fillWorldRect({rect.x, rect.y + rect.h - thickness, rect.w, thickness}, color);
    fillWorldRect({rect.x, rect.y, thickness, rect.h}, color);
    fillWorldRect({rect.x + rect.w - thickness, rect.y, thickness, rect.h}, color);
}

void OpenGLRenderBackend::fillRect(const RectF& rect, Color color)
{
    RectF dst = rect;
    if (dst.w <= 0.0f || dst.h <= 0.0f) {
        dst = RectF{
            0.0f,
            0.0f,
            static_cast<float>(m_width),
            static_cast<float>(m_height)
        };
    }

    m_spriteBatch.drawTexturedQuad(
        m_spriteBatch.whiteTexture(),
        TextureSize{1, 1},
        RectF{0.0f, 0.0f, 1.0f, 1.0f},
        dst,
        0.0f,
        color,
        OpenGLRenderSpace::Screen
    );
}

void OpenGLRenderBackend::fillWorldRect(const RectF& rect, Color color)
{
    if (rect.w <= 0.0f || rect.h <= 0.0f) {
        return;
    }

    m_spriteBatch.drawTexturedQuad(
        m_spriteBatch.whiteTexture(),
        TextureSize{1, 1},
        RectF{0.0f, 0.0f, 1.0f, 1.0f},
        rect,
        0.0f,
        color,
        OpenGLRenderSpace::World
    );
}

void OpenGLRenderBackend::drawScreenRadialGradient(
    Color color,
    float centerAlpha,
    float edgeAlpha,
    float pulse,
    float centerXRatio,
    float centerYRatio)
{
    if (m_overlayProgram == 0) {
        return;
    }

    glUseProgram(m_overlayProgram);
    glBindVertexArray(m_overlayVertexArray);

    glUniform2f(m_overlayScreenSizeUniform, static_cast<float>(m_width), static_cast<float>(m_height));
    glUniform2f(m_overlayCenterUniform,
        static_cast<float>(m_width) * centerXRatio,
        static_cast<float>(m_height) * centerYRatio);
    glUniform4f(
        m_overlayColorUniform,
        static_cast<float>(color.r) / 255.0f,
        static_cast<float>(color.g) / 255.0f,
        static_cast<float>(color.b) / 255.0f,
        static_cast<float>(color.a) / 255.0f);
    glUniform1f(m_overlayCenterAlphaUniform, centerAlpha);
    glUniform1f(m_overlayEdgeAlphaUniform, edgeAlpha);
    glUniform1f(m_overlayPulseUniform, pulse);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
}

void OpenGLRenderBackend::drawText(const TextDrawCommand& command)
{
    drawTextImpl(
        command.text,
        command.fontName,
        command.dst,
        command.color,
        OpenGLRenderSpace::Screen
    );
}

void OpenGLRenderBackend::drawWorldText(const WorldTextDrawCommand& command)
{
    drawTextImpl(
        command.text,
        command.fontName,
        command.dst,
        command.color,
        OpenGLRenderSpace::World
    );
}

void OpenGLRenderBackend::drawTextImpl(
    const std::string& text,
    const std::string& fontName,
    const RectF& dst,
    Color color,
    OpenGLRenderSpace renderSpace)
{
    if (text.empty()) {
        return;
    }

    const OpenGLGlyphAtlas& atlas = getFontAtlas(fontName);
    TTF_Font* font = getFont(fontName);
    if (atlas.height() <= 0 || dst.w == 0.0f || dst.h == 0.0f) {
        return;
    }

    float naturalWidth = 0.0f;
    char previousCharacter = '\0';
    bool hasPreviousCharacter = false;

    for (char rawCharacter : text) {
        const char character = OpenGLGlyphAtlas::atlasCharacterFor(rawCharacter);
        const OpenGLGlyphInfo* glyph = atlas.glyphFor(character);
        if (!glyph) {
            continue;
        }

        if (hasPreviousCharacter) {
            int kerning = 0;
            if (TTF_GetGlyphKerning(
                    font,
                    static_cast<Uint32>(static_cast<unsigned char>(previousCharacter)),
                    static_cast<Uint32>(static_cast<unsigned char>(character)),
                    &kerning)) {
                naturalWidth += static_cast<float>(kerning);
            }
        }

        naturalWidth += static_cast<float>(glyph->advance);
        previousCharacter = character;
        hasPreviousCharacter = true;
    }

    if (naturalWidth <= 0.0f) {
        return;
    }

    const float scaleX = dst.w / naturalWidth;
    const float scaleY = dst.h / static_cast<float>(atlas.height());

    float penX = 0.0f;
    previousCharacter = '\0';
    hasPreviousCharacter = false;

    for (char rawCharacter : text) {
        const char character = OpenGLGlyphAtlas::atlasCharacterFor(rawCharacter);
        const OpenGLGlyphInfo* glyph = atlas.glyphFor(character);
        if (!glyph) {
            continue;
        }

        if (hasPreviousCharacter) {
            int kerning = 0;
            if (TTF_GetGlyphKerning(
                    font,
                    static_cast<Uint32>(static_cast<unsigned char>(previousCharacter)),
                    static_cast<Uint32>(static_cast<unsigned char>(character)),
                    &kerning)) {
                penX += static_cast<float>(kerning);
            }
        }

        if (glyph->hasImage) {
            const float glyphX = penX + static_cast<float>(glyph->minX);
            const RectF glyphDst{
                dst.x + glyphX * scaleX,
                dst.y,
                static_cast<float>(glyph->surfaceWidth) * scaleX,
                static_cast<float>(glyph->surfaceHeight) * scaleY
            };

            m_spriteBatch.drawTexturedQuad(
                atlas.textureId(),
                atlas.size(),
                glyph->src,
                glyphDst,
                0.0f,
                color,
                renderSpace
            );
        }

        penX += static_cast<float>(glyph->advance);
        previousCharacter = character;
        hasPreviousCharacter = true;
    }
}

const OpenGLRenderBackend::OpenGLTexture& OpenGLRenderBackend::getTexture(
    const TextureHandle& texture) const
{
    try {
        return m_textures.at(texture.name);
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Texture not found: " + texture.name);
    }
}

TTF_Font* OpenGLRenderBackend::getFont(const std::string& name) const
{
    try {
        return m_fonts.at(name);
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Font not found: " + name);
    }
}

const OpenGLGlyphAtlas& OpenGLRenderBackend::getFontAtlas(const std::string& name) const
{
    try {
        return m_fontAtlases.at(name);
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Font atlas not found: " + name);
    }
}
