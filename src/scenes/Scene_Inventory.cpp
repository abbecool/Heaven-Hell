#include "scenes/Scene_Inventory.hpp"
#include "assets/Assets.hpp"
#include "ecs/Components.hpp"
#include "core/Action.hpp"
#include "physics/RandomArray.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

Scene_Inventory::Scene_Inventory(Game* game)
    : Scene(game)
{
    registerAction(InputCode::Escape, "QUIT");
    registerAction(InputCode::E, "QUIT");
    registerAction(InputCode::MouseLeft, "CLICK");
    
    spawnItem("campfire");
}

void Scene_Inventory::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "QUIT") { 
            onEnd(); // save inventory to savefile?
        } if (action.name() == "CLICK"){
            m_game->playAudio("loot_pickup");
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

    for (auto [eID, transform, sprite] : m_ECS.constView<CTransform, CSprite>()){
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
    const int itemIndex = static_cast<int>(entityID - 1);
    const int columnCount = static_cast<int>(m_inventorySize.x);
    Vec2 pos = {
        static_cast<float>(itemIndex % columnCount),
        static_cast<float>(itemIndex / columnCount)
    };
    m_ECS.addComponent<CTransform>(entityID, pos);
    m_ECS.addComponent<CCollider>(entityID, Vec2 {8, 8});

    addVisual(entityID, sprite, 3);
}

void Scene_Inventory::Scroll(int scroll)
{
    auto& pos = m_ECS.getComponent<CTransform>(m_item).pos;
    const int columnCount = static_cast<int>(m_inventorySize.x);
    const int column = static_cast<int>(pos.x + static_cast<float>(scroll)) % columnCount;
    pos.x = static_cast<float>(std::clamp(column, 0, columnCount - 1));
}

void Scene_Inventory::onEnd() {}

void Scene_Inventory::toggleInventory()
{
    m_open = !m_open;
}
