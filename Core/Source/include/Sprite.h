#pragma once

#include <SDL2/SDL.h>
#include <string>

class SDL_Sprite {

public:
    SDL_Sprite();
    SDL_Sprite(SDL_Texture* texture);
    ~SDL_Sprite();

    void setDestPosition(int x, int y);
    SDL_Rect* getDestRect();
    void setSrcPosition(int x, int y, int w, int h);
    SDL_Rect* getSrcRect();
    void setRotation(float angle);
    void setScale(float scaleX, float scaleY);
    SDL_Texture* getTexture();
    void render(SDL_Renderer* renderer);

private:
    SDL_Texture* texture;
    SDL_Rect srcRect;
    SDL_Rect destRect;
    float rotation;
    SDL_FPoint center;
    float scaleX, scaleY;

    void initialize();
};
