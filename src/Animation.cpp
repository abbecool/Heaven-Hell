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
) : 
    m_texture(t),
    m_frameCount(frameCount),
    m_currentFrame(0),
    m_speed(speed),
    m_name(name)
{
    m_size = Vec2((float)getTextureSize().x / frameCount, (float)getTextureSize().y);
    setSrcRect( std::floor(m_currentFrame) * m_size.x, 0,  m_size.x, m_size.y );
}

// update the animation to show the next frame, depending on its speed
// animation loops when it reaches the end
void Animation::update() {
    m_currentFrame++;
    size_t animFrame = (m_currentFrame / m_speed) % m_frameCount;
    setSrcRect( animFrame * m_size.x, 0, m_size.x, m_size.y );
}

const Vec2& Animation::getSize() const {
    return m_size;
}

const Vec2& Animation::getScale() const {
    return m_scale;
}

const std::string& Animation::getName() const {
    return m_name;
}

SDL_Texture* Animation::getTexture() {
    return m_texture;
}

SDL_Rect* Animation::getSrcRect()
{
    return &m_srcRect;
}

SDL_Rect* Animation::getDestRect()
{
    return &m_destRect;
}

bool Animation::hasEnded() const {
    return (m_currentFrame / m_speed) % m_frameCount == m_frameCount - 1;
}
void Animation::setSrcRect(const int x, const int y, const int w, const int h)   
{
    m_srcRect.x = x;
    m_srcRect.y = y;
    m_srcRect.w = w;
    m_srcRect.h = h;
}

void Animation::setDestRect(Vec2 pos)   
{
    m_destRect.x = pos.x;
    m_destRect.y = pos.y;
}

void Animation::setDestSize(Vec2 size)   
{
    m_destRect.w = size.x;
    m_destRect.h = size.y;
}

void Animation::setDestRect(const int x, const int y, const int w, const int h)   
{
    m_destRect.x = x;
    m_destRect.y = y;
    m_destRect.w = w;
    m_destRect.h = h;
}

Vec2 Animation::getDestSize()   
{
    return Vec2 {(float)m_destRect.w , (float)m_destRect.h};
}

void Animation::setTexture(SDL_Texture *tex) {
    m_texture = tex;
}

SDL_Point Animation::getTextureSize()
{
    SDL_Point size;
    SDL_QueryTexture(m_texture, NULL, NULL, &size.x, &size.y);
    return size;
}

void Animation::setAngle(double angle) {
    m_angle = angle;
}

void Animation::setScale(Vec2 scale) {
    m_scale = scale;
    m_destRect.w = m_srcRect.w * scale.x;
    m_destRect.h = m_srcRect.h * scale.y;
}

void Animation::setCurrentFrame(size_t frame){
    m_currentFrame = frame;
}

size_t Animation::frames(){
    return m_frameCount;
}
