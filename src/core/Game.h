#pragma once

#include "ecs/ECS.hpp"
#include "scenes/Scene.h"
#include "assets/Assets.h"
#include "render/RenderBackend.h"

#include <chrono>
#include <ctime>
#include <memory>
#include <map>

using seconds = std::chrono::seconds;
using milliseconds = std::chrono::milliseconds;
using nanoseconds = std::chrono::nanoseconds;
using steady_clock = std::chrono::steady_clock;

class Scene;
class SDLPlatform;
typedef std::map<std::string, std::shared_ptr<Scene>> SceneMap;
class Game
{   
protected:
    const int VIRTUAL_WIDTH = 640;
    const int VIRTUAL_HEIGHT = 360;
    int m_width = 640;
    int m_height = 360;
    int m_displayHeight = 360;

    std::unique_ptr<SDLPlatform> m_platform;
    std::unique_ptr<RenderBackend> m_renderBackend;
    SceneMap m_sceneMap;
    Assets m_assets;
    std::string m_currentScene;
    bool m_running = true;
    bool m_renderFPS = true;

    int m_currentFrame;
    bool m_paused;
    int m_framerate = 60;
    int m_scale = 1;

    std::chrono::steady_clock::time_point current_frame;
    std::chrono::steady_clock::time_point next_frame;
    std::chrono::steady_clock::time_point last_fps_update;

    int frame_count = 0;
    int accumulated_frame_time = 0;
    int average_fps = 0;

    RectF m_fpsRect = {
        static_cast<float>(m_width - 100),
        static_cast<float>(m_height - 20),
        100.0f,
        20.0f
    };
    
    void update();
    void setPaused(bool paused);
    void sUserInput();
    void FrametimeHandler();

public:
    Game(const std::string & pathImages, const std::string & pathText);
    ~Game();
    void changeScene(
        const std::string& sceneName,
        std::shared_ptr<Scene> scene,
        bool endCurrentScene=false);
    std::shared_ptr<Scene> currentScene();
    void quit();
    void run();
    bool isRunning();
    int framerate();
    RenderBackend& render();
    Assets& assets(); 
    SceneMap& sceneMap();
    int getVirtualWidth();
    int getVirtualHeight();
    int getWidth();
    int getHeight();
    void setWidth(int width);
    void setHeight(int height);
    void updateResolution(int scale);
    void ToggleFullscreen();
    void setScale(int scale);
    int getScale();
    void toggleRenderFPS();
};
