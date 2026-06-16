#pragma once

#include "physics/Vec2.hpp"
#include "render/RenderTypes.hpp"

#include <cstddef>
#include <string>

class SpriteDefinition
{
    TextureHandle m_texture;
    Vec2 m_frameSize = {1, 1};
    std::string m_name = "none";
    size_t m_frameCount = 1;
    size_t m_frameDuration = 1;
    int m_rows = 1;
    int m_cols = 1;

public:
    SpriteDefinition();
    SpriteDefinition(const std::string& name, TextureHandle texture);
    SpriteDefinition(
        const std::string& name,
        TextureHandle texture,
        size_t frameCount,
        size_t frameDuration,
        int rows,
        int cols,
        TextureSize textureSize
    );

    const std::string& name() const;
    TextureHandle texture() const;
    const Vec2& frameSize() const;
    size_t frameCount() const;
    size_t frameDuration() const;
    int cols() const;
    bool isAnimated() const;

    RectF frameRect(int col, int row) const;
    RectF firstFrame() const;
};
