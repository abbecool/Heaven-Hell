#include "core/SDLPlatform.hpp"

#include "core/Game.hpp"
#include "core/InputCode.hpp"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace {
std::optional<InputCode> inputCodeFromKey(SDL_Keycode key)
{
    switch (key) {
    case SDLK_W: return InputCode::W;
    case SDLK_UP: return InputCode::Up;
    case SDLK_S: return InputCode::S;
    case SDLK_DOWN: return InputCode::Down;
    case SDLK_A: return InputCode::A;
    case SDLK_LEFT: return InputCode::Left;
    case SDLK_D: return InputCode::D;
    case SDLK_RIGHT: return InputCode::Right;
    case SDLK_I: return InputCode::I;
    case SDLK_E: return InputCode::E;
    case SDLK_LSHIFT: return InputCode::LeftShift;
    case SDLK_LCTRL: return InputCode::LeftCtrl;
    case SDLK_ESCAPE: return InputCode::Escape;
    case SDLK_U: return InputCode::U;
    case SDLK_R: return InputCode::R;
    case SDLK_T: return InputCode::T;
    case SDLK_X: return InputCode::X;
    case SDLK_Z: return InputCode::Z;
    case SDLK_PLUS: return InputCode::Plus;
    case SDLK_MINUS: return InputCode::Minus;
    case SDLK_Q: return InputCode::Q;
    case SDLK_P: return InputCode::P;
    case SDLK_O: return InputCode::O;
    case SDLK_K: return InputCode::K;
    case SDLK_C: return InputCode::C;
    case SDLK_F: return InputCode::F;
    case SDLK_V: return InputCode::V;
    case SDLK_F3: return InputCode::F3;
    case SDLK_F4: return InputCode::F4;
    case SDLK_F5: return InputCode::F5;
    case SDLK_1: return InputCode::Num1;
    case SDLK_2: return InputCode::Num2;
    case SDLK_3: return InputCode::Num3;
    case SDLK_7: return InputCode::Num7;
    case SDLK_8: return InputCode::Num8;
    case SDLK_9: return InputCode::Num9;
    default: return std::nullopt;
    }
}

std::optional<InputCode> inputCodeFromMouseButton(std::uint8_t button)
{
    switch (button) {
    case SDL_BUTTON_LEFT: return InputCode::MouseLeft;
    case SDL_BUTTON_RIGHT: return InputCode::MouseRight;
    default: return std::nullopt;
    }
}
}

SDLPlatform::SDLPlatform(
    const char* title,
    int width,
    int height,
    RenderDriver renderDriver)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE;
    if (renderDriver == RenderDriver::OpenGL) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        windowFlags = static_cast<SDL_WindowFlags>(windowFlags | SDL_WINDOW_OPENGL);
    }

    m_window = SDL_CreateWindow(title, width, height, windowFlags);
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
    for (auto& [name, audio] : m_audios) {
        MIX_DestroyAudio(audio);
    }
    m_audios.clear();

    for (auto& [name, music] : m_music) {
        MIX_DestroyAudio(music);
    }
    m_music.clear();

    if (m_mixer) {
        MIX_DestroyMixer(m_mixer);
        m_mixer = nullptr;
        MIX_Quit();
    }

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

DisplaySize SDLPlatform::currentDisplaySize(int fallbackWidth, int fallbackHeight) const
{
    const SDL_DisplayMode* currentMode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    if (currentMode) {
        return DisplaySize{currentMode->w, currentMode->h};
    }
    return DisplaySize{fallbackWidth, fallbackHeight};
}

