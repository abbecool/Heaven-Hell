#include "render/OpenGLRenderBackend.hpp"

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <array>
#include <cmath>
#include <cstddef>

namespace {
constexpr float Pi =  3.14159265358979323846f;

unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success) {
        return shader;
    }

    char infoLog[512] = {};
    glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
    glDeleteShader(shader);
    throw std::runtime_error(std::string("OpenGL shader compile failed: ") + infoLog);
}

unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource)
{
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success) {
        return program;
    }

    char infoLog[512] = {};
    glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
    glDeleteProgram(program);
    throw std::runtime_error(std::string("OpenGL shader link failed: ") + infoLog);
}
}

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

    SDL_GL_SetSwapInterval(1); // Effectivly enables vsync
    SDL_GetWindowSize(m_window, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);

    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL version: " << (version ? reinterpret_cast<const char*>(version) : "unknown") << std::endl;

    if (!TTF_Init()) {
        std::string error = SDL_GetError();
        SDL_GL_DestroyContext(m_context);
        m_context = nullptr;
        throw std::runtime_error("TTF_Init failed: " + error);
    }

    createSpriteRenderer();
}

OpenGLRenderBackend::~OpenGLRenderBackend()
{
    if (m_context) {
        SDL_GL_MakeCurrent(m_window, m_context);
        if (m_spriteVertexBuffer != 0) {
            glDeleteBuffers(1, &m_spriteVertexBuffer);
            m_spriteVertexBuffer = 0;
        }
        if (m_spriteIndexBuffer != 0) {
            glDeleteBuffers(1, &m_spriteIndexBuffer);
            m_spriteIndexBuffer = 0;
        }
        if (m_whiteTexture != 0) {
            glDeleteTextures(1, &m_whiteTexture);
            m_whiteTexture = 0;
        }
        if (m_spriteVertexArray != 0) {
            glDeleteVertexArrays(1, &m_spriteVertexArray);
            m_spriteVertexArray = 0;
        }
        if (m_spriteProgram != 0) {
            glDeleteProgram(m_spriteProgram);
            m_spriteProgram = 0;
        }
        for (auto& [name, texture] : m_textures) {
            if (texture.id != 0) {
                glDeleteTextures(1, &texture.id);
            }
        }
        m_textures.clear();

        for (auto& [name, font] : m_fonts) {
            TTF_CloseFont(font);
        }
        m_fonts.clear();
        TTF_Quit();

        SDL_GL_DestroyContext(m_context);
        m_context = nullptr;
    }
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

    if (auto it = m_textures.find(name); it != m_textures.end()) {
        if (it->second.id != 0) {
            glDeleteTextures(1, &it->second.id);
        }
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

    if (auto it = m_fonts.find(name); it != m_fonts.end()) {
        TTF_CloseFont(it->second);
    }
    m_fonts[name] = font;
}

void OpenGLRenderBackend::onWindowResized(int width, int height)
{
    m_width = width;
    m_height = height;
    glViewport(0, 0, m_width, m_height);
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

    m_spriteVertices.clear();
    m_spriteBatchCount = 0;
    m_currentBatchTexture = 0;
}

void OpenGLRenderBackend::endFrame()
{
    flushSpriteBatch();
    SDL_GL_SwapWindow(m_window);
}

void OpenGLRenderBackend::flushSpriteBatch()
{
    if (m_spriteBatchCount == 0) {
        return;
    }

    
    glUseProgram(m_spriteProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_currentBatchTexture);

    glBindVertexArray(m_spriteVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVertexBuffer);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        m_spriteVertices.size() * sizeof(SpriteVertex),
        m_spriteVertices.data()
    );
    glDrawElements(
        GL_TRIANGLES,
        m_spriteBatchCount * IndicesPerSprite,
        GL_UNSIGNED_INT,
        nullptr
    );
    
    m_spriteVertices.clear();
    m_spriteBatchCount = 0;
    m_currentBatchTexture = 0;
}

