#pragma once

#include "ecs/Components.hpp"

#include <algorithm>
#include <string>

namespace TextBoxHelpers {

inline float estimateTextWidth(const std::string& text, float lineHeight)
{
    return static_cast<float>(text.length()) * lineHeight * 0.5f;
}

inline Vec2 measureTextBox(const CText& text, const CSprite& sprite, float padding)
{
    const Vec2 spriteSize = sprite.size();

    return Vec2{
        std::max(spriteSize.x, text.size.x + padding * 2.0f),
        std::max(spriteSize.y, text.size.y + padding * 2.0f)
    };
}

inline void configureSpriteBackedTextBox(
    CText& text,
    CTransform& transform,
    const CSprite& sprite,
    float padding
) {
    text.size.x = std::max(1.0f, estimateTextWidth(text.text, text.size.y));
    const Vec2 boxSize = measureTextBox(text, sprite, padding);
    const Vec2 spriteSize = sprite.size();
    if (spriteSize.x <= 0.0f || spriteSize.y <= 0.0f) {
        return;
    }
    transform.scale = Vec2{
        boxSize.x / spriteSize.x,
        boxSize.y / spriteSize.y
    };
}

} // namespace TextBoxHelpers
