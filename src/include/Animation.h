#pragma once

#include <SDL2/SDL.h>
// #include <Sprite.h>
#include "Vec2.h"
#include <string>
class Animation
{
    // SDL_Sprite m_sprite;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_texture;
    SDL_Rect m_srcRect;
    SDL_Rect m_destRect;
    Vec2 m_scale = {1.0, 1.0};    
    float m_angle = 0;
    size_t m_frameCount = 1; // total number of frames of animation
    size_t m_currentFrame = 0; // the current frame of animation being played
    size_t m_speed = 0; // the speed to play this animation
    Vec2 m_size = { 1, 1 }; // size of the animation frame
    std::string m_name = "none";

    public:

    Animation();
    Animation(const std::string& name, SDL_Texture* t);
    Animation(
        const std::string& name,
        SDL_Texture* t,
        size_t frameCount,
        size_t speed
    );

    void update();
    bool hasEnded() const;
    const std::string& getName() const;
    const Vec2& getSize() const;
    SDL_Texture* getTexture();
    SDL_Rect* getSrcRect();
    SDL_Rect* getDestRect();
    void setSrcRect(const int x, const int y, const int w, const int h);
    void setDestRect(const int x, const int y, const int w, const int h);
    void setDestRect(Vec2 pos);
    void setAngle(double angle);
    void setScale(Vec2 scale);
    void setTexture(SDL_Texture *tex);
    SDL_Point getTextureSize(SDL_Texture *texture);
    // SDL_Sprite& getSprite();
    void setCurrentFrame(size_t frame);
    size_t frames();
    };
