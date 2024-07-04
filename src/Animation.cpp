// #pragma once

#include "Animation.h"
#include <cmath>
#include <iostream>

Animation::Animation() {};

Animation::Animation(const std::string& name, SDL_Texture* t) 
: Animation(name, t, 1, 0) {}


Animation::Animation(
        const std::string& name,
        SDL_Texture* t,
        size_t frameCount,
        size_t speed
) : m_texture(t),
    m_frameCount(frameCount),
    m_currentFrame(0),
    m_speed(speed),
    m_name(name) 
{
    m_size = Vec2((float)getTextureSize(t).x / frameCount, (float)getTextureSize(t).y);
    // m_sprite.setOrigin(m_size.x / 2.0f, m_size.y / 2.0f);
    setTextureRect( std::floor(m_currentFrame) * m_size.x, 0,  m_size.x, m_size.y );
}

// update the animation to show the next frame, depending on its speed
// animation loops when it reaches the end
void Animation::update() {
    // add the speed variable to the current frame
    m_currentFrame++;
    // if (m_name == "Run") std::cout << "current frame=" << m_currentFrame << std::endl;
    // todo: 1) calculate the correct frame of animation to play based on 
    //          current frame and speed
    //       2) set the texture rectangle properly (see constructor for sample)
    size_t animFrame = (m_currentFrame / m_speed) % m_frameCount;
    setTextureRect( animFrame * m_size.x, 0, m_size.x, m_size.y );
}

const Vec2& Animation::getSize() const {
    return m_size;
}

const std::string& Animation::getName() const {
    return m_name;
}

SDL_Texture* Animation::getTexture() {
    return m_texture;
}
SDL_Rect Animation::getRect() {
    return m_rect;
}
SDL_Rect* Animation::getPtrRect()
{
    return &m_rect;
}

bool Animation::hasEnded() const {
    // todo: detect when animation has ended (last frame was played)
    // and return true
    return (m_currentFrame / m_speed) % m_frameCount == m_frameCount - 1;
}

void Animation::setTextureRect(const int x, const int y, const int w, const int h)   
{
    m_rect.x = x;
    m_rect.y = y;
    m_rect.w = w;
    m_rect.h = h;
}

void Animation::setTexture(SDL_Texture *tex) {
    m_texture = tex;
}

SDL_Point Animation::getTextureSize(SDL_Texture *texture)
{
    SDL_Point size;
    SDL_QueryTexture(texture, NULL, NULL, &size.x, &size.y);
    return size;
}