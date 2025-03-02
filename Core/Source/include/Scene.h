#pragma once

#include "Action.h"
#include "ECS.hpp"
#include "Renderer.hpp"
#include "Game.h"

#include <cstddef>
#include <map>

class Game;

typedef std::map<int, std::string> ActionMap;

struct MouseState {
    Vec2 pos = {0,0};
    int scroll = 0;
};

class Scene
{
    protected:

    Game* m_game = nullptr;  
    ActionMap m_actionMap;
    bool m_pause = false;
    bool m_hasEnded = false;
    size_t m_currentFrame = 0;
    Vec2 m_mousePosition;
    int m_mouseScroll;
    MouseState m_mouseState;

    virtual void onEnd() = 0;

    public:
    ECS m_ECS;
    RendererManager m_rendererManager;

    Scene();
    Scene(Game* game);
    virtual ~Scene();

    virtual void update() = 0;
    virtual void sDoAction(const Action& action) = 0;
    virtual void sRender() = 0;

    virtual void doAction(const Action& action);
    void registerAction(int inputKey, const std::string& actionName);

    int width() const;
    int height() const;
    size_t currentFrame() const;

    bool hasEnded() const;
    ActionMap& getActionMap();

    void updateMousePosition(Vec2 pos);
    void updateMouseScroll(int scroll);

    Vec2 getMousePosition();
    MouseState getMouseState();
    // void DestroySubScene();
};
