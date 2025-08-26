#include "scenes/Scene_Menu.h"
#include "scenes/Scene_Play.h"
#include "assets/Sprite.h"
#include "assets/Assets.h"
#include "core/Game.h"
#include "ecs/Components.h"
#include "core/Action.h"
#include "physics/RandomArray.h"

#include <SDL_image.h>
#include <SDL_ttf.h>
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

Scene_Menu::Scene_Menu(Game* game)
    : Scene(game)
{
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_t, "TOGGLE_TEXTURE");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    registerAction(SDLK_f, "FULLSCREEN");
    registerAction(SDL_BUTTON_LEFT , "MOUSE LEFT CLICK");
    loadMenu();
}

void Scene_Menu::loadMenu()
{
    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    entity.addComponent<CAnimation> (getAnimation("level0_screenshot"), true, 9);
    m_rendererManager.addEntityToLayer(entityId, 1);
    Vec2 midPixel = gridToMidPixel(Vec2{0, 0}, entityId);
    entity.addComponent<CTransform>(midPixel);
    entity.addComponent<CName>("title_screen");

    EntityID entityId1 = m_ECS.addEntity();
    Entity entity1 = {entityId1, &m_ECS};
    entity1.addComponent<CAnimation> (getAnimation("game_title"), true, 7);
    m_rendererManager.addEntityToLayer(entityId1, 2);
    entity1.addComponent<CTransform>(Vec2 {300, 45});
    entity1.addComponent<CName>("game_title");

    spawnButton(Vec2 {64.f, 64.f}, "button_unpressed", "new", "NEW GAME");
    spawnButton(Vec2 {64.f, 128.f}, "button_unpressed", "continue", "CONTINUE");
    spawnButton(Vec2 {float(width())-64.f,64.f }, "button_unpressed", "360p", "360p");
    spawnButton(Vec2 {float(width())-64.f,2*64.f }, "button_unpressed", "720p", "720p");
    spawnButton(Vec2 {float(width())-64.f,3*64.f }, "button_unpressed", "1080p", "1080p");
    spawnButton(Vec2 {float(width())-64.f,4*64.f }, "button_unpressed", "1440p", "1440p");
    spawnButton(Vec2 {float(width())-64.f,5*64.f }, "button_unpressed", "4K", "4K");
}

void Scene_Menu::spawnLevel(const Vec2 pos, std::string level)
{   
    EntityID id = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(id, getAnimation(level), 9);
    m_ECS.addComponent<CTransform>(id, pos);
    m_ECS.addComponent<CName>(id, level);
}

void Scene_Menu::sDoAction(const Action& action)
{
    if (action.type() == "START"){
        if (action.name() == "TOGGLE_TEXTURE"){
            m_drawTextures = !m_drawTextures; 
        }
        else if (action.name() == "TOGGLE_COLLISION"){ 
            m_drawCollision = !m_drawCollision; 
        }
        else if (action.name() == "TOGGLE_GRID"){ 
            m_drawDrawGrid = !m_drawDrawGrid;
        }
        else if (action.name() == "QUIT"){ 
            onEnd();
        }
        else if (action.name() == "FULLSCREEN"){
            m_game->ToggleFullscreen();
        }
        else if (action.name() == "MOUSE LEFT CLICK") {
            auto view = m_ECS.View<CCollisionBox, CTransform, CText, CAnimation>();
            for (auto e : view){
                auto &transform = m_ECS.getComponent<CTransform>(e);
                auto &collision = m_ECS.getComponent<CCollisionBox>(e);
                if (!m_physics.PointInRect(m_mousePosition, transform.pos, collision.size)){
                    continue;
                }
                m_ECS.getComponent<CAnimation>(e).animation = getAnimation("button_pressed");
            }
        }   
    }
    else if (action.type() == "END"){
        if (!(action.name() == "MOUSE LEFT CLICK")){
            return;
        }   
        auto view = m_ECS.View<CCollisionBox, CTransform, CText, CAnimation, CName>();
        for (auto e : view){
            auto &transform = m_ECS.getComponent<CTransform>(e);
            auto &collision = m_ECS.getComponent<CCollisionBox>(e);
            if (!m_physics.PointInRect(m_mousePosition, transform.pos, collision.size)){
                continue;
            }
            auto &name = m_ECS.getComponent<CName>(e).name;
            m_ECS.getComponent<CAnimation>(e).animation = getAnimation("button_unpressed");
            std::string levelPath = "assets/images/levels/levelWorld.png";
            if ( name == "new" ){
                m_game->changeScene(
                    "PLAY", 
                    std::make_shared<Scene_Play>(m_game, levelPath, true), 
                    true);
            }
            else if ( name == "continue" ){
                m_game->changeScene(
                    "PLAY", 
                    std::make_shared<Scene_Play>(m_game, levelPath, false), 
                    true);
            }
            else if ( name == "360p" ){
                m_game->updateResolution(1);
            }
            else if ( name == "720p" ){
                m_game->updateResolution(2);
            }
            else if ( name == "1080p" ){
                m_game->updateResolution(3);
            }
            else if ( name == "1440p" ){
                m_game->updateResolution(4);
            }
            else if ( name == "4K" ){
                m_game->updateResolution(6);
            }
        }
    }
}

void Scene_Menu::update() 
{
    sAnimation();
    sRender();
}

void Scene_Menu::sAnimation() 
{
    auto view = m_ECS.View<CAnimation>();
    for ( auto e : view){
        m_ECS.getComponent<CAnimation>(e).animation.update(m_currentFrame);
    }
}

void Scene_Menu::sRender()
{
    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
    SDL_RenderClear(m_game->renderer());
    sRenderBasic();
}

void Scene_Menu::onEnd() 
{
    m_game->quit();
}

void Scene_Menu::setPaused(bool pause) 
{
    m_pause = pause;
}
