#include "Scene_Pause.h"
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

Scene_Pause::Scene_Pause(Game* game)
    : Scene(game)
{
    init();
}

void Scene_Pause::init() {
    registerAction(SDLK_ESCAPE, "QUIT");
    loadPause();
}

Vec2 Scene_Pause::gridToMidPixel(float gridX, float gridY, Entity entity) {
    Vec2 offset;
    Vec2 grid = Vec2{gridX, gridY};
    Vec2 eSize;
    if ( entity.hasComponent<CAnimation>() ){
        eSize = entity.getComponent<CAnimation>().animation.getSize();
    } else {
        eSize = m_gridSize/2;
    }
    
    Vec2 eScale;
    switch ((int)eSize.y) {
        case 270:
            eScale.x = 0.15f;
            eScale.y = 0.18f;
            break;
        case 225:
            eScale.x = 0.18f;
            eScale.y = 0.18f;
            break;
        case 192:
            eScale.x = 1;
            eScale.y = 1;
            eSize.x = 64;
            eSize.y = 64;
            break;
        case 128:
            eScale.x = 2;
            eScale.y = 2;
            eSize.x = 32;
            eSize.y = 32;
            break;
        case 64:
            eScale.x = 1.0f;
            eScale.y = 1.0f;
            break;
        case 32:
            eScale.x = 2.0f;
            eScale.y = 2.0f;
            break;
        case 16:
            eScale.x = 4.0f;
            eScale.y = 4.0f;
            break;
        case 24:
            eScale.x = 2.0f;
            eScale.y = 2.0f;
            break; 
        default:
            eScale.x = 1.0f;
            eScale.y = 1.0f;
    }
    
    offset = (m_gridSize - eSize * eScale) / 2.0;

    return grid + m_gridSize / 2 - offset;
}

void Scene_Pause::loadPause(){

    // EntityID entityId1 = m_ECS.addEntity();
    // Entity entity1 = {entityId1, &m_ECS};
    // entity1.addComponent<CAnimation> (m_game->assets().getAnimation("game_title"), true, 7);
    // entity1.addComponent<CTopLayer>();
    // entity1.addComponent<CTransform>(Vec2 {1250, 180},Vec2 {0, 0}, false);
    // entity1.getComponent<CTransform>().scale = Vec2{1.2f, 1.2f};
    // entity1.addComponent<CName>("game_title");

    spawnButton(Vec2 {512.0f +0.0f,64*7.0f +38.4f}, "button_unpressed", "new", "NEW GAME");
    spawnButton(Vec2 {512.0f +0.0f,64*9.0f +12.8f}, "button_unpressed", "continue", "CONTINUE");
}

void Scene_Pause::spawnButton(const Vec2 pos, const std::string& unpressed, const std::string& name, const std::string& dialog)
{   
    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    entity.addComponent<CAnimation>(m_game->assets().getAnimation(unpressed), true, 5);
    entity.addComponent<CTopLayer>();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity.addComponent<CTransform>(midGrid,Vec2 {0, 0}, false);
    entity.getComponent<CTransform>().scale = Vec2{4,4};
    entity.addComponent<CBoundingBox>(entity.getComponent<CAnimation>().animation.getSize()*4);
    entity.addComponent<CName>(name);
    entity.addComponent<CDialog>(midGrid, entity.getComponent<CAnimation>().animation.getSize()*4, m_game->assets().getTexture(dialog));

}

void Scene_Pause::sDoAction(const Action& action) {
    if (action.name() == "QUIT") {
        onEnd();
    }
}

void Scene_Pause::update() {
    sAnimation();
    sRender();
}

void Scene_Pause::sAnimation() {
    auto view = m_ECS.view<CAnimation>();
    for ( auto e : view){
        m_ECS.getComponent<CAnimation>(e).animation.update(m_currentFrame);
    }
}

void Scene_Pause::sRender() {
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 200); // 50% transparent black
    // SDL_Rect fullscreenRect = {0, 0, m_game->windowSize().x, m_game->windowSize().y};
    SDL_RenderFillRect(m_game->renderer(), nullptr);
    if (m_drawTextures){
        auto view = m_ECS.view<CBottomLayer>();
        for (auto e : view)
        {        
            auto& transform = m_ECS.getComponent<CTransform>(e);
            auto& animation = m_ECS.getComponent<CAnimation>(e).animation;

            // Adjust the entity's position based on the camera position
            Vec2 adjustedPos = transform.pos;

            // Set the destination rectangle for rendering
            animation.setScale(transform.scale*cameraZoom);
            animation.setDestRect(adjustedPos - animation.getDestSize()/2);
            animation.setAngle(transform.angle);
            
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
        auto view1 = m_ECS.view<CTopLayer>();
        for (auto e : view1)
        {        
            auto& transform = m_ECS.getComponent<CTransform>(e);
            auto& animation = m_ECS.getComponent<CAnimation>(e).animation;

            // Adjust the entity's position based on the camera position
            Vec2 adjustedPos = transform.pos;

            // Set the destination rectangle for rendering
            animation.setScale(transform.scale*cameraZoom);
            animation.setDestRect(adjustedPos - animation.getDestSize()/2);
            animation.setAngle(transform.angle);
            
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
        auto& view2 = m_ECS.view<CDialog>();
        for (auto e : view2){
            auto& dialog = m_ECS.getComponent<CDialog>(e);
            auto& Bbox = m_ECS.getComponent<CBoundingBox>(e);

            SDL_Rect texRect;
            texRect.x = (int)(dialog.pos.x - Bbox.halfSize.x * 0.9f);
            texRect.y = (int)(dialog.pos.y - Bbox.halfSize.y * 0.8f);
            texRect.w = (int)(dialog.size.x * 0.9f);
            texRect.h = (int)(dialog.size.y * 0.8f);

            SDL_RenderCopyEx(
                    m_game->renderer(), 
                    dialog.dialog, 
                    nullptr,
                    &texRect, 
                    0,
                    NULL,
                    SDL_FLIP_NONE
                );
        }
    }

    if (m_drawCollision){
        auto& view2 = m_ECS.view<CBoundingBox>();
        for (auto e : view2){
            auto& transform = m_ECS.getComponent<CTransform>(e);
            auto& box = m_ECS.getComponent<CBoundingBox>(e);

            // Adjust the collision box position based on the camera position
            SDL_Rect collisionRect;
            collisionRect.x = static_cast<int>(transform.pos.x - box.halfSize.x);
            collisionRect.y = static_cast<int>(transform.pos.y - box.halfSize.y);
            collisionRect.w = static_cast<int>(box.size.x);
            collisionRect.h = static_cast<int>(box.size.y);

            SDL_SetRenderDrawColor(m_game->renderer(), 255, 255, 255, 255);
            SDL_RenderDrawRect(m_game->renderer(), &collisionRect);
        }
    }

    if (m_drawCoordinates) {
        std::cout << m_mousePosition.x << " " << m_mousePosition.y << std::endl;
    } 
}

void Scene_Pause::onEnd() {
    m_game->quit();
}

void Scene_Pause::setPaused(bool pause) {
    m_pause = pause;
}
