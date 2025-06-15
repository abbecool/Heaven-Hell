#pragma once

#include "Action.h"
#include "ECS.hpp"
#include "Renderer.hpp"
#include "Game.h"
#include "Camera.h"

#include <cstddef>
#include <map>

class Game;

typedef std::map<int, std::string> ActionMap;

struct MouseState {
    Vec2 pos = {0,0};
    int scroll = 0;
    bool hold_left_click = false;
    bool hold_right_click = false;
};

class Scene
{
    protected:

    Game* m_game = nullptr;  
    ActionMap m_actionMap;
    bool m_pause = false;
    bool m_hasEnded = false;
    size_t m_currentFrame = 0;
    bool m_restart = false;

    Camera m_camera;
    Vec2 m_mousePosition;
    int m_mouseScroll;
    MouseState m_mouseState;
    bool m_drawTextures = true;
    bool m_drawCollision = false;
    bool m_drawInteraction = false;
    bool m_drawDrawGrid = false;
    Vec2 m_gridSize = {16, 16};    
    virtual void onEnd() = 0;
    Vec2 gridToMidPixel(Vec2 grid, EntityID);
    
    public:
    ECS m_ECS;
    RendererManager m_rendererManager;
    
    Scene();
    Scene(Game* game);
    virtual ~Scene();
    
    virtual void update() = 0;
    virtual void sDoAction(const Action& action) = 0;
    
    void spriteRender(Animation &animation);
    void sRenderBasic();
    template<typename T>
    void renderBox(std::vector<EntityID> viewCollisions, ComponentPool<CTransform> transform, ComponentPool<T> box, const Vec2& screenCenterZoomed, int totalZoom);

    virtual void doAction(const Action& action);
    void registerAction(int inputKey, const std::string& actionName);
    
    int width() const;
    int height() const;
    size_t currentFrame() const;

    bool hasEnded() const;
    ActionMap& getActionMap();

    void updateMousePosition(Vec2 pos);
    void updateMouseScroll(int scroll);

    MouseState getMouseState();
    Vec2 getMousePosition();
    virtual Vec2 getCameraPosition();

};
