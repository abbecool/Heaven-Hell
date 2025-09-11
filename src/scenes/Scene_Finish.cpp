#include "scenes/Scene_Finish.h"
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

Scene_Finish::Scene_Finish(Game* game)
    : Scene(game)
{
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_t, "TOGGLE_TEXTURE");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    registerAction(SDL_BUTTON_LEFT , "MOUSE LEFT CLICK");
    registerAction(SDLK_v , "SHOW COORDINATES");

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
            auto view = m_ECS.View<CCollisionBox, CTransform, CAnimation, CText>();
            for (auto e : view)
            {
                auto &transform = m_ECS.getComponent<CTransform>(e);
                auto &collision = m_ECS.getComponent<CCollisionBox>(e);
                auto &animation = m_ECS.getComponent<CAnimation>(e);
                if (m_physics.PointInRect(m_mousePosition, transform.pos, collision.size))
                {
                    animation.animation = getAnimation("button_pressed");
                }
            }
        }   
    }
    else if (action.type() == "END")
    {
        if (action.name() == "MOUSE LEFT CLICK")
        {
            auto view = m_ECS.View<CCollisionBox, CTransform, CName>();
            for (auto e : view)
            {
                auto &transform = m_ECS.getComponent<CTransform>(e);
                auto &collision = m_ECS.getComponent<CCollisionBox>(e);
                if (!m_physics.PointInRect(m_mousePosition, transform.pos, collision.size))
                {
                    continue;
                }
                auto &name = m_ECS.getComponent<CName>(e).name;
                auto &animation = m_ECS.getComponent<CAnimation>(e);
                animation.animation = getAnimation("button_unpressed");
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
    auto view = m_ECS.View<CAnimation>();
    for ( auto e : view){
        m_ECS.getComponent<CAnimation>(e).animation.update(m_currentFrame);
    }
}

void Scene_Finish::sRender()
{
    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
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
