#include "Sprite.h"
#include <iostream>

SDL_Sprite::SDL_Sprite(){
    
}

SDL_Sprite::SDL_Sprite(SDL_Texture* texture)
    : texture(texture), rotation(0.0), scaleX(1.0f), scaleY(1.0f) {
    initialize();
    }

SDL_Sprite::~SDL_Sprite() {
}

void SDL_Sprite::initialize() {
    if (texture) {
        center = {destRect.w / 2.0f, destRect.h / 2.0f};
    }
}

void SDL_Sprite::setDestPosition(float x, float y) {
    destRect.x = (int)x;
    destRect.y = (int)y;
}

SDL_Rect* SDL_Sprite::getDestRect() {
    return &destRect;
}

void SDL_Sprite::setSrcPosition(float x, float y, float w, float h) {
    srcRect.x = (int)x;
    srcRect.y = (int)y;
    srcRect.w = (int)w;
    srcRect.h = (int)h;
}

SDL_Rect* SDL_Sprite::getSrcRect() {
    return &srcRect;
}

void SDL_Sprite::setRotation(double angle) {
    rotation = angle;
}

void SDL_Sprite::setScale(float scaleX, float scaleY) {
    this->scaleX = scaleX;
    this->scaleY = scaleY;
    destRect.w = srcRect.w * (int)scaleX;
    destRect.h = srcRect.h * (int)scaleY;
    center = {destRect.w / 2.0f, destRect.h / 2.0f};
}

SDL_Texture* SDL_Sprite::getTexture() {
    return texture;
}

void SDL_Sprite::render(SDL_Renderer* renderer) {
}
