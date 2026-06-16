#include "core/SDLPlatform.hpp"

#include "core/Game.h"

#include <SDL3_image/SDL_image.h>

#include <iostream>
#include <stdexcept>

SDLPlatform::SDLPlatform(const char* title, int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        std::string error = SDL_GetError();
        SDL_Quit();
        throw std::runtime_error("Window creation failed: " + error);
    }

    SDL_Surface* icon = IMG_Load("assets/images/wizard_profile_pic.png");
    if (icon) {
        SDL_SetWindowIcon(m_window, icon);
        SDL_DestroySurface(icon);
    }
    SDL_SetWindowPosition(m_window, 0, 30);
}

SDLPlatform::~SDLPlatform()
{
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
}

SDL_Window* SDLPlatform::window()
{
    return m_window;
}

SDL_DisplayMode SDLPlatform::currentDisplayMode(int fallbackWidth, int fallbackHeight) const
{
    SDL_DisplayMode displayMode{};
    const SDL_DisplayMode* currentMode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    if (currentMode) {
        displayMode = *currentMode;
    } else {
        displayMode.w = fallbackWidth;
        displayMode.h = fallbackHeight;
    }
    return displayMode;
}

void SDLPlatform::pollEvents(Game& game)
{
    SDL_Event event;
    game.currentScene()->updateMouseScroll(0);
    while (SDL_PollEvent(&event)) {
        if (SDL_EVENT_QUIT == event.type) {
            game.quit();
        }

        ActionMap actionMap = game.currentScene()->getActionMap();
        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            SDL_Keycode key = event.key.key;
            if (actionMap.find(key) == actionMap.end()) {
                continue;
            }
            const std::string actionType = (event.type == SDL_EVENT_KEY_DOWN) ? "START" : "END";
            game.currentScene()->doAction(Action(actionMap.at(key), actionType));
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            if (actionMap.find(event.button.button) == actionMap.end()) {
                continue;
            }
            const std::string actionType = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? "START" : "END";
            game.currentScene()->doAction(Action(actionMap.at(event.button.button), actionType));
        } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            game.currentScene()->updateMousePosition(
                Vec2{float(event.motion.x), float(event.motion.y)} / game.getScale()
            );
        } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            game.currentScene()->updateMouseScroll(event.wheel.integer_y);
            if (actionMap.find(event.wheel.direction) == actionMap.end()) {
                continue;
            }
            game.currentScene()->doAction(Action(actionMap.at(event.wheel.direction), ""));
        }
    }
}

void SDLPlatform::setWindowSize(int width, int height)
{
    SDL_SetWindowSize(m_window, width, height);
}

void SDLPlatform::setFullscreen(bool enabled)
{
    SDL_SetWindowFullscreen(m_window, enabled);
}

bool SDLPlatform::isFullscreen() const
{
    return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) != 0;
}
