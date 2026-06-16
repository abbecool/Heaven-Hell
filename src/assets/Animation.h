#pragma once

#include "physics/Vec2.h"
#include "render/RenderTypes.h"

#include <cstddef>
#include <string>
class Animation
{
    TextureHandle m_texture;
    RectF m_srcRect;
    RectF m_destRect;
    Vec2 m_scale = Vec2{1, 1};    
    float m_angle = 0;
    size_t m_frameCount = 1; // total number of frames of animation
    size_t m_currentFrame = 0; // the current frame of animation being played
    size_t m_speed = 0; // the speed to play this animation
    Vec2 m_size = { 1, 1 }; // size of the animation frame
    std::string m_name = "none";
    int m_rows = 1;
    int m_cols = 1;
    int m_currentRow = 1;
    int m_currentCol = 1;

    public:

    Animation();
    Animation(const std::string& name, TextureHandle texture);
    Animation(
        const std::string& name,
        TextureHandle texture,
        size_t frameCount,
        size_t speed,
        int rows,
        int cols,
        TextureSize textureSize
    );

    void update(size_t currentFrame);
    void setRow(int row);
    bool hasEnded() const;
    const std::string& getName() const;
    const Vec2& getSize() const;
    const Vec2& getScale() const;
    TextureHandle getTextureHandle() const;
    RectF getSrcRectF() const;
    RectF getDestRectF() const;
    void setSrcRect(const int x, const int y, const int w, const int h);
    void setSrcSize(Vec2 size);
    void setDestRect(const int x, const int y, const int w, const int h);
    void setDestRect(Vec2 pos);
    void setDestSize(Vec2 size);
    void setAngle(float angle);
    void setScale(Vec2 scale);
    void setTile(Vec2 grid);
    Vec2 getDestSize() const;
    float getAngle() const;
    void setCurrentFrame(size_t frame);
    size_t frames() const;
    Vec2 getRowColumn() const;
    Vec2 getShape() const;
    };
