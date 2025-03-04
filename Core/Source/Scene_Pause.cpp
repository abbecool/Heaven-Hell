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
    registerAction(SDLK_ESCAPE, "ESC");
    registerAction(SDL_BUTTON_LEFT , "CLICK");
    registerAction(SDLK_t, "TOGGLE_TEXTURES");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    registerAction(SDLK_s, "SAVE_LAYOUT");
    registerAction(SDLK_LCTRL, "CTRL");
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
        spawnButton(Vec2 {pos_x, pos_y}, "button_unpressed", dialog, dialog); 
    }
}

void Scene_Pause::spawnButton(const Vec2 pos, const std::string& unpressed, const std::string& name, const std::string& dialog)
{   
    EntityID entityId = m_ECS.addEntity();
    Entity entity = {entityId, &m_ECS};
    entity.addComponent<CAnimation>(m_game->assets().getAnimation(unpressed), true, 5);
    m_rendererManager.addEntityToLayer(entityId, 5);
    entity.addComponent<CTransform>(pos,Vec2 {0, 0}, false);
    float dynamic_length = (float)(dialog.length()/4);
    entity.getComponent<CTransform>().scale = Vec2{dynamic_length,1};
    entity.addComponent<CBoundingBox>(entity.getComponent<CAnimation>().animation.getSize()*Vec2{dynamic_length,1});
    entity.addComponent<CName>(name);
    entity.addComponent<CDialog>(pos, entity.getComponent<CAnimation>().animation.getSize()*Vec2{dynamic_length,1}, m_game->assets().getTexture(dialog), dialog);
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
                        } else if ( name == "Resolution" ){
                            if (m_game->getWidth() == 1920) 
                            {
                                m_game->updateResolution(640, 360);
                                m_game->setScale(1);
                            }
                            else
                            {
                                m_game->updateResolution(1920, 1080);
                                m_game->setScale(3);
                            }
                        }
                    }
                }
            }
        } else if (action.name() == "TOGGLE_TEXTURES") {
                m_drawTextures = !m_drawTextures;
        } else if ( action.name() == "TOGGLE_COLLISION") { 
            m_drawCollision = !m_drawCollision; 
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
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 175); // 50% transparent black
    SDL_RenderFillRect(m_game->renderer(), nullptr);
    sRenderBasic();
}

void Scene_Pause::setPaused(bool pause) {
    m_pause = pause;
}
