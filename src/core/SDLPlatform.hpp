#pragma once

#include <SDL3/SDL.h>

class Game;

class SDLPlatform {
    SDL_Window* m_window = nullptr;

public:
    SDLPlatform(const char* title, int width, int height);
    ~SDLPlatform();

    SDL_Window* window();
    SDL_DisplayMode currentDisplayMode(int fallbackWidth, int fallbackHeight) const;
    void pollEvents(Game& game);
    void setWindowSize(int width, int height);
    void setFullscreen(bool enabled);
    bool isFullscreen() const;
};
