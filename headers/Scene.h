#pragma once

#include "Game.h"
#include "EntityManager.h"
#include <memory>

class Scene
{
protected:
    Game* m_game = nullptr;
    EntityManager m_entities;
    int currentFrame;
    bool m_paused = false;
    bool m_hasEnded = false;
    size_t m_currentFrame = 0;

//     virtual void onEnd() = 0;
//     void setPaused(bool paused);
// public:
//     Scene();
   
};
