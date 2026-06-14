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

struct TextureHandle
{
    std::string name;
};

struct SpriteDrawCommand
{
    TextureHandle texture;
    RectF src;
    RectF dst;
    float angle = 0.0f;
};

struct TextDrawCommand
{
    std::string text;
    std::string fontName;
    RectF dst;
    Color color = {255, 255, 255, 255};
};
