#include "Scene_Inventory.h"
#include "Sprite.h"
#include "Assets.h"
#include "Components.h"
#include "Action.h"

#include "RandomArray.h"

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

Scene_Inventory::Scene_Inventory(Game* game)
    : Scene(game)
{
    init();
}

void Scene_Inventory::init() {
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_e, "QUIT");
    registerAction(SDL_BUTTON_LEFT , "CLICK");

    spawnItem("campfire");
    // spawnItem("staff");
}

void Scene_Inventory::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "QUIT") { 
            onEnd(); // save inventory to savefile?
        } if (action.name() == "CLICK"){
            Mix_PlayChannel(-1, m_game->assets().getAudio("loot_pickup"), 0);
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
        Animation inventoryAnimation = m_game->assets().getAnimation("inventory_open");
        inventoryAnimation.setSrcSize(m_inventorySize*32);
        inventoryAnimation.setScale({4, 4});
        inventoryAnimation.setDestRect(m_inventoryPos + Vec2{-16, 16});
        spriteRender(inventoryAnimation);
    }
    Animation hotbar = m_game->assets().getAnimation("inventory_open");
    hotbar.setSrcSize(Vec2{4,1}*32);
    hotbar.setScale({4, 4});
    hotbar.setDestRect( Vec2{m_game->getWidth()-hotbar.getDestSize().x, 0} + Vec2{-16, 16});
    spriteRender(hotbar);

    auto view = m_ECS.view<CTransform, CAnimation>();
    auto& transformPool2 = m_ECS.getComponentPool<CTransform>();
    auto& animationPool2 = m_ECS.getComponentPool<CAnimation>();

    for (auto eID : view){
            
        auto& transform = transformPool2.getComponent(eID);
        auto& animation = animationPool2.getComponent(eID).animation;

        Vec2 adjustedPos = Vec2{m_game->getWidth()-hotbar.getDestSize().x, 0} + Vec2{64, 64} + transform.pos*128 + Vec2{-16, 16};

        animation.setScale(transform.scale);
        animation.setAngle(transform.angle);
        animation.setDestRect(adjustedPos - animation.getDestSize()/2);
        
        spriteRender(animation);
    }
}

void Scene_Inventory::spriteRender(Animation &animation)
{
    SDL_RenderCopyEx(
        m_game->renderer(), 
        animation.getTexture(), 
        animation.getSrcRect(), 
        animation.getDestRect(),
        animation.getAngle(),
        NULL,
        SDL_FLIP_NONE
    );
}

// void Scene_Inventory::sAudio(){
//     if( Mix_PlayingMusic() == 0 )
//     {
//         // Mix_PlayChannel(-1, m_game->assets().getAudio("AbbeGameTrack1"), 0);
//         // Mix_PlayMusic(m_game->assets().getMusic("AbbeGameTrack1"), -1);
//     }
// }

void Scene_Inventory::spawnItem(std::string sprite)
{
    auto entityID = m_ECS.addEntity();
    m_item = entityID;
    std::cout << entityID << std::endl;
    Vec2 pos = {(int)(entityID-1)%(int)m_inventorySize.x, (int)(entityID-1)/(int)m_inventorySize.x};
    m_ECS.addComponent<CTransform>(entityID, pos, Vec2{0,0}, Vec2{4, 4}, 0.0f, 0.0f, true);
    m_ECS.addComponent<CBoundingBox>(entityID, Vec2 {32, 32});

    m_ECS.addComponent<CAnimation>(entityID, m_game->assets().getAnimation(sprite), true, 3);
}

void Scene_Inventory::Scroll(int scroll)
{
    auto& pos = m_ECS.getComponent<CTransform>(m_item).pos;
    pos.x = (int)(pos.x+scroll)%(int)m_inventorySize.x;
    pos.x = std::min(std::max(0, (int)pos.x), (int)(m_inventorySize.x-1));
    // std::cout << "slot " << number << " : " << pos.x << " " << pos.y << "!!!" << std::endl;
    // number += scroll;
    // number = std::min(std::max(0, number), 11);
    // pos = {(int)(number)%(int)m_inventorySize.x, (int)(number)/(int)m_inventorySize.x};
}

void Scene_Inventory::onEnd() {}

void Scene_Inventory::toggleInventory()
{
    m_open = !m_open;
}
