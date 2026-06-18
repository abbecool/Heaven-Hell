#include "SpriteDefinition.hpp"

#include <algorithm>

SpriteDefinition::SpriteDefinition() {};

SpriteDefinition::SpriteDefinition(const std::string& name, TextureHandle texture)
    : SpriteDefinition(name, texture, 1, 1, 1, 1, TextureSize{1, 1}) {}

SpriteDefinition::SpriteDefinition(
    const std::string& name,
    TextureHandle texture,
    size_t frameCount,
    size_t frameDuration,
    int rows,
    int cols,
    TextureSize textureSize
) : SpriteDefinition(
    name,
    texture,
    frameCount,
    frameDuration,
    rows,
    cols,
    RectF{
        0.0f,
        0.0f,
        static_cast<float>(textureSize.w),
        static_cast<float>(textureSize.h)
    }
)
{
}

SpriteDefinition::SpriteDefinition(
    const std::string& name,
    TextureHandle texture,
    size_t frameCount,
    size_t frameDuration,
    int rows,
    int cols,
    RectF sourceRegion
) :
    m_texture(texture),
    m_sourceRegion(sourceRegion),
    m_name(name),
    m_frameCount(std::max<size_t>(1, frameCount)),
    m_frameDuration(std::max<size_t>(1, frameDuration)),
    m_rows(std::max(1, rows)),
    m_cols(std::max(1, cols))
{
    m_frameSize = Vec2{
        m_sourceRegion.w / m_cols,
        m_sourceRegion.h / m_rows
    };
}

const std::string& SpriteDefinition::name() const
{
    return m_name;
}

TextureHandle SpriteDefinition::texture() const
{
    return m_texture;
}

const Vec2& SpriteDefinition::frameSize() const
{
    return m_frameSize;
}

const RectF& SpriteDefinition::sourceRegion() const
{
    return m_sourceRegion;
}

size_t SpriteDefinition::frameCount() const
{
    return m_frameCount;
}

size_t SpriteDefinition::frameDuration() const
{
    return m_frameDuration;
}

int SpriteDefinition::cols() const
{
    return m_cols;
}

bool SpriteDefinition::isAnimated() const
{
    return m_frameCount > 1;
}

RectF SpriteDefinition::frameRect(int col, int row) const
{
    return RectF{
        m_sourceRegion.x + col * m_frameSize.x,
        m_sourceRegion.y + row * m_frameSize.y,
        m_frameSize.x,
        m_frameSize.y
    };
}

RectF SpriteDefinition::firstFrame() const
{
    return frameRect(0, 0);
}
