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

    void sDoAction(const Action&) override;
    void onEnd() override;
    void setPaused(bool);

public:
    explicit Scene_Finish(Game* game);
    void update() override;
};
