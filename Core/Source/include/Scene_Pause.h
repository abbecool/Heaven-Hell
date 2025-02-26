#pragma once

#include "Components.h"
#include "Physics.h"
#include "Scene.h"
#include <memory>

class Scene_Pause : public Scene
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
    Physics m_physics;
    Vec2 levelSize;
    Vec2 cameraPos;
    bool cameraFollow = true;
    float cameraZoom = 1;
    bool m_drawTextures = true;
    bool m_drawCollision = false;
    bool m_drawDrawGrid = false;
    bool m_drawCoordinates = false;
    const Vec2 m_gridSize = { 64, 64 };

    void init();
    void loadPause();
    Vec2 gridToMidPixel(float, float, Entity);

    void spawnButton(const Vec2 pos, const std::string& button_name, const std::string& name, const std::string& dialog);

    void sAnimation();
    void sRender();
    
    void sDoAction(const Action&);
    void onEnd();
    void setPaused(bool);

    public:
    Scene_Pause(Game* game);
    void update();
};