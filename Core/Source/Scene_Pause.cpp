#include "Scene_Pause.h"
#include "Scene_Play.h"
#include "Scene_Menu.h"
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
    registerAction(SDLK_ESCAPE, "ESC");
    registerAction(SDL_BUTTON_LEFT , "CLICK");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    registerAction(SDLK_s, "SAVE_LAYOUT");
    registerAction(SDLK_LCTRL, "CTRL");
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
    // spawnButton(Vec2 {512.0f +0.0f,64*7.0f}, 1.f, "button_unpressed", "continue", "CONTINUE");
    // spawnButton(Vec2 {512.0f +0.0f,64*13.0f}, 2.f, "button_unpressed", "main menu", "Save_and_return_to_Main_Menu");
    
    loadLayout("config_files/pause_menu/button_placement.txt");
}

void Scene_Pause::saveLayout(const std::string& filename) {
    std::ofstream saveFile(filename);
    

    auto dialogPool = m_ECS.getComponentPool<CDialog>();    
    auto transformPool = m_ECS.getComponentPool<CTransform>();
    auto view = m_ECS.signatureView<CDialog, CTransform>();
    if (saveFile.is_open()) {
        for (auto e : view) {
            auto dialog_text = dialogPool.getComponent(e).dialog_text;
            auto pos = transformPool.getComponent(e).pos;
            saveFile << dialog_text << " " << pos.x << " " << pos.y << std::endl;
        }
        saveFile.close();
    } else {
        std::cerr << "Unable to open file for saving!" << std::endl;
    }
}

void Scene_Pause::loadLayout(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Could not load button_placement.txt file!\n";
        exit(-1);
    }
    std::string head;
    std::string dialog;
    float pos_x, pos_y;
    while (file >> dialog) {
        file >> pos_x >> pos_y;      
        std::cout << dialog << " " << pos_x << " " << pos_y << std::endl;
        spawnButton(Vec2 {pos_x, pos_y}, 2.f, "button_unpressed", dialog, dialog); 
        // } else {
        //     std::cerr << "head to " << head << "\n";
        //     std::cerr << "The config file format is incorrect!\n";
        //     exit(-1);
        // }
    }
}

void Scene_Pause::spawnButton(const Vec2 pos, const float length, const std::string& unpressed, const std::string& name, const std::string& dialog)
{   
    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    entity.addComponent<CAnimation>(m_game->assets().getAnimation(unpressed), true, 5);
    entity.addComponent<CTopLayer>();
    // Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity.addComponent<CTransform>(pos,Vec2 {0, 0}, false);
    float dynamic_length = (float)(dialog.length());
    entity.getComponent<CTransform>().scale = Vec2{dynamic_length,4};
    entity.addComponent<CBoundingBox>(entity.getComponent<CAnimation>().animation.getSize()*Vec2{dynamic_length,4});
    entity.addComponent<CName>(name);
    entity.addComponent<CDialog>(pos, entity.getComponent<CAnimation>().animation.getSize()*Vec2{dynamic_length,4}, m_game->assets().getTexture(dialog), dialog);

}

void Scene_Pause::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "CTRL") {
            m_hold_CTRL = true;
        }
        if (action.name() == "CLICK") {
            m_hold_CLICK = true;
        }
    }
    if (action.type() == "END") {
        if (action.name() == "CLICK") {
            m_hold_CLICK = false;
            auto view = m_ECS.view<CBoundingBox>();
            for (auto e : view){
                auto &transform = m_ECS.getComponent<CTransform>(e);
                auto &Bbox = m_ECS.getComponent<CBoundingBox>(e);
                auto &name = m_ECS.getComponent<CName>(e).name;
                if (m_hold_CTRL) {
                    continue;
                }
                if ( m_mousePosition.x < transform.pos.x + Bbox.halfSize.x && m_mousePosition.x >= transform.pos.x -Bbox.halfSize.x ){
                    if ( m_mousePosition.y < transform.pos.y + Bbox.halfSize.y && m_mousePosition.y >= transform.pos.y -Bbox.halfSize.y ){
                        if ( name == "CONTINUE" ){
                            if (m_game->sceneMap().find("PLAY") != m_game->sceneMap().end()) {
                                auto playScene = std::dynamic_pointer_cast<Scene_Play>(m_game->sceneMap().at("PLAY"));
                                playScene->setPaused(false);
                            }
                            m_game->changeScene("PLAY", nullptr, true);
                            
                        } else if ( name == "Save_and_return_to_Main_Menu" ){
                            m_game->changeScene("MAIN_MENU", std::make_shared<Scene_Menu>(m_game), true);
                            if (m_game->sceneMap().find("PLAY") != m_game->sceneMap().end()) {
                                m_game->sceneMap().erase("PLAY");
                            }
                        }
                    }
                }
            }
        } else if ( action.name() == "TOGGLE_COLLISION") { 
            m_drawCollision = !m_drawCollision; 
            std::cout << "Collision: " << m_drawCollision << std::endl;
        } else if ( action.name() == "ESC") { 
            m_game->changeScene("PLAY", nullptr, true);
            if (m_game->sceneMap().find("PLAY") != m_game->sceneMap().end()) {
                auto playScene = std::dynamic_pointer_cast<Scene_Play>(m_game->sceneMap().at("PLAY"));
                playScene->setPaused(false);
            }
        }
        if ( action.name() == "SAVE_LAYOUT") {
            saveLayout("config_files/pause_menu/button_placement.txt");
        }
        if (action.name() == "CTRL") {
            m_hold_CTRL = false;
        }
    }
}

void Scene_Pause::sDragButton() {
    if (m_hold_CTRL && m_hold_CLICK) {
        auto view = m_ECS.signatureView<CBoundingBox, CTransform, CName>();
        for (auto e : view) {
            auto& transform = m_ECS.getComponent<CTransform>(e);
            auto& Bbox = m_ECS.getComponent<CBoundingBox>(e);
            auto& dialog = m_ECS.getComponent<CDialog>(e);

            if (m_mousePosition.x < transform.pos.x + Bbox.halfSize.x && m_mousePosition.x >= transform.pos.x - Bbox.halfSize.x) {
                if (m_mousePosition.y < transform.pos.y + Bbox.halfSize.y && m_mousePosition.y >= transform.pos.y - Bbox.halfSize.y) {
                    transform.pos = m_mousePosition;
                    dialog.pos = m_mousePosition;
                }
            }
        }
    }
}

void Scene_Pause::update() {
    sAnimation();
    sDragButton();
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
}

void Scene_Pause::setPaused(bool pause) {
    m_pause = pause;
}