void SDLPlatform::pollEvents(Game& game)
{
    SDL_Event event;
    game.currentScene()->updateMouseScroll(0);
    while (SDL_PollEvent(&event)) {
        if (SDL_EVENT_QUIT == event.type) {
            game.quit();
        }

        ActionMap& actionMap = game.currentScene()->getActionMap();
        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            auto inputCode = inputCodeFromKey(event.key.key);
            if (!inputCode || actionMap.find(*inputCode) == actionMap.end()) {
                continue;
            }
            const std::string actionType = (event.type == SDL_EVENT_KEY_DOWN) ? "START" : "END";
            game.currentScene()->doAction(Action(actionMap.at(*inputCode), actionType));
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            auto inputCode = inputCodeFromMouseButton(event.button.button);
            if (!inputCode || actionMap.find(*inputCode) == actionMap.end()) {
                continue;
            }
            const std::string actionType = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? "START" : "END";
            game.currentScene()->doAction(Action(actionMap.at(*inputCode), actionType));
        } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
            game.currentScene()->updateMousePosition(
                Vec2{float(event.motion.x), float(event.motion.y)} / game.getScale()
            );
        } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            game.currentScene()->updateMouseScroll(event.wheel.integer_y);
            if (actionMap.find(InputCode::MouseWheel) == actionMap.end()) {
                continue;
            }
            game.currentScene()->doAction(Action(actionMap.at(InputCode::MouseWheel), ""));
        }
    }
}

bool SDLPlatform::ensureMixer()
{
    if (m_mixer) {
        return true;
    }
    if (!MIX_Init()) {
        std::cerr << "Failed to initialize SDL_mixer! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!m_mixer) {
        std::cerr << "Failed to create audio mixer! SDL_Error: " << SDL_GetError() << std::endl;
        MIX_Quit();
        return false;
    }
    return true;
}

void SDLPlatform::loadAudio(const std::string& name, const std::string& path)
{
    if (!ensureMixer()) {
        return;
    }
    std::string audioPath = "assets/audio/" + path;
    MIX_Audio* audio = MIX_LoadAudio(m_mixer, audioPath.c_str(), true);
    if (audio == nullptr) {
        std::cerr << "Failed to load audio! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    if (auto it = m_audios.find(name); it != m_audios.end()) {
        MIX_DestroyAudio(it->second);
    }
    m_audios[name] = audio;
}

void SDLPlatform::loadMusic(const std::string& name, const std::string& path)
{
    if (!ensureMixer()) {
        return;
    }
    std::string musicPath = "assets/music/" + path;
    MIX_Audio* music = MIX_LoadAudio(m_mixer, musicPath.c_str(), true);
    if (music == nullptr) {
        std::cerr << "Failed to load music! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    if (auto it = m_music.find(name); it != m_music.end()) {
        MIX_DestroyAudio(it->second);
    }
    m_music[name] = music;
}

MIX_Audio* SDLPlatform::getAudio(const std::string& name) const
{
    try {
        return m_audios.at(name);
    } catch (const std::out_of_range&) {
        std::cerr << "Audio not found: " << name << std::endl;
        throw;
    }
}

void SDLPlatform::playAudio(const std::string& name)
{
    if (!ensureMixer()) {
        return;
    }
    MIX_Audio* audio = getAudio(name);
    if (!MIX_PlayAudio(m_mixer, audio)) {
        std::cerr << "Failed to play audio: " << name << ", SDL_Error: " << SDL_GetError() << std::endl;
    }
}

PixelImage SDLPlatform::loadImagePixels(const std::string& path) const
{
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == nullptr) {
        throw std::runtime_error(path + " not loaded! SDL_Error: " + SDL_GetError());
    }

    SDL_Surface* convertedSurface = SDL_ConvertSurface(loadedSurface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loadedSurface);
    if (convertedSurface == nullptr) {
        throw std::runtime_error(path + " could not be converted! SDL_Error: " + SDL_GetError());
    }

    if (!SDL_LockSurface(convertedSurface)) {
        std::string error = SDL_GetError();
        SDL_DestroySurface(convertedSurface);
        throw std::runtime_error(path + " could not be locked! SDL_Error: " + error);
    }

    PixelImage image;
    image.width = convertedSurface->w;
    image.height = convertedSurface->h;
    image.pixels.resize(image.width * image.height);

    const std::uint8_t* pixels = static_cast<const std::uint8_t*>(convertedSurface->pixels);
    for (int y = 0; y < image.height; ++y) {
        const std::uint8_t* row = pixels + y * convertedSurface->pitch;
        for (int x = 0; x < image.width; ++x) {
            const int offset = x * 4;
            image.pixels[y * image.width + x] = PixelRGBA{
                row[offset],
                row[offset + 1],
                row[offset + 2],
                row[offset + 3]
            };
        }
    }

    SDL_UnlockSurface(convertedSurface);
    SDL_DestroySurface(convertedSurface);
    return image;
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
