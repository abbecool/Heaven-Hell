#include "scenes/Scene_Menu.hpp"
#include "scenes/Scene_Play.hpp"
#include "assets/Assets.hpp"
#include "core/Game.hpp"
#include "ecs/Components.hpp"
#include "core/Action.hpp"
#include "physics/RandomArray.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

Scene_Menu::Scene_Menu(Game* game)
    : Scene(game)
{
    registerAction(InputCode::Escape, "QUIT");
    registerAction(InputCode::T, "TOGGLE_TEXTURE");
    registerAction(InputCode::C, "TOGGLE_COLLISION");
    registerAction(InputCode::F, "FULLSCREEN");
    registerAction(InputCode::MouseLeft, "MOUSE LEFT CLICK");
    loadMenu();
}

void Scene_Menu::loadMenu()
{
    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    addSprite(entityId, "level0_screenshot", RenderLayer::MenuBackground);
    Vec2 midPixel = gridToMidPixel(Vec2{0, 0}, entityId);
    entity.addComponent<CTransform>(midPixel);
    entity.addComponent<CName>("title_screen");

    EntityID entityId1 = m_ECS.addEntity();
    Entity entity1 = {entityId1, &m_ECS};
    addSprite(entityId1, "game_title", RenderLayer::MenuTitle);
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
    addSprite(id, level, RenderLayer::MenuBackground);
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
            for (auto [e, collider, transform, text, sprite] : m_ECS.View<CCollider, CTransform, CText, CSprite>()){
                if (!m_physics.PointInCollider(m_mousePosition, transform, collider)){
                    continue;
                }
                setSprite(e, "button_pressed");
            }
        }   
    }
    else if (action.type() == "END"){
        if (!(action.name() == "MOUSE LEFT CLICK")){
            return;
        }   
        for (auto [e, collider, transform, text, sprite, nameComponent] : m_ECS.View<CCollider, CTransform, CText, CSprite, CName>()){
            if (!m_physics.PointInCollider(m_mousePosition, transform, collider)){
                continue;
            }
            auto &name = nameComponent.name;
            setSprite(e, "button_unpressed");
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
    updateAnimations();
}

void Scene_Menu::sRender()
{
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
