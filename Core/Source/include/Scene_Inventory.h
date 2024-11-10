#pragma once

#include "Scene.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>

class Scene_Inventory : public Scene
{
    protected:
    EntityID m_item;
    Vec2 m_mousePosition;
    Vec2 m_inventoryPos = {0, 0};
    Vec2 m_inventorySize = {3, 1};
    bool m_drawTextures;
    bool m_drawCollision;

    void init();
    // void saveGame(const std::string& filename);

    void spawnBox(Vec2 pos, std::string sprite);
    void spawnItem(std::string sprite);

    void sScripting();
    void sMovement();
    void sCollision();
    void sStatus();
    void sAnimation();
    void sRender();
    void spriteRender(Animation &animation);
    void sAudio();

    void sDoAction(const Action&);
    void onEnd();

    public:
    Scene_Inventory(Game* play);
    void update();
    void Scroll(int scroll);
};