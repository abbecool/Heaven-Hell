#pragma once

#include <cstdint>
#include <vector>

struct PixelRGBA
{
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 0;
};

struct PixelImage
{
    int width = 0;
    int height = 0;
    std::vector<PixelRGBA> pixels;
};
