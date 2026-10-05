#pragma once

#include "ecs/Components.hpp"
#include "scenes/Scene.hpp"
#include <memory>

class Scene_GameOver : public Scene
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

    void loadGameOver();

    void spawnLevel(const Vec2 pos, const std::string tile);

    void sAnimation();
    void sRender();

    void sDoAction(const Action&) override;
    void onEnd() override;
    void setPaused(bool);

public:
    explicit Scene_GameOver(Game* game);
    void update() override;
};
