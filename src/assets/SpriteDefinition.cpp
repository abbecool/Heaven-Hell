#include "SpriteDefinition.h"

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
) :
    m_texture(texture),
    m_name(name),
    m_frameCount(std::max<size_t>(1, frameCount)),
    m_frameDuration(std::max<size_t>(1, frameDuration)),
    m_rows(std::max(1, rows)),
    m_cols(std::max(1, cols))
{
    m_frameSize = Vec2{
        static_cast<float>(textureSize.w) / m_cols,
        static_cast<float>(textureSize.h) / m_rows
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
        col * m_frameSize.x,
        row * m_frameSize.y,
        m_frameSize.x,
        m_frameSize.y
    };
}

RectF SpriteDefinition::firstFrame() const
{
    return frameRect(0, 0);
}
