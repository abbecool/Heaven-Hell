#pragma once

#include "ECS.hpp"

#include "Scene.h"
#include "Assets.h"

#include <memory>
#include <map>

class Scene;
typedef std::map<std::string, std::shared_ptr<Scene>> SceneMap;

class Game
{   
protected:
    const int WIDTH = 1920;
    const int HEIGHT = 1080;

    SDL_Window *m_window;
    SDL_Renderer *m_renderer;
    SceneMap m_sceneMap;
    size_t m_simulationSpeed = 1;
    Assets m_assets;
    std::string m_currentScene;
    bool m_running = true;

    int m_currentFrame;
    bool m_paused;
    int m_framerate = 500;
    
    void init(const std::string & pathImages, const std::string & pathText);
    void update();
    void setPaused(bool paused);

    void sUserInput();

public:
    Game(const std::string & pathImages, const std::string & pathText);
    void changeScene(
        const std::string& sceneName,
        std::shared_ptr<Scene> scene,
        bool endCurrentScene=false);
    std::shared_ptr<Scene> currentScene();
    void quit();
    void run();
    bool isRunning();
    int framerate();
    SDL_Renderer* renderer(); 
    SDL_Window* window(); 
    Assets& assets(); 
    int getWidth();
    int getHeight();
};