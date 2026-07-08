#include "scenes/Scene_GameOver.hpp"
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

Scene_GameOver::Scene_GameOver(Game* game)
    : Scene(game)
{
    registerAction(InputCode::Escape, "QUIT");
    registerAction(InputCode::T, "TOGGLE_TEXTURE");
    registerAction(InputCode::C, "TOGGLE_COLLISION");
    registerAction(InputCode::MouseLeft, "MOUSE LEFT CLICK");
    registerAction(InputCode::V, "SHOW COORDINATES");
    loadGameOver();
}

void Scene_GameOver::loadGameOver()
{
    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    m_rendererManager.addEntityToLayer(entityId, RenderLayer::MenuControl);
    Vec2 pos = Vec2{m_game->getVirtualWidth(), m_game->getVirtualHeight()/2}/2;
    entity.addComponent<CTransform>(pos);
    Vec2 size = Vec2{512, 128};
    entity.addComponent<CName>("death_text");
    entity.addComponent<CText>("You Died!", 64, "Minecraft");

    spawnButton(
        Vec2{float(m_game->getVirtualWidth()*0.5), 
        float(m_game->getVirtualHeight()*0.75)}, 
        "button_unpressed", 
        "restart", 
        "RESTART"
    );
}

void Scene_GameOver::spawnLevel(const Vec2 pos, std::string level)
{   
    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    addSprite(entityId, level, RenderLayer::MenuBackground);
    entity.addComponent<CTransform>(pos);
    entity.addComponent<CName>(level);
}

void Scene_GameOver::sDoAction(const Action& action)
{
    if (action.type() == "START")
    {
        if (action.name() == "TOGGLE_TEXTURE")
        {
            m_drawTextures = !m_drawTextures; 
        }
        else if (action.name() == "TOGGLE_COLLISION")
        { 
            m_drawCollision = !m_drawCollision; 
        }
        else if (action.name() == "TOGGLE_GRID")
        { 
            m_drawDrawGrid = !m_drawDrawGrid;
        }
        else if (action.name() == "QUIT")
        { 
            onEnd();
        }
        else if (action.name() == "MOUSE LEFT CLICK")
        {
            for (auto [e, collider, transform, sprite, text] : m_ECS.View<CCollider, CTransform, CSprite, CText>())
            {
                if (m_physics.PointInCollider(m_mousePosition, transform, collider))
                {
                    setSprite(e, "button_pressed");
                }
            }
        }   
    }
    else if (action.type() == "END")
    {
        if (action.name() == "MOUSE LEFT CLICK")
        {
            for (auto [e, collider, transform, nameComponent] : m_ECS.View<CCollider, CTransform, CName>())
            {
                if (!m_physics.PointInCollider(m_mousePosition, transform, collider))
                {
                    continue;
                }
                auto &name = nameComponent.name;
                setSprite(e, "button_unpressed");
                if ( name == "restart" )
                {
                    std::string levelPath = "assets/images/levels/levelWorld.png";
                    m_game->changeScene(
                        "PLAY", 
                        std::make_shared<Scene_Play>(m_game, levelPath, true), 
                        true
                    );
                }
            }
        }   
    }
}

void Scene_GameOver::update() 
{
    sAnimation();
    sRender();
}

void Scene_GameOver::sAnimation() 
{
    updateAnimations();
}

void Scene_GameOver::sRender()
{
    sRenderBasic();
}

void Scene_GameOver::onEnd() 
{
    m_game->quit();
}

void Scene_GameOver::setPaused(bool pause) 
{
    m_pause = pause;
}
