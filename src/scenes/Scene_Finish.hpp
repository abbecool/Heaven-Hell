#pragma once

#include "ecs/Components.hpp"
#include "scenes/Scene.hpp"
#include "scenes/Scene_Menu.hpp"
#include <memory>

class Scene_Finish : public Scene
{
    protected:
    std::string m_levelPath;
    Vec2 levelSize;

    void sAnimation();
    void sRender();
    
    void sDoAction(const Action&);
    void onEnd();
    void setPaused(bool);

    public:
    Scene_Finish(Game* game);
    void update();
};