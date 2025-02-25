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
    Vec2 m_inventorySize = {4, 2};
    Vec2 m_inventoryPos = Vec2{m_game->getWidth()-m_inventorySize.x*128, 256};
    bool m_drawTextures = false;
    bool m_drawCollision = false;
    bool m_open = false;

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
    void toggleInventory();
};