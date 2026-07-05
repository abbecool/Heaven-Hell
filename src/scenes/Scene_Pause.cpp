#include "scenes/Scene_Pause.hpp"
#include "scenes/Scene_Play.hpp"
#include "scenes/Scene_Menu.hpp"
#include "assets/Assets.hpp"
#include "core/Game.hpp"
#include "ecs/Components.hpp"
#include "core/Action.hpp"
#include "physics/RandomArray.hpp"
#include "external/json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>


Scene_Pause::Scene_Pause(Game* game)
    : Scene(game)
{
    registerAction(InputCode::Escape, "ESC");
    registerAction(InputCode::MouseLeft, "CLICK");
    registerAction(InputCode::T, "TOGGLE_TEXTURES");
    registerAction(InputCode::C, "TOGGLE_COLLISION");
    registerAction(InputCode::S, "SAVE_LAYOUT");
    registerAction(InputCode::LeftCtrl, "CTRL");
    loadLayout("config_files/pause_menu/button_placement.json");
}

void Scene_Pause::saveLayout(const std::string& filename) {
    json j;
    for (auto [e, dialog, collision, transform] : m_ECS.constView<CText, CCollisionBox, CTransform>()) {
        json button;
        button["label"] = dialog.text;
        button["position"]["x"] = transform.pos.x;
        button["position"]["y"] = transform.pos.y;
        j["buttons"].push_back(button);
        }
    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("Could not open button placement file: " + filename);
    }
    file << j.dump(4);
    file.close();
}

void Scene_Pause::loadLayout(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Could not open button placement file: " + filename);
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

void Scene_Pause::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "CTRL") {
            m_hold_CTRL = true;
        }
        if (action.name() == "CLICK") {
            m_hold_CLICK = true;
            for (auto [e, collision, transform, sprite, text] : m_ECS.View<CCollisionBox, CTransform, CSprite, CText>())
            {
                if (m_physics.PointInRect(m_mousePosition, transform.pos, collision.size)){
                    setSprite(e, "button_pressed");
                }
            }
        }
    }
    if (action.type() == "END") {
        if (action.name() == "CLICK") {
            m_hold_CLICK = false;
            for (auto [e, collision, transform, nameComponent] : m_ECS.constView<CCollisionBox, CTransform, CName>()){
                auto &name = nameComponent.name;
                if (m_hold_CTRL) {
                    continue;
                }
                if (!m_physics.PointInRect(m_mousePosition, transform.pos, collision.size)){
                    continue;
                }
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
        for (auto [e, collision, transform, name] : m_ECS.View<CCollisionBox, CTransform, CName>()) {
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
    updateAnimations();
}

void Scene_Pause::sRender() {
    m_game->render().fillRect({0.0f, 0.0f, 0.0f, 0.0f}, {0, 0, 0, 175});
    sRenderBasic();
}

void Scene_Pause::setPaused(bool pause) {
    m_pause = pause;
}
