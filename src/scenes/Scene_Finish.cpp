#include "scenes/Scene_Finish.hpp"
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

Scene_Finish::Scene_Finish(Game* game)
    : Scene(game)
{
    registerAction(InputCode::Escape, "QUIT");
    registerAction(InputCode::T, "TOGGLE_TEXTURE");
    registerAction(InputCode::C, "TOGGLE_COLLISION");
    registerAction(InputCode::MouseLeft, "MOUSE LEFT CLICK");
    registerAction(InputCode::V, "SHOW COORDINATES");

    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    m_rendererManager.addEntityToLayer(entityId, 3);
    Vec2 pos = Vec2{m_game->getVirtualWidth(), m_game->getVirtualHeight()/2}/2;
    entity.addComponent<CTransform>(pos);
    Vec2 size = Vec2{512, 128};
    entity.addComponent<CName>("death_text");
    entity.addComponent<CText>("You Won!", 64, "Minecraft");

    spawnButton(
        Vec2{float(m_game->getVirtualWidth()*0.5),
        float(m_game->getVirtualHeight()*0.75)},
        "button_unpressed", 
        "restart", 
        "Return to Main menu"
    );
}

void Scene_Finish::sDoAction(const Action& action)
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
                    m_game->changeScene("MAIN_MENU", std::make_shared<Scene_Menu>(m_game));
                }
            }
        }   
    }
}

void Scene_Finish::update() 
{
    sAnimation();
    sRender();
}

void Scene_Finish::sAnimation() 
{
    updateAnimations();
}

void Scene_Finish::sRender()
{
    sRenderBasic();
}

void Scene_Finish::onEnd() 
{
    m_game->quit();
}

void Scene_Finish::setPaused(bool pause) 
{
    m_pause = pause;
}