void OpenGLRenderBackend::drawTexturedQuad(
    unsigned int textureId,
    TextureSize textureSize,
    const RectF& src,
    const RectF& dst,
    float angle,
    Color color)
{
    if (m_spriteBatchCount > 0 && m_currentBatchTexture != textureId) {
        flushSpriteBatch();
    }
    
    if (m_spriteBatchCount >= MaxSpritesPerBatch) {
        flushSpriteBatch();
    }
    
    m_currentBatchTexture = textureId;

    const float u0 = src.x / static_cast<float>(textureSize.w);
    const float u1 = (src.x + src.w) / static_cast<float>(textureSize.w);
    const float v0 = src.y / static_cast<float>(textureSize.h);
    const float v1 = (src.y + src.h) / static_cast<float>(textureSize.h);

    const float centerX = dst.x + dst.w * 0.5f;
    const float centerY = dst.y + dst.h * 0.5f;
    const float halfWidth = dst.w * 0.5f;
    const float halfHeight = dst.h * 0.5f;
    const float radians = angle * Pi / 180.0f;
    const float cosAngle = std::cos(radians);
    const float sinAngle = std::sin(radians);

    auto rotatePoint = [&](float x, float y) -> std::array<float, 2> {
        return {
            centerX + x * cosAngle - y * sinAngle,
            centerY + x * sinAngle + y * cosAngle
        };
    };

    auto toClipSpace = [&](const std::array<float, 2>& point) -> std::array<float, 2> {
        return {
            point[0] / static_cast<float>(m_width) * 2.0f - 1.0f,
            1.0f - point[1] / static_cast<float>(m_height) * 2.0f
        };
    };

    const auto [topLeftX, topLeftY] = toClipSpace(rotatePoint(-halfWidth, -halfHeight));
    const auto [topRightX, topRightY] = toClipSpace(rotatePoint(halfWidth, -halfHeight));
    const auto [bottomRightX, bottomRightY] = toClipSpace(rotatePoint(halfWidth, halfHeight));
    const auto [bottomLeftX, bottomLeftY] = toClipSpace(rotatePoint(-halfWidth, halfHeight));

    const float r = static_cast<float>(color.r) / 255.0f;
    const float g = static_cast<float>(color.g) / 255.0f;
    const float b = static_cast<float>(color.b) / 255.0f;
    const float a = static_cast<float>(color.a) / 255.0f;

    m_spriteVertices.push_back({topRightX,      topRightY,      u1, v0, r, g, b, a});
    m_spriteVertices.push_back({bottomRightX,   bottomRightY,   u1, v1, r, g, b, a});
    m_spriteVertices.push_back({bottomLeftX,    bottomLeftY,    u0, v1, r, g, b, a});
    m_spriteVertices.push_back({topLeftX,       topLeftY,       u0, v0, r, g, b, a});

    m_spriteBatchCount++;
}

void OpenGLRenderBackend::drawSprite(const SpriteDrawCommand& command)
{
    const OpenGLTexture& texture = getTexture(command.texture);
    drawTexturedQuad(
        texture.id,
        texture.size,
        command.src,
        command.dst,
        command.angle,
        {255, 255, 255, 255}
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

    drawTexturedQuad(
        m_whiteTexture,
        TextureSize{1, 1},
        RectF{0.0f, 0.0f, 1.0f, 1.0f},
        dst,
        0.0f,
        color
    );
}

void OpenGLRenderBackend::drawText(const TextDrawCommand& command)
{
    if (command.text.empty()) {
        return;
    }

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

    SDL_Surface* convertedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);
    if (!convertedSurface) {
        SDL_Log("SDL_ConvertSurface error: %s", SDL_GetError());
        return;
    }

    if (!SDL_LockSurface(convertedSurface)) {
        SDL_Log("SDL_LockSurface error: %s", SDL_GetError());
        SDL_DestroySurface(convertedSurface);
        return;
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

    const TextureSize textTextureSize{convertedSurface->w, convertedSurface->h};
    SDL_DestroySurface(convertedSurface);

    drawTexturedQuad(
        textureId,
        textTextureSize,
        RectF{
            0.0f,
            0.0f,
            static_cast<float>(textTextureSize.w),
            static_cast<float>(textTextureSize.h)
        },
        command.dst,
        0.0f,
        {255, 255, 255, 255}
    );
    flushSpriteBatch();
    glDeleteTextures(1, &textureId);
}

const OpenGLRenderBackend::OpenGLTexture& OpenGLRenderBackend::getTexture(const TextureHandle& texture) const
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

void OpenGLRenderBackend::createSpriteRenderer()
{
    constexpr const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 position;
        layout (location = 1) in vec2 texCoord;
        layout (location = 2) in vec4 color;
        out vec2 vertexTexCoord;
        out vec4 vertexColor;

        void main()
        {
            vertexTexCoord = texCoord;
            vertexColor = color;
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

    constexpr const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 vertexTexCoord;
        in vec4 vertexColor;
        out vec4 fragColor;

        uniform sampler2D spriteTexture;

        void main()
        {
            fragColor = texture(spriteTexture, vertexTexCoord) * vertexColor;
        }
    )";

    m_spriteProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    const unsigned char whitePixel[] = {255, 255, 255, 255};
    glGenTextures(1, &m_whiteTexture);
    glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        1,
        1,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        whitePixel
    );
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenVertexArrays(1, &m_spriteVertexArray);
    glGenBuffers(1, &m_spriteVertexBuffer);
    glGenBuffers(1, &m_spriteIndexBuffer);

    glBindVertexArray(m_spriteVertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        MaxSpritesPerBatch * VerticesPerSprite * sizeof(SpriteVertex),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_spriteIndexBuffer);
    std::vector<unsigned int> indices;
    indices.reserve(MaxSpritesPerBatch * IndicesPerSprite);
    
    for (int i = 0; i < MaxSpritesPerBatch; ++i) {
        unsigned int base = i * VerticesPerSprite;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }
    
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );
    m_spriteVertices.reserve(MaxSpritesPerBatch * VerticesPerSprite);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 
        2, 
        GL_FLOAT, 
        GL_FALSE, 
        sizeof(SpriteVertex), 
        reinterpret_cast<void*>(offsetof(SpriteVertex, u))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(SpriteVertex),
        reinterpret_cast<void*>(offsetof(SpriteVertex, r))
    );
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glUseProgram(m_spriteProgram);
    glUniform1i(glGetUniformLocation(m_spriteProgram, "spriteTexture"), 0);
    glUseProgram(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
