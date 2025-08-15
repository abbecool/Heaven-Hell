#include "Animation.h"
#include <cmath>
#include <iostream>

Animation::Animation() {};

Animation::Animation(const std::string& name, SDL_Texture* t)
: Animation(name, t, 1, 0, 1, 1) {}

Animation::Animation(
        const std::string& name,
        SDL_Texture* t,
        size_t frameCount,
        size_t speed,
        int rows,
        int cols
) : 
    m_texture(t),
    m_frameCount(frameCount),
    m_currentFrame(0),
    m_speed(speed),
    m_name(name),
    m_rows(rows),
    m_cols(cols),
    m_currentRow(0),
    m_currentCol(0)
{
    m_size = Vec2((float)getTextureSize().x / cols, (float)getTextureSize().y / rows);
    setSrcRect( 0, 0, (int)m_size.x, (int)m_size.y );
}

Animation::Animation(const std::string& name, SDL_Texture* t, size_t frameCount, size_t speed, int rows, int cols, int width, int height)
{
    m_texture = t;
    m_frameCount = rows*cols;
    m_currentFrame = 0;
    m_speed = speed;
    m_name = name;
    m_rows = rows;
    m_cols = cols;
    m_currentRow = 0;
    m_currentCol = 0;    
    m_size = Vec2((float)getTextureSize().x / cols, (float)getTextureSize().y / rows);

    setSrcRect( 0, 0, (int)m_size.x, (int)m_size.y );

}

// update the animation to show the next frame, depending on its speed
// animation loops when it reaches the end
void Animation::update(size_t currentFrame) {
    m_currentFrame++;
    size_t animFrame = (currentFrame / m_speed) % m_frameCount;
    setSrcRect(
        (int)((m_currentCol + animFrame*(m_cols/m_frameCount)) * m_size.x),
        m_currentRow * (int)m_size.y,
        (int)m_size.x,
        (int)m_size.y
    );
}

void Animation::setRow(int row){
    m_currentRow = row;
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

float Animation::getAngle() const
{
    return m_angle;
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

void Animation::setSrcSize(Vec2 size)   
{
    m_srcRect.w = size.x;
    m_srcRect.h = size.y;
}

void Animation::setDestRect(Vec2 pos)   
{
    m_destRect.x = (int)pos.x;
    m_destRect.y = (int)pos.y;
}

void Animation::setDestSize(Vec2 size)   
{
    m_destRect.w = (int)size.x;
    m_destRect.h = (int)size.y;
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

void Animation::setTile(Vec2 grid) {
    m_currentCol = (int)grid.x;
    m_currentRow = (int)grid.y;
}

SDL_Point Animation::getTextureSize()
{
    SDL_Point size;
    SDL_QueryTexture(m_texture, NULL, NULL, &size.x, &size.y);
    return size;
}

void Animation::setAngle(float angle) {
    m_angle = angle;
}

void Animation::setScale(Vec2 scale) {
    m_scale = scale;
    m_destRect.w = m_srcRect.w * (int)scale.x;
    m_destRect.h = m_srcRect.h * (int)scale.y;
}

void Animation::setCurrentFrame(size_t frame){
    m_currentFrame = frame;
}

size_t Animation::frames(){
    return m_frameCount;
}

Vec2 Animation::getRowColumn() const {
    return Vec2{(float)m_currentCol, (float)m_currentRow};
}

Vec2 Animation::getShape() const {
    return Vec2{(float)m_cols, (float)m_rows};
}