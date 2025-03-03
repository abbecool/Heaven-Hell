#pragma once

#include "Components.h"
#include "Physics.h"
#include "Scene.h"
#include <memory>

class Scene_Pause : public Scene
{

    protected:

    EntityID m_player;
    std::string m_levelPath;
    Vec2 cameraPos;
    bool cameraFollow = true;
    float cameraZoom = 1;
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