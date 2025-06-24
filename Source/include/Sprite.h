#pragma once

#include <SDL2/SDL.h>
#include <string>

class SDL_Sprite {

public:
    SDL_Sprite();
    SDL_Sprite(SDL_Texture* texture);
    ~SDL_Sprite();

    void setDestPosition(float x, float y);
    SDL_Rect* getDestRect();
    void setSrcPosition(float x, float y, float w, float h);
    SDL_Rect* getSrcRect();
    void setRotation(double angle);
    void setScale(float scaleX, float scaleY);
    SDL_Texture* getTexture();
    void render(SDL_Renderer* renderer);

private:
    SDL_Texture* texture;
    SDL_Rect srcRect;
    SDL_Rect destRect;
    double rotation;
    SDL_FPoint center;
    float scaleX, scaleY;

    void initialize();
};
