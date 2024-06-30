#pragma once

#include "EntityManager.h"
#include "Scene.h"
#include "../../Assets.cpp"

class Scene;
typedef std::map<std::string, std::shared_ptr<Scene>> SceneMap;

class Game
{   
protected:
    const int HEIGHT = 1080; 
    const int WIDTH = 1920; 

    SDL_Window *m_window;
    SDL_Renderer *m_renderer;
    std::string m_currentScene;
    // SceneMap m_sceneMap;
    size_t m_simulationSpeed = 1;
    Assets m_assets;
    std::string currentScene;
    bool m_running = true;

    int m_currentFrame;
    bool m_paused;
    std::shared_ptr<Entity> m_player;
    int m_speed = 400;
    int m_framerate = 60;
    EntityManager m_entities;
    // SDL_Texture *m_texture;
    // SDL_Texture *m_texture_field;
    // SDL_Texture *m_texture_releif;
    // SDL_Texture *m_texture_angel;
    // SDL_Texture *m_texture_devil;
    // SDL_Texture *m_texture_key;
    // SDL_Texture *m_texture_boulder_small;
    // SDL_Texture *m_texture_outofbound;
    
    void init(const std::string & config);
    // void update();
    void setPaused(bool paused);
    // std::shared_ptr<Scene> currentScene();

    void sMovement();
    void sUserInput();
    void sRender();
    void sCollisions();
    // void sAnimation();

    bool isCollided(std::shared_ptr<Entity>, std::shared_ptr<Entity>);
    Vec2 Overlap(std::shared_ptr<Entity>, std::shared_ptr<Entity>);
    void spawnPlayer(const Vec2 pos, const std::string name, bool movable);
    void spawnObstacle(const Vec2 pos, const Vec2 size, bool movable );
    void spawnDragon(const Vec2 pos, const Vec2 size, bool movable, const std::string &ani);
    void spawnBackground(const Vec2 pos, const Vec2 size, bool movable);
    void spawnWorldBorder(const Vec2 pos, const Vec2 size, bool movable);
    void spawnOutofboundBorder(const Vec2 pos, const Vec2 size, bool movable);
    void spawnGoal(const Vec2 pos, const Vec2 size, bool movable);
    void spawnKey(const Vec2 pos, const Vec2 size, const std::string, bool movable);
public:
    Game(const std::string & config);
    // void changeScene(
    //     const std::string& sceneName,
    //     std::shared_ptr<Scene> scene,
    //     bool endCurrentScene=false);
    void quit();
    void run();
    bool isRunning();
    SDL_Renderer* renderer(); 
    SDL_Window* window(); 
    int getWidth();
    int getHeight();
};
