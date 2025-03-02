#include "Scene_Menu.h"
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

Scene_Menu::Scene_Menu(Game* game)
    : Scene(game)
{
    init();
}

void Scene_Menu::init() {
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_t, "TOGGLE_TEXTURE");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    registerAction(SDLK_1, "LEVEL0");
    registerAction(SDLK_2, "LEVEL5");
    registerAction(SDL_BUTTON_LEFT , "MOUSE LEFT CLICK");
    registerAction(SDLK_v , "SHOW COORDINATES");
    loadMenu();
}

Vec2 Scene_Menu::gridToMidPixel(float gridX, float gridY, Entity entity) {
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

void Scene_Menu::loadMenu(){
    // spawnTitleScreen
    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    entity.addComponent<CAnimation> (m_game->assets().getAnimation("level0_screenshot"), true, 9);
    entity.addComponent<CBottomLayer>();
    Vec2 midGrid1 = gridToMidPixel(0, 0, entity);
    entity.addComponent<CTransform>(midGrid1,Vec2 {0, 0}, false);
    entity.getComponent<CTransform>().scale = Vec2{1, 1};
    entity.addComponent<CName>("title_screen");

    EntityID entityId1 = m_ECS.addEntity();
    Entity entity1 = {entityId1, &m_ECS};
    entity1.addComponent<CAnimation> (m_game->assets().getAnimation("game_title"), true, 7);
    entity1.addComponent<CTopLayer>();
    // Vec2 midGrid2 = gridToMidPixel(1250, 180, entity);
    entity1.addComponent<CTransform>(Vec2 {1250, 180},Vec2 {0, 0}, false);
    entity1.getComponent<CTransform>().scale = Vec2{1.2f, 1.2f};
    entity1.addComponent<CName>("game_title");

    spawnButton(Vec2 {128.0f +0.0f,64*7.0f +38.4f}, "button_unpressed", "new", "NEW GAME");
    spawnButton(Vec2 {128.0f +0.0f,64*9.0f +12.8f}, "button_unpressed", "continue", "CONTINUE");
    spawnButton(Vec2 {(float)width()-196.0f,64.0f }, "button_unpressed", "720p", "720p");
    spawnButton(Vec2 {(float)width()-196.0f,196.0f }, "button_unpressed", "1080p", "1080p");
    spawnButton(Vec2 {(float)width()-196.0f,320.0f }, "button_unpressed", "1440p", "1440p");
    spawnButton(Vec2 {(float)width()-196.0f,428.0f }, "button_unpressed", "1440p Wide", "1440p Wide");
    spawnButton(Vec2 {(float)width()-196.0f,536.0f }, "button_unpressed", "1440p Super wide", "1440p Super wide");
}

void Scene_Menu::spawnLevel(const Vec2 pos, std::string level)
{   
    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    entity.addComponent<CAnimation> (m_game->assets().getAnimation(level), true, 9);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity.addComponent<CTransform>(midGrid,Vec2 {0, 0}, false);
    entity.addComponent<CName>(level);
}

void Scene_Menu::spawnButton(const Vec2 pos, const std::string& unpressed, const std::string& name, const std::string& dialog)
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

void Scene_Menu::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "TOGGLE_TEXTURE") {
            m_drawTextures = !m_drawTextures; 
        } else if (action.name() == "TOGGLE_COLLISION") { 
            m_drawCollision = !m_drawCollision; 
        } else if (action.name() == "TOGGLE_GRID") { 
            m_drawDrawGrid = !m_drawDrawGrid; 
        } else if (action.name() == "SHOW COORDINATES") { 
            m_drawCoordinates = !m_drawCoordinates; 
        } else if (action.name() == "QUIT") { 
            onEnd();
        } else if (action.name() == "MOUSE LEFT CLICK") {
            auto view = m_ECS.view<CBoundingBox>();
            for (auto e : view){
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

    else if (action.type() == "END") {
        if (action.name() == "MOUSE LEFT CLICK") {
            auto view = m_ECS.view<CBoundingBox>();
            for (auto e : view){
                auto &transform = m_ECS.getComponent<CTransform>(e);
                auto &Bbox = m_ECS.getComponent<CBoundingBox>(e);
                auto &name = m_ECS.getComponent<CName>(e).name;
                if ( m_mousePosition.x < transform.pos.x + Bbox.halfSize.x && m_mousePosition.x >= transform.pos.x -Bbox.halfSize.x ){

                    if ( m_mousePosition.y < transform.pos.y + Bbox.halfSize.y && m_mousePosition.y >= transform.pos.y -Bbox.halfSize.y ){
                        m_ECS.getComponent<CAnimation>(e).animation = m_game->assets().getAnimation("button_unpressed");
                        m_ECS.getComponent<CDialog>(e).pos.y = m_ECS.getComponent<CDialog>(e).pos.y - 8;
                        if ( name == "new" ){
                            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/levelStartingArea.png", true));
                        } else if ( name == "continue" ){
                            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/levelStartingArea.png", false));
                        } else if ( name == "720p" ){
                            SDL_SetWindowSize(m_game->window(), 1280, 720);
                            m_game->setWidth(1280);
                            m_game->setHeight(720);
                        } else if ( name == "1080p" ){
                            SDL_SetWindowSize(m_game->window(), 1920, 1080);
                            m_game->setWidth(1920);
                            m_game->setHeight(1080);
                        } else if ( name == "1440p" ){
                            SDL_SetWindowSize(m_game->window(), 2560, 1440);
                            m_game->setWidth(2560);
                            m_game->setHeight(1440);
                        } else if ( name == "1440p Wide" ){
                            SDL_SetWindowSize(m_game->window(), 3440, 1440);
                            m_game->setWidth(3440);
                            m_game->setHeight(1440);
                        } else if ( name == "1440p Super wide" ){
                            SDL_SetWindowSize(m_game->window(), 5120, 1440);
                            m_game->setWidth(5120);
                            m_game->setHeight(1440);
                        }
                    }
                }
            }
        }   
    }
}

void Scene_Menu::update() {
    sAnimation();
    sRender();
}

void Scene_Menu::sAnimation() {
    auto view = m_ECS.view<CAnimation>();
    for ( auto e : view){
        m_ECS.getComponent<CAnimation>(e).animation.update(m_currentFrame);
    }
}

void Scene_Menu::sRender() {
    
    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
    SDL_RenderClear(m_game->renderer());

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

void Scene_Menu::onEnd() {
    m_game->quit();
}

void Scene_Menu::setPaused(bool pause) {
    m_pause = pause;
}
