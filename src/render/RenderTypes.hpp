#pragma once

#include <cstdint>
#include <string>

struct Color
{
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
};

struct RectF
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

struct RenderView
{
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float scale = 1.0f;
    float originX = 0.0f;
    float originY = 0.0f;

    float worldToScreenX(float x) const
    {
        return (x - cameraX) * scale + originX;
    }

    float worldToScreenY(float y) const
    {
        return (y - cameraY) * scale + originY;
    }

    RectF worldToScreen(const RectF& rect) const
    {
        return RectF{
            worldToScreenX(rect.x),
            worldToScreenY(rect.y),
            rect.w * scale,
            rect.h * scale
        };
    }
};

struct TextureHandle
{
    std::string name;
};

struct TextureSize
{
    int w = 0;
    int h = 0;
};

struct SpriteDrawCommand
{
    TextureHandle texture;
    RectF src;
    RectF dst;
    float angle = 0.0f;
    float whiteTint = 0.0f;
};

struct WorldSpriteDrawCommand
{
    TextureHandle texture;
    RectF src;
    RectF dst;
    float angle = 0.0f;
    float whiteTint = 0.0f;
};

struct TextDrawCommand
{
    std::string text;
    std::string fontName;
    RectF dst;
    Color color = {255, 255, 255, 255};
};

struct WorldTextDrawCommand
{
    std::string text;
    std::string fontName;
    RectF dst;
    Color color = {255, 255, 255, 255};
};
