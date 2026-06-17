#include "render/OpenGLRenderBackend.hpp"

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
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

    SDL_GL_SetSwapInterval(1);
    SDL_GetWindowSize(m_window, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);

    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL version: " << (version ? reinterpret_cast<const char*>(version) : "unknown") << std::endl;

    createDebugTriangle();
}

OpenGLRenderBackend::~OpenGLRenderBackend()
{
    if (m_context) {
        SDL_GL_MakeCurrent(m_window, m_context);
        if (m_triangleVertexBuffer != 0) {
            glDeleteBuffers(1, &m_triangleVertexBuffer);
            m_triangleVertexBuffer = 0;
        }
        if (m_triangleVertexArray != 0) {
            glDeleteVertexArrays(1, &m_triangleVertexArray);
            m_triangleVertexArray = 0;
        }
        if (m_triangleProgram != 0) {
            glDeleteProgram(m_triangleProgram);
            m_triangleProgram = 0;
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
    glUseProgram(m_triangleProgram);
    glBindVertexArray(m_triangleVertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);

    SDL_GL_SwapWindow(m_window);
}

void OpenGLRenderBackend::drawSprite(const SpriteDrawCommand& command)
{
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

void OpenGLRenderBackend::createDebugTriangle()
{
    constexpr const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 position;
        layout (location = 1) in vec3 color;
        out vec3 vertexColor;

        void main()
        {
            vertexColor = color;
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

    constexpr const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 vertexColor;
        out vec4 fragColor;

        void main()
        {
            fragColor = vec4(vertexColor, 1.0);
        }
    )";

    const float vertices[] = {
         0.0f,  0.6f,  1.0f, 0.2f, 0.2f,
        -0.6f, -0.5f,  0.2f, 1.0f, 0.2f,
         0.6f, -0.5f,  0.2f, 0.4f, 1.0f
    };

    m_triangleProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    glGenVertexArrays(1, &m_triangleVertexArray);
    glGenBuffers(1, &m_triangleVertexBuffer);

    glBindVertexArray(m_triangleVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_triangleVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
