#include "scenes/Scene_Inventory.hpp"
#include "assets/Assets.hpp"
#include "ecs/Components.hpp"
#include "core/Action.hpp"
#include "physics/RandomArray.hpp"

#include <SDL3/SDL.h>

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

Scene_Inventory::Scene_Inventory(Game* game)
    : Scene(game)
{
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_E, "QUIT");
    registerAction(SDL_BUTTON_LEFT , "CLICK");
    
    spawnItem("campfire");
}

void Scene_Inventory::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "QUIT") { 
            onEnd(); // save inventory to savefile?
        } if (action.name() == "CLICK"){
            m_game->assets().playAudio("loot_pickup");
        }
    }
}

void Scene_Inventory::update() {
    m_ECS.update();
    // m_pause = m_camera.update(m_ECS.getComponent<CTransform>(m_player).pos, m_pause);
    if (!m_pause) {
        // sScripting();
        // sMovement();
        // sStatus();
        // sCollision();
        // sAnimation();
        // sAudio();
        m_currentFrame++;
    }
    sRender();
}

void Scene_Inventory::sRender() {
    if ( m_open ) 
    {
        CSprite inventorySprite(getSprite("inventory_open"), 0);
        inventorySprite.src.w = (m_inventorySize*8).x;
        inventorySprite.src.h = (m_inventorySize*8).y;
        Vec2 pos = m_inventoryPos + Vec2{-4, 4};
        drawSprite(inventorySprite, RectF{pos.x, pos.y, inventorySprite.src.w, inventorySprite.src.h});
    }
    CSprite hotbar(getSprite("inventory_open"), 0);
    hotbar.src.w = (Vec2{4,1}*8).x;
    hotbar.src.h = (Vec2{4,1}*8).y;
    Vec2 hotbarPos = Vec2{m_game->getWidth()-hotbar.src.w, 0.0f} + Vec2{-4, 4};
    drawSprite(hotbar, RectF{hotbarPos.x, hotbarPos.y, hotbar.src.w, hotbar.src.h});

    auto view = m_ECS.View<CTransform, CSprite>();
    auto& transformPool2 = m_ECS.getComponentPool<CTransform>();
    auto& spritePool = m_ECS.getComponentPool<CSprite>();

    for (auto eID : view){
            
        auto& transform = transformPool2.getComponent(eID);
        auto& sprite = spritePool.getComponent(eID);

        Vec2 adjustedPos = Vec2{m_game->getWidth()-hotbar.src.w, 0.0f} + 
                            Vec2{16, 16} + transform.pos*32 + Vec2{-4, 4};

        Vec2 destSize = sprite.size() * transform.scale;
        drawSprite(sprite, RectF{
            adjustedPos.x - destSize.x/2,
            adjustedPos.y - destSize.y/2,
            destSize.x,
            destSize.y
        }, transform.angle);
    }
}

void Scene_Inventory::spawnItem(std::string sprite)
{
    auto entityID = m_ECS.addEntity();
    m_item = entityID;
    Vec2 pos = {(float)((int)(entityID-1)%(int)m_inventorySize.x), 
                (float)((int)(entityID-1)/(int)m_inventorySize.x)};
    m_ECS.addComponent<CTransform>(entityID, pos);
    m_ECS.addComponent<CCollisionBox>(entityID, Vec2 {8, 8});

    addVisual(entityID, sprite, 3);
}

void Scene_Inventory::Scroll(int scroll)
{
    auto& pos = m_ECS.getComponent<CTransform>(m_item).pos;
    pos.x = (int)(pos.x+scroll)%(int)m_inventorySize.x;
    pos.x = std::min(std::max(0, (int)pos.x), (int)(m_inventorySize.x-1));
}

void Scene_Inventory::onEnd() {}

void Scene_Inventory::toggleInventory()
{
    m_open = !m_open;
}
