#pragma once

#include "render/RenderTypes.hpp"

class RenderBackend
{
public:
    virtual ~RenderBackend() = default;

    virtual void loadTexture(const std::string& name, const std::string& path) = 0;
    virtual TextureSize textureSize(const TextureHandle& texture) const = 0;
    virtual void loadFont(const std::string& name, const std::string& path, int size) = 0;

    virtual void beginFrame(Color clearColor) = 0;
    virtual void endFrame() = 0;
    virtual void drawSprite(const SpriteDrawCommand& command) = 0;
    virtual void drawRect(const RectF& rect, Color color) = 0;
    virtual void fillRect(const RectF& rect, Color color) = 0;
    virtual void drawText(const TextDrawCommand& command) = 0;
};
