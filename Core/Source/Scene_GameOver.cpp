#include "Scene_GameOver.h"
#include "Scene_Play.h"
#include "Sprite.h"
#include "Assets.h"
#include "Game.h"
#include "Components.h"
#include "Action.h"

#include "RandomArray.h"

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

Scene_GameOver::Scene_GameOver(Game* game)
    : Scene(game)
{
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_t, "TOGGLE_TEXTURE");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    registerAction(SDLK_1, "LEVEL0");
    registerAction(SDLK_2, "LEVEL5");
    registerAction(SDL_BUTTON_LEFT , "MOUSE LEFT CLICK");
    registerAction(SDLK_v , "SHOW COORDINATES");
    loadGameOver();
}

void Scene_GameOver::loadGameOver()
{
    // spawnTitleScreen
    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    entity.addComponent<CAnimation> (m_game->assets().getAnimation("dwarf"), true, 9);
    m_rendererManager.addEntityToLayer(entityId, 1);
    Vec2 midPixel = gridToMidPixel(Vec2{0, 0}, entityId);
    entity.addComponent<CTransform>(midPixel, Vec2{0, 0}, false);
    // entity.getComponent<CTransform>().scale = Vec2{0.5, 0.5};
    entity.addComponent<CName>("title_screen");

    spawnButton(Vec2{m_game->getVirtualWidth(), m_game->getVirtualHeight()}/2, "button_unpressed", "restart", "RESTART");
}

void Scene_GameOver::spawnLevel(const Vec2 pos, std::string level)
{   
    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    entity.addComponent<CAnimation> (m_game->assets().getAnimation(level), true, 9);
    entity.addComponent<CTransform>(pos,Vec2 {0, 0}, false);
    entity.addComponent<CName>(level);
}

void Scene_GameOver::spawnButton(const Vec2 pos, const std::string& unpressed, const std::string& name, const std::string& dialog)
{   
    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    entity.addComponent<CAnimation>(m_game->assets().getAnimation(unpressed), true, 5);
    m_rendererManager.addEntityToLayer(entityId, 3);
    entity.addComponent<CTransform>(pos,Vec2 {0, 0}, false);
    entity.getComponent<CTransform>().scale = Vec2{1,1};
    entity.addComponent<CBoundingBox>(entity.getComponent<CAnimation>().animation.getSize()*1);
    entity.addComponent<CName>(name);
    entity.addComponent<CDialog>(pos, entity.getComponent<CAnimation>().animation.getSize()*1, m_game->assets().getTexture(dialog), dialog);

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
            auto view = m_ECS.view<CBoundingBox>();
            for (auto e : view)
            {
                auto &transform = m_ECS.getComponent<CTransform>(e);
                auto &Bbox = m_ECS.getComponent<CBoundingBox>(e);

                if ( m_mousePosition.x < transform.pos.x + Bbox.halfSize.x && m_mousePosition.x >= transform.pos.x -Bbox.halfSize.x ){
                    if ( m_mousePosition.y < transform.pos.y + Bbox.halfSize.y && m_mousePosition.y >= transform.pos.y -Bbox.halfSize.y ){
                        m_ECS.getComponent<CAnimation>(e).animation = m_game->assets().getAnimation("button_pressed");
                        m_ECS.getComponent<CDialog>(e).pos.y = m_ECS.getComponent<CDialog>(e).pos.y + 8;
                    }
                }
            }
        }   
    }

    else if (action.type() == "END")
    {
        if (action.name() == "MOUSE LEFT CLICK")
        {
            auto view = m_ECS.view<CBoundingBox>();
            for (auto e : view)
            {
                auto &transform = m_ECS.getComponent<CTransform>(e);
                auto &Bbox = m_ECS.getComponent<CBoundingBox>(e);
                auto &name = m_ECS.getComponent<CName>(e).name;
                if ( m_mousePosition.x >= transform.pos.x + Bbox.halfSize.x || m_mousePosition.x < transform.pos.x -Bbox.halfSize.x )
                {
                    continue;
                }
                if ( m_mousePosition.y >= transform.pos.y + Bbox.halfSize.y || m_mousePosition.y < transform.pos.y -Bbox.halfSize.y )
                {
                    continue;
                }
                m_ECS.getComponent<CAnimation>(e).animation = m_game->assets().getAnimation("button_unpressed");
                m_ECS.getComponent<CDialog>(e).pos.y = m_ECS.getComponent<CDialog>(e).pos.y - 8;
                if ( name == "restart" )
                {
                    m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/levelStartingArea.png", true), true);
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
    auto view = m_ECS.view<CAnimation>();
    for ( auto e : view){
        m_ECS.getComponent<CAnimation>(e).animation.update(m_currentFrame);
    }
}

void Scene_GameOver::sRender()
{
    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
    SDL_RenderClear(m_game->renderer());
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
