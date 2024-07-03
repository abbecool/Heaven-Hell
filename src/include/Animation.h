#pragma once

#include <SDL2/SDL.h>
#include "Vec2.h"
#include <string>

// #include <SFML/Graphics.hpp>

class Animation
{
    SDL_Texture* m_texture;
    SDL_Rect m_rect;
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
    SDL_Rect getRect();
    SDL_Rect* getPtrRect();
    void setTextureRect(const int x, const int y, const int w, const int h);
    void setTexture(SDL_Texture *tex);
    SDL_Point getTextureSize(SDL_Texture *texture);
};
