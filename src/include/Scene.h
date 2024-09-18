#pragma once

#include "Action.h"
#include "ECS.h"
#include "Game.h"

#include <cstddef>
#include <map>

class Game;

typedef std::map<int, std::string> ActionMap;

class Scene
{
    protected:

    Game* m_game = nullptr;  
    ECS m_ECS;
    ActionMap m_actionMap;
    bool m_pause = false;
    bool m_hasEnded = false;
    size_t m_currentFrame = 0;
    Vec2 m_mousePosition;

    virtual void onEnd() = 0;

    public:

    Scene();
    Scene(Game* game);
    virtual ~Scene();

    virtual void update() = 0;
    virtual void sDoAction(const Action& action) = 0;
    virtual void sRender() = 0;

    virtual void doAction(const Action& action);
    void simulate(const size_t frames);
    void registerAction(int inputKey, const std::string& actionName);

    int width() const;
    int height() const;
    size_t currentFrame() const;

    bool hasEnded() const;
    ActionMap& getActionMap();

    void updateMousePosition(Vec2 pos);
    Vec2 getMousePosition();
};