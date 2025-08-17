#pragma once

#include "ecs/ECS.hpp"
#include "scenes/Scene.h"
#include "assets/Assets.h"

#include <chrono>
#include <ctime>
#include <memory>
#include <map>

class Scene;
typedef std::map<std::string, std::shared_ptr<Scene>> SceneMap;
class Game
{   
protected:
    SDL_DisplayMode DM;
    const int VIRTUAL_WIDTH = 640;
    const int VIRTUAL_HEIGHT = 360;
    int m_width = 640;
    int m_height = 360;

    SDL_Window *m_window;
    SDL_Renderer *m_renderer;
    SceneMap m_sceneMap;
    size_t m_simulationSpeed = 1;
    Assets m_assets;
    std::string m_currentScene;
    bool m_running = true;

    int m_currentFrame;
    bool m_paused;
    int m_framerate = 60;
    int m_scale = 1;

    std::chrono::steady_clock::time_point current_frame;
    std::chrono::steady_clock::time_point next_frame;
    std::chrono::steady_clock::time_point last_fps_update;

    int frame_count = 0;
    int accumulated_frame_time = 0.0;
    
    void update();
    void setPaused(bool paused);
    void sUserInput();
    void FrametimeHandler();

public:
    Game(const std::string & pathImages, const std::string & pathText);
    void changeScene(
        const std::string& sceneName,
        std::shared_ptr<Scene> scene,
        bool endCurrentScene=false);
    void changeSceneBack(const std::string& sceneName);
    std::shared_ptr<Scene> currentScene();
    void quit();
    void run();
    bool isRunning();
    int framerate();
    SDL_Renderer* renderer(); 
    SDL_Window* window(); 
    Assets& assets(); 
    SceneMap& sceneMap();
    int getVirtualWidth();
    int getVirtualHeight();
    int getWidth();
    int getHeight();
    void setWidth(int width);
    void setHeight(int height);
    void updateResolution(int scale);
    void setScale(int scale);
    int getScale();
};