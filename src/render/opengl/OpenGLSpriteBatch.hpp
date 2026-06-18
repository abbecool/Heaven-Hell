#pragma once

#include "render/RenderTypes.hpp"

#include <vector>

class OpenGLSpriteBatch
{
    static constexpr int MaxSpritesPerBatch = 8192;
    static constexpr int IndicesPerSprite = 6;

    struct QuadVertex {
        float x, y;
    };

    struct SpriteInstance {
        float dstX, dstY, dstW, dstH;
        float srcU0, srcV0, srcU1, srcV1;
        float angle;
        float r, g, b, a;
    };

    unsigned int m_program = 0;
    int m_screenSizeUniform = -1;
    unsigned int m_vertexArray = 0;
    unsigned int m_vertexBuffer = 0;
    unsigned int m_indexBuffer = 0;
    unsigned int m_instanceBuffer = 0;
    unsigned int m_whiteTexture = 0;
    unsigned int m_currentTexture = 0;
    int m_screenWidth = 1;
    int m_screenHeight = 1;
    int m_batchCount = 0;
    std::vector<SpriteInstance> m_instances;

public:
    void create();
    void destroy();
    void setScreenSize(int width, int height);
    void beginFrame();
    void flush();

    unsigned int whiteTexture() const;
    void drawTexturedQuad(
        unsigned int textureId,
        TextureSize textureSize,
        const RectF& src,
        const RectF& dst,
        float angle,
        Color color);
};
