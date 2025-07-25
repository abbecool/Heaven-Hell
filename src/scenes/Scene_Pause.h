#pragma once

#include "ecs/Components.h"
#include "physics/Physics.h"
#include "scenes/Scene.h"

#include <memory>

class Scene_Pause : public Scene
{

    protected:

    EntityID m_player;
    std::string m_levelPath;
    bool m_hold_CTRL = false;
    bool m_hold_CLICK = false;

    void sAnimation();
    void sRender();
    void sDragButton();

    void spawnButton(const Vec2 pos, const std::string& button_name, const std::string& name, const std::string& dialog);
    void saveLayout(const std::string& path);
    void loadLayout(const std::string& path);
    void setPaused(bool);

    void onEnd(){};
    
    public:

    Scene_Pause(Game* game);
    void update();
    void sDoAction(const Action&);
};