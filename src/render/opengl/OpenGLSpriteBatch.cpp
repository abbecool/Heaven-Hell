#include "render/opengl/OpenGLSpriteBatch.hpp"

#include <glad/glad.h>

#include <cstddef>
#include <stdexcept>
#include <string>

namespace {
constexpr unsigned char WhitePixel[] = {255, 255, 255, 255};

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

void OpenGLSpriteBatch::create()
{
    constexpr const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 quadPosition;
        layout (location = 1) in vec4 dst;
        layout (location = 2) in vec4 srcUv;
        layout (location = 3) in float angle;
        layout (location = 4) in vec4 color;

        out vec2 vertexTexCoord;
        out vec4 vertexColor;

        uniform vec2 screenSize;

        void main()
        {
            vec2 scaled = quadPosition * dst.zw;
            float angleRadians = radians(angle);
            float cosAngle = cos(angleRadians);
            float sinAngle = sin(angleRadians);
            vec2 rotated = vec2(
                scaled.x * cosAngle - scaled.y * sinAngle,
                scaled.x * sinAngle + scaled.y * cosAngle
            );
            vec2 center = dst.xy + dst.zw * 0.5;
            vec2 pixelPosition = center + rotated;
            vec2 clipPosition = vec2(
                pixelPosition.x / screenSize.x * 2.0 - 1.0,
                1.0 - pixelPosition.y / screenSize.y * 2.0
            );

            vertexTexCoord = mix(srcUv.xy, srcUv.zw, quadPosition + vec2(0.5));
            vertexColor = color;
            gl_Position = vec4(clipPosition, 0.0, 1.0);
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

    m_program = createShaderProgram(vertexShaderSource, fragmentShaderSource);

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
        WhitePixel
    );
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenBuffers(1, &m_instanceBuffer);
    glGenVertexArrays(1, &m_vertexArray);
    glGenBuffers(1, &m_vertexBuffer);
    glGenBuffers(1, &m_indexBuffer);

    glBindVertexArray(m_vertexArray);

    constexpr QuadVertex quadVertices[] = {
        {0.5f, -0.5f},
        {0.5f, 0.5f},
        {-0.5f, 0.5f},
        {-0.5f, -0.5f}
    };

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    constexpr unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        MaxSpritesPerBatch * sizeof(SpriteInstance),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(SpriteInstance),
        reinterpret_cast<void*>(offsetof(SpriteInstance, dstX))
    );
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glVertexAttribPointer(
        2,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(SpriteInstance),
        reinterpret_cast<void*>(offsetof(SpriteInstance, srcU0))
    );
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glVertexAttribPointer(
        3,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(SpriteInstance),
        reinterpret_cast<void*>(offsetof(SpriteInstance, angle))
    );
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glVertexAttribPointer(
        4,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(SpriteInstance),
        reinterpret_cast<void*>(offsetof(SpriteInstance, r))
    );
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    m_instances.reserve(MaxSpritesPerBatch);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glUseProgram(m_program);
    glUniform1i(glGetUniformLocation(m_program, "spriteTexture"), 0);
    m_screenSizeUniform = glGetUniformLocation(m_program, "screenSize");
    glUseProgram(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OpenGLSpriteBatch::destroy()
{
    if (m_vertexBuffer != 0) {
        glDeleteBuffers(1, &m_vertexBuffer);
        m_vertexBuffer = 0;
    }
    if (m_indexBuffer != 0) {
        glDeleteBuffers(1, &m_indexBuffer);
        m_indexBuffer = 0;
    }
    if (m_instanceBuffer != 0) {
        glDeleteBuffers(1, &m_instanceBuffer);
        m_instanceBuffer = 0;
    }
    if (m_whiteTexture != 0) {
        glDeleteTextures(1, &m_whiteTexture);
        m_whiteTexture = 0;
    }
    if (m_vertexArray != 0) {
        glDeleteVertexArrays(1, &m_vertexArray);
        m_vertexArray = 0;
    }
    if (m_program != 0) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
    m_instances.clear();
    m_batchCount = 0;
    m_currentTexture = 0;
}

void OpenGLSpriteBatch::setScreenSize(int width, int height)
{
    m_screenWidth = width > 0 ? width : 1;
    m_screenHeight = height > 0 ? height : 1;
}

void OpenGLSpriteBatch::beginFrame()
{
    m_instances.clear();
    m_batchCount = 0;
    m_currentTexture = 0;
}

void OpenGLSpriteBatch::flush()
{
    if (m_batchCount == 0) {
        return;
    }

    glUseProgram(m_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_currentTexture);

    glBindVertexArray(m_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        m_instances.size() * sizeof(SpriteInstance),
        m_instances.data()
    );
    glUniform2f(
        m_screenSizeUniform,
        static_cast<float>(m_screenWidth),
        static_cast<float>(m_screenHeight)
    );
    glDrawElementsInstanced(
        GL_TRIANGLES,
        IndicesPerSprite,
        GL_UNSIGNED_INT,
        nullptr,
        m_batchCount
    );

    m_instances.clear();
    m_batchCount = 0;
    m_currentTexture = 0;
}

unsigned int OpenGLSpriteBatch::whiteTexture() const
{
    return m_whiteTexture;
}

void OpenGLSpriteBatch::drawTexturedQuad(
    unsigned int textureId,
    TextureSize textureSize,
    const RectF& src,
    const RectF& dst,
    float angle,
    Color color)
{
    if (m_batchCount > 0 && m_currentTexture != textureId) {
        flush();
    }

    if (m_batchCount >= MaxSpritesPerBatch) {
        flush();
    }

    m_currentTexture = textureId;

    const float u0 = src.x / static_cast<float>(textureSize.w);
    const float u1 = (src.x + src.w) / static_cast<float>(textureSize.w);
    const float v0 = src.y / static_cast<float>(textureSize.h);
    const float v1 = (src.y + src.h) / static_cast<float>(textureSize.h);

    const float r = static_cast<float>(color.r) / 255.0f;
    const float g = static_cast<float>(color.g) / 255.0f;
    const float b = static_cast<float>(color.b) / 255.0f;
    const float a = static_cast<float>(color.a) / 255.0f;

    m_instances.push_back(SpriteInstance{
        dst.x, dst.y, dst.w, dst.h,
        u0, v0, u1, v1,
        angle,
        r, g, b, a
    });

    m_batchCount++;
}
