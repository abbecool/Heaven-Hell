#pragma once

#include "render/RenderTypes.h"

class RenderBackend
{
public:
    virtual ~RenderBackend() = default;

    virtual void beginFrame(Color clearColor) = 0;
    virtual void endFrame() = 0;
    virtual void drawSprite(const SpriteDrawCommand& command) = 0;
    virtual void drawRect(const RectF& rect, Color color) = 0;
    virtual void fillRect(const RectF& rect, Color color) = 0;
    virtual void drawText(const TextDrawCommand& command) = 0;
};
