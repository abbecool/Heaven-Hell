#pragma once

#include "ecs/Components.h"
#include "scenes/Scene.h"

#include <memory>

class Scene_Menu : public Scene
{
    struct PlayerConfig
    {
        float X, Y, CX, CY, SPEED, MAXSPEED, JUMP, GRAVITY;
        std::string WEAPON; 
    };

    protected:

    EntityID m_player;
    std::string m_levelPath;
    PlayerConfig m_playerConfig;
    Vec2 levelSize;

    void loadMenu();
    void spawnLevel(const Vec2 pos, const std::string tile);

    void sAnimation();
    void sRender();
    
    void sDoAction(const Action&);
    void onEnd();
    void setPaused(bool);

    public:
    Scene_Menu(Game* game);
    void update();
};