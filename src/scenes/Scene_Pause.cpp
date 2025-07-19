#include "scenes/Scene_Pause.h"
#include "scenes/Scene_Play.h"
#include "scenes/Scene_Menu.h"
#include "assets/Sprite.h"
#include "assets/Assets.h"
#include "core/Game.h"
#include "ecs/Components.h"
#include "core/Action.h"
#include "physics/RandomArray.h"
#include "external/json.hpp"
using json = nlohmann::json;

#include <SDL_image.h>
#include <SDL_ttf.h>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
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
    loadLayout("config_files/pause_menu/button_placement.json");
}

void Scene_Pause::saveLayout(const std::string& filename) {
    
    auto dialogPool = m_ECS.getComponentPool<CDialog>();    
    auto transformPool = m_ECS.getComponentPool<CTransform>();
    auto view = m_ECS.signatureView<CDialog, CCollisionBox, CTransform>();
    json j;
    for (auto e : view) {
        json button;
        button["label"] = dialogPool.getComponent(e).dialog_text;
        button["position"]["x"] = transformPool.getComponent(e).pos.x;
        button["position"]["y"] = transformPool.getComponent(e).pos.y;
        j["buttons"].push_back(button);
        }
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Could not load button_placement.json file!\n";
        exit(-1);
    }
    file << j.dump(4);
    file.close();
}

void Scene_Pause::loadLayout(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Could not load button_placement.json file!\n";
        exit(-1);
    }
    json j;
    file >> j;
    file.close();
    for (const auto& button : j["buttons"]) {
        std::string dialog = button["label"];
        float pos_x = button["position"]["x"];
        float pos_y = button["position"]["y"];
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
    entity.addComponent<CCollisionBox>(entity.getComponent<CAnimation>().animation.getSize()*Vec2{dynamic_length,1});
    entity.addComponent<CName>(name);
    entity.addComponent<CDialog>(entity.getComponent<CAnimation>().animation.getSize()*Vec2{dynamic_length,1}, m_game->assets().getTexture(dialog), dialog);
}

void Scene_Pause::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "CTRL") {
            m_hold_CTRL = true;
        }
        if (action.name() == "CLICK") {
            m_hold_CLICK = true;
            auto view = m_ECS.view<CCollisionBox>();
            for (auto e : view)
            {
                auto &transform = m_ECS.getComponent<CTransform>(e);
                auto &collision = m_ECS.getComponent<CCollisionBox>(e);
    
                if ( m_mousePosition.x < transform.pos.x + collision.halfSize.x && m_mousePosition.x >= transform.pos.x -collision.halfSize.x ){
                    if ( m_mousePosition.y < transform.pos.y + collision.halfSize.y && m_mousePosition.y >= transform.pos.y -collision.halfSize.y ){
                        m_ECS.getComponent<CAnimation>(e).animation = m_game->assets().getAnimation("button_pressed");
                        m_ECS.getComponent<CDialog>(e).offset.y = m_ECS.getComponent<CDialog>(e).offset.y + 3;
                    }
                }
            }
        }
    }
    if (action.type() == "END") {
        if (action.name() == "CLICK") {
            m_hold_CLICK = false;
            auto view = m_ECS.view<CCollisionBox>();
            for (auto e : view){
                auto &transform = m_ECS.getComponent<CTransform>(e);
                auto &collision = m_ECS.getComponent<CCollisionBox>(e);
                auto &name = m_ECS.getComponent<CName>(e).name;
                if (m_hold_CTRL) {
                    continue;
                }
                if ( m_mousePosition.x < transform.pos.x + collision.halfSize.x && m_mousePosition.x >= transform.pos.x -collision.halfSize.x ){
                    if ( m_mousePosition.y < transform.pos.y + collision.halfSize.y && m_mousePosition.y >= transform.pos.y -collision.halfSize.y ){
                        m_ECS.getComponent<CAnimation>(e).animation = m_game->assets().getAnimation("button_unpressed");
                        m_ECS.getComponent<CDialog>(e).offset.y = m_ECS.getComponent<CDialog>(e).offset.y - 3;
                        if ( name == "CONTINUE" ){
                            if (m_game->sceneMap().find("PLAY") != m_game->sceneMap().end()) {
                                auto playScene = std::dynamic_pointer_cast<Scene_Play>(m_game->sceneMap().at("PLAY"));
                                playScene->setPaused(false);
                            }
                            m_game->changeScene("PLAY", nullptr, true);
                            
                        } else if ( name == "Quit" ){
                            m_game->changeScene("MAIN_MENU", std::make_shared<Scene_Menu>(m_game), true);
                            if (m_game->sceneMap().find("PLAY") != m_game->sceneMap().end()) {
                                m_game->sceneMap().erase("PLAY");
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
            saveLayout("config_files/pause_menu/button_placement.json");
        }
        if (action.name() == "CTRL") {
            m_hold_CTRL = false;
        }
    }
}

void Scene_Pause::sDragButton() {
    if (m_hold_CTRL && m_hold_CLICK) {
        auto view = m_ECS.signatureView<CCollisionBox, CTransform, CName>();
        for (auto e : view) {
            auto& transform = m_ECS.getComponent<CTransform>(e);
            auto& collision = m_ECS.getComponent<CCollisionBox>(e);

            if (m_mousePosition.x < transform.pos.x + collision.halfSize.x && m_mousePosition.x >= transform.pos.x - collision.halfSize.x) {
                if (m_mousePosition.y < transform.pos.y + collision.halfSize.y && m_mousePosition.y >= transform.pos.y - collision.halfSize.y) {
                    transform.pos = m_mousePosition;
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
