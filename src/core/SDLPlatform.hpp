#pragma once

#include "core/PixelImage.hpp"
#include "render/RenderDriver.hpp"

#include <map>
#include <string>

struct MIX_Audio;
struct MIX_Mixer;
struct SDL_Window;

class Game;

struct DisplaySize {
    int w = 0;
    int h = 0;
};

class SDLPlatform {
    SDL_Window* m_window = nullptr;
    MIX_Mixer* m_mixer = nullptr;
    std::map<std::string, MIX_Audio*> m_audios;
    std::map<std::string, MIX_Audio*> m_music;

    bool ensureMixer();
    MIX_Audio* getAudio(const std::string& name) const;

public:
    SDLPlatform(
        const char* title,
        int width,
        int height,
        RenderDriver renderDriver = RenderDriver::SDLRenderer);
    ~SDLPlatform();

    SDL_Window* window();
    DisplaySize currentDisplaySize(int fallbackWidth, int fallbackHeight) const;
    void pollEvents(Game& game);
    void loadAudio(const std::string& name, const std::string& path);
    void loadMusic(const std::string& name, const std::string& path);
    void playAudio(const std::string& name);
    PixelImage loadImagePixels(const std::string& path) const;
    void setWindowSize(int width, int height);
    void setFullscreen(bool enabled);
    bool isFullscreen() const;
};
