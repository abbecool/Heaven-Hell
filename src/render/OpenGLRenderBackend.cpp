#include "render/OpenGLRenderBackend.hpp"

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <array>
#include <cmath>

namespace {
const float Pi =  3.14159265358979323846f;
const unsigned int QuadIndices[] = {
    0, 1, 2,
    0, 2, 3
};

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
}

void OpenGLRenderBackend::endFrame()
{
    SDL_GL_SwapWindow(m_window);
}

void OpenGLRenderBackend::drawSprite(const SpriteDrawCommand& command)
{
    const OpenGLTexture& texture = getTexture(command.texture);

    // const float left = command.dst.x / static_cast<float>(m_width) * 2.0f - 1.0f;
    // const float right = (command.dst.x + command.dst.w) / static_cast<float>(m_width) * 2.0f - 1.0f;
    // const float top = 1.0f - command.dst.y / static_cast<float>(m_height) * 2.0f;
    // const float bottom = 1.0f - (command.dst.y + command.dst.h) / static_cast<float>(m_height) * 2.0f;

    const float u0 = command.src.x / static_cast<float>(texture.size.w);
    const float u1 = (command.src.x + command.src.w) / static_cast<float>(texture.size.w);
    const float v0 = command.src.y / static_cast<float>(texture.size.h);
    const float v1 = (command.src.y + command.src.h) / static_cast<float>(texture.size.h);

    const float centerX = command.dst.x + command.dst.w * 0.5f;
    const float centerY = command.dst.y + command.dst.h * 0.5f;
    const float halfWidth = command.dst.w * 0.5f;
    const float halfHeight = command.dst.h * 0.5f;
    const float radians = command.angle * Pi / 180.0f;
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

    const float vertices[] = {
        //  x,           y,         u,  v
        topRightX,    topRightY,    u1, v0,
        bottomRightX, bottomRightY, u1, v1,
        bottomLeftX,  bottomLeftY,  u0, v1,
        topLeftX,     topLeftY,     u0, v0,
    };

    glUseProgram(m_spriteProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id);

    glBindVertexArray(m_spriteVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void OpenGLRenderBackend::drawRect(const RectF& rect, Color color)
{
}

void OpenGLRenderBackend::fillRect(const RectF& rect, Color color)
{
}

void OpenGLRenderBackend::drawText(const TextDrawCommand& command)
{
}

const OpenGLRenderBackend::OpenGLTexture& OpenGLRenderBackend::getTexture(const TextureHandle& texture) const
{
    try {
        return m_textures.at(texture.name);
    } catch (const std::out_of_range&) {
        throw std::runtime_error("Texture not found: " + texture.name);
    }
}

void OpenGLRenderBackend::createSpriteRenderer()
{
    constexpr const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec4 position;
        layout (location = 1) in vec2 texCoord;
        out vec2 vertexTexCoord;

        void main()
        {
            vertexTexCoord = texCoord;
            gl_Position = position;
        }
    )";

    constexpr const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 vertexTexCoord;
        out vec4 fragColor;

        uniform sampler2D spriteTexture;

        void main()
        {
            fragColor = texture(spriteTexture, vertexTexCoord);
        }
    )";

    m_spriteProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    glGenVertexArrays(1, &m_spriteVertexArray);
    glGenBuffers(1, &m_spriteVertexBuffer);
    glGenBuffers(1, &m_spriteIndexBuffer);

    glBindVertexArray(m_spriteVertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_spriteIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(QuadIndices), QuadIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 
        2, 
        GL_FLOAT, 
        GL_FALSE, 
        4 * sizeof(float), 
        reinterpret_cast<void*>(2 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glUseProgram(m_spriteProgram);
    glUniform1i(glGetUniformLocation(m_spriteProgram, "spriteTexture"), 0);
    glUseProgram(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
