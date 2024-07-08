#include "Sprite.h"
#include <iostream>

SDL_Sprite::SDL_Sprite(){
    
}

SDL_Sprite::SDL_Sprite(SDL_Texture* texture)
    : texture(texture), rotation(0.0), scaleX(1.0f), scaleY(1.0f) {
    initialize();
    // srcRect = {0, 0, 0, 0};
    // destRect = {0.0f, 0.0f, 0.0f, 0.0f};
    }

SDL_Sprite::~SDL_Sprite() {
    // Do not destroy the texture as it is managed externally
}

void SDL_Sprite::initialize() {
    if (texture) {
        // std::cout << "test init 1" << std::endl;
        // std::cout << srcRect.w << std::endl;
        // srcRect.w = 64;
        // srcRect.h = 64;
        // SDL_QueryTexture(texture, NULL, NULL, &srcRect.w, &srcRect.h);
        // destRect.w = srcRect.w;
        // std::cout << "test init 2" << std::endl;
        // destRect.h = srcRect.h;
        center = {destRect.w / 2.0f, destRect.h / 2.0f};
    }
}

void SDL_Sprite::setDestPosition(float x, float y) {
    destRect.x = x;
    destRect.y = y;
}

SDL_Rect* SDL_Sprite::getDestRect() {
    return &destRect;
}

void SDL_Sprite::setSrcPosition(float x, float y, float w, float h) {
    srcRect.x = x;
    srcRect.y = y;
    srcRect.w = w;
    srcRect.h = h;
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
    destRect.w = srcRect.w * scaleX;
    destRect.h = srcRect.h * scaleY;
    center = {destRect.w / 2.0f, destRect.h / 2.0f};
}

SDL_Texture* SDL_Sprite::getTexture() {
    return texture;
}

void SDL_Sprite::render(SDL_Renderer* renderer) {
    // std::cout << destRect.w << " " << destRect.h << " " << destRect.x << " " << destRect.y << std::endl;
    // SDL_RenderCopy(renderer, texture, &srcRect, &destRect);

    // SDL_RenderCopyExF(
    //     renderer,
    //     texture,
    //     srcRect,
    //     destRect,
    //     rotation,
    //     NULL,
    //     SDL_FLIP_NONE
    // );
}


// SDL_Sprite::SDL_Sprite(SDL_Renderer* renderer, const std::string& filePath)
//     : texture(nullptr), rotation(0.0), scaleX(1.0f), scaleY(1.0f) {
//     loadTexture(renderer, filePath);
//     srcRect = {0, 0, 0, 0};
//     destRect = {0.0f, 0.0f, 0.0f, 0.0f};
// }

// SDL_Sprite::~SDL_Sprite() {
//     if (texture) {
//         SDL_DestroyTexture(texture);
//     }
// }

// void SDL_Sprite::loadTexture(SDL_Renderer* renderer, const std::string& filePath) {
//     SDL_Surface* surface = SDL_LoadBMP(filePath.c_str());
//     if (!surface) {
//         // Handle error
//         return;
//     }
//     texture = SDL_CreateTextureFromSurface(renderer, surface);
//     SDL_FreeSurface(surface);

//     if (texture) {
//         SDL_QueryTexture(texture, NULL, NULL, &srcRect.w, &srcRect.h);
//         destRect.w = static_cast<float>(srcRect.w);
//         destRect.h = static_cast<float>(srcRect.h);
//         center = {destRect.w / 2.0f, destRect.h / 2.0f};
//     }
// }

// void SDL_Sprite::setPosition(float x, float y) {
//     destRect.x = x;
//     destRect.y = y;
// }

// void SDL_Sprite::setRotation(double angle) {
//     rotation = angle;
// }

// void SDL_Sprite::setScale(float scaleX, float scaleY) {
//     this->scaleX = scaleX;
//     this->scaleY = scaleY;
//     destRect.w = srcRect.w * scaleX;
//     destRect.h = srcRect.h * scaleY;
//     center = {destRect.w / 2.0f, destRect.h / 2.0f};
// }

// void SDL_Sprite::render(SDL_Renderer* renderer) {
//     SDL_RenderCopyExF(
//         renderer,
//         texture,
//         &srcRect,
//         &destRect,
//         rotation,
//         &center,
//         SDL_FLIP_NONE
//     );
// }
