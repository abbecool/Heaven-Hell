#pragma once

#include "ecs/Components.h"
#include "scenes/Scene.h"
#include "scenes/Scene_Menu.h"
#include <memory>

class Scene_Finish : public Scene
{
    struct PlayerConfig
    {
        float X, Y, CX, CY, SPEED, MAXSPEED, JUMP, GRAVITY;
        std::string WEAPON; 
    };

    protected:

    std::string m_levelPath;
    Vec2 levelSize;

    void spawnLevel(const Vec2 pos, const std::string tile);

    void sAnimation();
    void sRender();
    
    void sDoAction(const Action&);
    void onEnd();
    void setPaused(bool);

    public:
    Scene_Finish(Game* game);
    void update();
};