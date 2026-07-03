#include "render/opengl/OpenGLGlyphAtlas.hpp"

#include <glad/glad.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstring>
#include <stdexcept>
#include <vector>

namespace {
constexpr int FirstAtlasCharacter = 32;
constexpr int LastAtlasCharacter = 126;
constexpr int GlyphPadding = 1;
constexpr unsigned char White = 255;

struct PendingGlyph {
    char character = '\0';
    OpenGLGlyphInfo info;
    SDL_Surface* surface = nullptr;
};

struct PendingGlyphs {
    std::vector<PendingGlyph> glyphs;

    ~PendingGlyphs()
    {
        for (PendingGlyph& glyph : glyphs) {
            if (glyph.surface) {
                SDL_DestroySurface(glyph.surface);
                glyph.surface = nullptr;
            }
        }
    }
};
}

OpenGLGlyphAtlas OpenGLGlyphAtlas::build(TTF_Font* font, const std::string& name)
{
    OpenGLGlyphAtlas atlas;
    atlas.m_ascent = TTF_GetFontAscent(font);
    atlas.m_descent = TTF_GetFontDescent(font);
    atlas.m_height = TTF_GetFontHeight(font);

    SDL_Color white = {White, White, White, White};
    PendingGlyphs pending;
    pending.glyphs.reserve(LastAtlasCharacter - FirstAtlasCharacter + 1);

    for (int code = FirstAtlasCharacter; code <= LastAtlasCharacter; ++code) {
        OpenGLGlyphInfo info;
        if (!TTF_GetGlyphMetrics(
                font,
                static_cast<Uint32>(code),
                &info.minX,
                &info.maxX,
                &info.minY,
                &info.maxY,
                &info.advance)) {
            throw std::runtime_error(
                "Failed to read glyph metrics for font " + name + ": " + SDL_GetError());
        }

        SDL_Surface* convertedSurface = nullptr;
        if (code != ' ') {
            SDL_Surface* surface = TTF_RenderGlyph_Blended(font, static_cast<Uint32>(code), white);
            if (!surface) {
                throw std::runtime_error(
                    "Failed to render glyph for font " + name + ": " + SDL_GetError());
            }

            convertedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(surface);
            if (!convertedSurface) {
                throw std::runtime_error(
                    "Failed to convert glyph surface for font " + name + ": " + SDL_GetError());
            }

            info.surfaceWidth = convertedSurface->w;
            info.surfaceHeight = convertedSurface->h;
            info.hasImage = info.surfaceWidth > 0 && info.surfaceHeight > 0;
        }

        pending.glyphs.push_back(PendingGlyph{
            static_cast<char>(code),
            info,
            convertedSurface
        });
    }

    auto packGlyphs = [&](int atlasSize) -> bool {
        int x = GlyphPadding;
        int y = GlyphPadding;
        int rowHeight = 0;

        for (PendingGlyph& glyph : pending.glyphs) {
            if (!glyph.info.hasImage) {
                glyph.info.src = {};
                continue;
            }

            if (glyph.info.surfaceWidth + GlyphPadding * 2 > atlasSize) {
                return false;
            }

            if (x + glyph.info.surfaceWidth + GlyphPadding > atlasSize) {
                x = GlyphPadding;
                y += rowHeight + GlyphPadding;
                rowHeight = 0;
            }

            if (y + glyph.info.surfaceHeight + GlyphPadding > atlasSize) {
                return false;
            }

            glyph.info.src = RectF{
                static_cast<float>(x),
                static_cast<float>(y),
                static_cast<float>(glyph.info.surfaceWidth),
                static_cast<float>(glyph.info.surfaceHeight)
            };

            x += glyph.info.surfaceWidth + GlyphPadding;
            if (rowHeight < glyph.info.surfaceHeight) {
                rowHeight = glyph.info.surfaceHeight;
            }
        }

        return true;
    };

    int atlasSize = 512;
    if (!packGlyphs(atlasSize)) {
        atlasSize = 1024;
        if (!packGlyphs(atlasSize)) {
            throw std::runtime_error("Printable ASCII glyph atlas does not fit for font " + name);
        }
    }

    std::vector<unsigned char> atlasPixels(
        static_cast<size_t>(atlasSize) * static_cast<size_t>(atlasSize) * 4,
        0);

    for (PendingGlyph& glyph : pending.glyphs) {
        atlas.m_glyphs[glyph.character] = glyph.info;
        if (!glyph.info.hasImage) {
            continue;
        }

        if (!SDL_LockSurface(glyph.surface)) {
            throw std::runtime_error("Failed to lock glyph surface for font " + name + ": " + SDL_GetError());
        }

        const int dstX = static_cast<int>(glyph.info.src.x);
        const int dstY = static_cast<int>(glyph.info.src.y);
        const unsigned char* srcPixels = static_cast<const unsigned char*>(glyph.surface->pixels);

        for (int row = 0; row < glyph.info.surfaceHeight; ++row) {
            const unsigned char* srcRow = srcPixels + row * glyph.surface->pitch;
            unsigned char* dstRow = atlasPixels.data()
                + (static_cast<size_t>(dstY + row) * static_cast<size_t>(atlasSize)
                + static_cast<size_t>(dstX)) * 4;
            std::memcpy(dstRow, srcRow, static_cast<size_t>(glyph.info.surfaceWidth) * 4);
        }

        SDL_UnlockSurface(glyph.surface);
    }

    unsigned int textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        atlasSize,
        atlasSize,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        atlasPixels.data()
    );
    glBindTexture(GL_TEXTURE_2D, 0);

    atlas.m_textureId = textureId;
    atlas.m_size = TextureSize{atlasSize, atlasSize};
    return atlas;
}

char OpenGLGlyphAtlas::atlasCharacterFor(char character)
{
    const unsigned char value = static_cast<unsigned char>(character);
    if (value >= FirstAtlasCharacter && value <= LastAtlasCharacter) {
        return static_cast<char>(value);
    }
    return '?';
}

void OpenGLGlyphAtlas::destroy()
{
    if (m_textureId != 0) {
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
    }
    m_glyphs.clear();
}

unsigned int OpenGLGlyphAtlas::textureId() const
{
    return m_textureId;
}

TextureSize OpenGLGlyphAtlas::size() const
{
    return m_size;
}

int OpenGLGlyphAtlas::ascent() const
{
    return m_ascent;
}

int OpenGLGlyphAtlas::descent() const
{
    return m_descent;
}

int OpenGLGlyphAtlas::height() const
{
    return m_height;
}

const OpenGLGlyphInfo* OpenGLGlyphAtlas::glyphFor(char character) const
{
    const auto it = m_glyphs.find(character);
    if (it == m_glyphs.end()) {
        return nullptr;
    }
    return &it->second;
}
