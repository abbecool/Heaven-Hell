#pragma once

#include "render/RenderTypes.hpp"

#include <map>
#include <string>

struct TTF_Font;

struct OpenGLGlyphInfo {
    RectF src;
    int surfaceWidth = 0;
    int surfaceHeight = 0;
    int minX = 0;
    int maxX = 0;
    int minY = 0;
    int maxY = 0;
    int advance = 0;
    bool hasImage = false;
};

class OpenGLGlyphAtlas
{
    unsigned int m_textureId = 0;
    TextureSize m_size;
    int m_ascent = 0;
    int m_descent = 0;
    int m_height = 0;
    std::map<char, OpenGLGlyphInfo> m_glyphs;

public:
    static OpenGLGlyphAtlas build(TTF_Font* font, const std::string& name);
    static char atlasCharacterFor(char character);

    void destroy();

    unsigned int textureId() const;
    TextureSize size() const;
    int ascent() const;
    int descent() const;
    int height() const;
    const OpenGLGlyphInfo* glyphFor(char character) const;
};
