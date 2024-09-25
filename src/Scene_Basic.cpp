#include "Scene_Basic.h"
#include "Sprite.h"
#include "Assets.h"
#include "Game.h"
#include "Action.h"
#include "Level_Loader.h"
#include "Camera.h"
#include "ScriptableEntity.h"
#include "RandomArray.h"

#include <SDL2/SDL_image.h>
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

Scene_Basic::Scene_Basic(Game* game, std::string levelPath, bool newGame)
    : Scene(game), m_levelPath(levelPath), m_newGame(newGame)
{
    init(m_levelPath);
    m_camera.calibrate(Vec2 {(float)width(), (float)height()}, m_levelSize, m_gridSize);
}

void Scene_Basic::init(const std::string& levelPath) {
    registerAction(SDLK_w, "UP");
    registerAction(SDLK_s, "DOWN");
    registerAction(SDLK_a, "LEFT");
    registerAction(SDLK_d, "RIGHT");
    registerAction(SDL_BUTTON_LEFT , "ATTACK");
    registerAction(SDLK_LSHIFT, "SHIFT");
    registerAction(SDLK_LCTRL, "CTRL");
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_f, "CAMERA FOLLOW");
    registerAction(SDLK_z, "CAMERA PAN");
    registerAction(SDLK_PLUS, "ZOOM IN");
    registerAction(SDLK_MINUS, "ZOOM OUT");
    registerAction(SDLK_r, "RESET");
    registerAction(SDLK_p, "PAUSE");
    registerAction(SDLK_t, "TOGGLE_TEXTURE");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    registerAction(SDLK_0, "LEVEL0");
    registerAction(SDLK_1, "LEVEL1");
    registerAction(SDLK_5, "LEVEL5");
    registerAction(SDLK_u, "SAVE");

    // spawnPlayer();
    spawnObstacle(Vec2 {64*2, 64*2}, false, 7);
    // spawnCoin(Vec2{64*15,64*12}, 4);
}

void Scene_Basic::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "TOGGLE_TEXTURE") {
            m_drawTextures = !m_drawTextures; 
        } else if (action.name() == "TOGGLE_COLLISION") { 
            m_drawCollision = !m_drawCollision; 
        } else if (action.name() == "TOGGLE_GRID") { 
            m_drawDrawGrid = !m_drawDrawGrid; 
        } else if (action.name() == "PAUSE") { 
            setPaused(!m_pause);
        } else if (action.name() == "QUIT") { 
            onEnd();
        } else if (action.name() == "ZOOM IN"){
            m_camera.setCameraZoom(Vec2 {2, 2});
        } else if (action.name() == "ZOOM OUT"){
            m_camera.setCameraZoom(Vec2{0.5, 0.5});
        } else if (action.name() == "CAMERA FOLLOW"){
            m_camera.toggleCameraFollow();
        } else if (action.name() == "CAMERA PAN"){
            m_pause = m_camera.startPan(2048, 1000, Vec2 {(float)(64*52+32 - width()/2), (float)(64*44+32 - height()/2)}, m_pause);
        // } else if (action.name() == "SAVE"){
        // } else if (action.name() == "LEVEL0") { 
        //     m_game->changeScene("PLAY", std::make_shared<Scene_Basic>(m_game, "assets/images/levels/level0.png", true));
        // } else if (action.name() == "LEVEL1") { 
        //     m_game->changeScene("PLAY", std::make_shared<Scene_Basic>(m_game, "assets/images/levels/level1.png", true));
        // } else if (action.name() == "LEVEL5") { 
        //     m_game->changeScene("PLAY", std::make_shared<Scene_Basic>(m_game, "assets/images/levels/level5.png", true));
        // } else if (action.name() == "RESET") { 
        //     m_game->changeScene("PLAY", std::make_shared<Scene_Basic>(m_game, "assets/images/levels/level0.png", true));
        }

        // if (action.name() == "UP") {
        //     m_ECS.getComponent<CInputs>(m_player).up = true;
        // }
        // if (action.name() == "DOWN") {
        //     m_ECS.getComponent<CInputs>(m_player).down = true; 
        // }
        // if (action.name() == "LEFT") {
        //     m_ECS.getComponent<CInputs>(m_player).left = true;
        // }
        // if (action.name() == "RIGHT") {
        //     m_ECS.getComponent<CInputs>(m_player).right = true;
        // }
        // if (action.name() == "SHIFT") {
        //     m_ECS.getComponent<CInputs>(m_player).shift = true;
        // }
        // if (action.name() == "CTRL") {
        //     m_ECS.getComponent<CInputs>(m_player).ctrl = true;
        // }
        // if (action.name() == "ATTACK"){
        //     if ( m_player->getComponent<CInputs>().canShoot ){
        //         m_player->getComponent<CInputs>().shoot = true;
        //         spawnProjectile(m_player->getLinkEntity(), getMousePosition()-m_player->getLinkEntity()->getComponent<CTransform>().pos+m_camera.position);
        //     }
        // }
    }
    else if (action.type() == "END") {
        // if (action.name() == "DOWN") {
        //     m_ECS.getComponent<CInputs>(m_player).down = false;
        // } if (action.name() == "UP") {
        //     m_ECS.getComponent<CInputs>(m_player).up = false;
        // } if (action.name() == "LEFT") {
        //     m_ECS.getComponent<CInputs>(m_player).left = false;
        // } if (action.name() == "RIGHT") {
        //     m_ECS.getComponent<CInputs>(m_player).right = false;
        // } if (action.name() == "SHIFT") {
        //     m_ECS.getComponent<CInputs>(m_player).shift = false;
        // } if (action.name() == "CTRL") {
        //     m_ECS.getComponent<CInputs>(m_player).ctrl = false;
        // } if (action.name() == "ATTACK") {
        //     m_ECS.getComponent<CInputs>(m_player).canShoot = false;
        // }
    }
}

void Scene_Basic::update() {
    m_ECS.update();
    // m_pause = m_camera.update(m_ECS.getComponent<CTransform>(m_player).pos, m_pause);
    m_pause = m_camera.update(Vec2{0,0}, m_pause);
    if (!m_pause) {
        // sMovement();
        m_currentFrame++;
        // if ( m_player->hasComponent<CScript>() ) {
        //     auto& sc = m_player->getComponent<CScript>();
        //     if ( !sc.Instance )
        //     {
        //             sc.Instance = sc.InstantiateScript();
        //         sc.Instance->m_entity = m_player;
        //         sc.Instance->OnCreateFunction();
        //     }
        //     sc.Instance->OnUpdateFunction();
        //     // memory leak, destroy 
        // }
        // sCollision();
        // sAnimation();
    }
    sRender();
}

void Scene_Basic::sMovement() {
    // // TODO: make a view reference
    // auto view = m_ECS.view<CTransform, CInputs>();
    // for (auto e : view){    
    //     auto &transform = m_ECS.getComponent<CTransform>(e);
    //     auto &inputs = m_ECS.getComponent<CInputs>(e);

    //     if ( e == m_player ){
    //         transform.vel = { 0,0 };
    //         if (inputs.up){
    //             transform.vel.y--;
    //         } if (inputs.down){
    //             transform.vel.y++;
    //         } if (inputs.left){
    //             transform.vel.x--;
    //         } if (inputs.right){
    //             transform.vel.x++;
    //         } if (inputs.shift){
    //             transform.tempo = 0.5f;
    //         } else if (inputs.ctrl){
    //             transform.tempo = 3.0f;
    //         } else{
    //             transform.tempo = 1.0f;
    //         }
    //     }

    //     transform.prevPos = transform.pos;
    //     if (!(transform.vel.isnull()) && transform.isMovable ){
    //         transform.pos += transform.vel.norm(transform.tempo*transform.speed/m_game->framerate());
    //     }
    // }
}

void Scene_Basic::sCollision() {
// ------------------------------- Player collisions -------------------------------------------------------------------------

    // Entity player = {m_player, &m_ECS};
    // auto &view = m_ECS.view<CBoundingBox>();
    // for (auto e : view ){
    //     Entity entity = {e, &m_ECS};
    //     if ( m_physics.isCollided(player, entity) )
    //     {
    //         player.getComponent<CTransform>().pos += m_physics.overlap(player,entity);
    //     }
    // }
}

void Scene_Basic::sAnimation() {
    // auto view = m_ECS.view<CTransform, CState>();
    // for ( auto e : view ){
    //     if( view.getComponent<CTransform>(e).vel.isnull() ) {
    //         changePlayerStateTo(e, PlayerState::STAND);
    //     } else if( view.getComponent<CTransform>(e).vel.mainDir().x > 0 ) {
    //         changePlayerStateTo(e, PlayerState::RUN_RIGHT);
    //     } else if(view.getComponent<CTransform>(e).vel.mainDir().x < 0) {
    //         changePlayerStateTo(e, PlayerState::RUN_LEFT);
    //     } else if(view.getComponent<CTransform>(e).vel.mainDir().y > 0) {
    //         changePlayerStateTo(e, PlayerState::RUN_DOWN);
    //     } else if(view.getComponent<CTransform>(e).vel.mainDir().y < 0) {
    //         changePlayerStateTo(e, PlayerState::RUN_UP);
    //     }
    //     // // change player animation
    //     if (view.getComponent<CState>(e).changeAnimate) {
    //         m_ECS.getComponent<CAnimation>(e).animation.setRow((int)m_ECS.getComponent<CState>(e).state);
    //     }
    // }

    // auto &view2 = m_ECS.view<CAnimation>();
    // for ( auto e : view2 ){
    //     // auto& animation = m_ECS.getComponent<CAnimation>(e);
    //     auto& animation = view2.getComponent(e);
    //     if (animation.animation.hasEnded() && !animation.repeat) {
    //         // if (m_ECS.getComponent<CName>(e).name == "Dragon") {
    //         //     m_ECS.addComponent<CAnimation>(e, m_game->assets().getAnimation("snoring_dragon"), true);
    //         // } else {
    //         //     // e->kill();
    //         // }
    //     }
    //     animation.animation.update();
    // }
}

void Scene_Basic::sRender() {
    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
    SDL_RenderClear(m_game->renderer());

    if (m_drawTextures){
        // auto viewSorted = m_ECS.view_sorted<CAnimation>();
        auto view = m_ECS.view<CTransform, CAnimation>();
        for (auto e : view){
                
            auto& transform = view.getComponent<CTransform>(e);
            auto& animation = view.getComponent<CAnimation>(e).animation;

            // Adjust the entity's position based on the camera position
            Vec2 adjustedPos = transform.pos - m_camera.position;
            // if (cameraZoom != 1) {

            // }

            animation.setScale(transform.scale*cameraZoom);
            animation.setAngle(transform.angle);
            animation.setDestRect(adjustedPos - animation.getDestSize()/2);
            spriteRender(animation);
        }

    }

    if (m_drawCollision){
        auto view = m_ECS.view<CTransform, CBoundingBox>();
        for (auto e : view){      
            auto transform = view.getComponent<CTransform>(e);
            auto box = view.getComponent<CBoundingBox>(e);

            // Adjust the collision box position based on the camera position
            SDL_Rect collisionRect;
            collisionRect.x = static_cast<int>(transform.pos.x - box.halfSize.x - m_camera.position.x);
            collisionRect.y = static_cast<int>(transform.pos.y - box.halfSize.y - m_camera.position.y);
            collisionRect.w = static_cast<int>(box.size.x);
            collisionRect.h = static_cast<int>(box.size.y);

            SDL_SetRenderDrawColor(m_game->renderer(), 255, 255, 255, 255);
            SDL_RenderDrawRect(m_game->renderer(), &collisionRect);
        }
    }
}

void Scene_Basic::spriteRender(Animation &animation){
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

// void Scene_Basic::spawnPlayer(){

//     auto entityID = m_ECS.addEntity();
//     m_player = entityID;

//     Vec2 pos = Vec2{64*4, 64*4};

//     m_ECS.addComponent<CTransform>(entityID, pos, Vec2{0,0}, Vec2{4, 4}, 0, m_playerConfig.SPEED, true);
//     m_ECS.addComponent<CBoundingBox>(entityID, Vec2 {32, 32});

//     m_ECS.addComponent<CAnimation>(entityID, m_game->assets().getAnimation("demon"), true, 3);

//     m_ECS.addComponent<CInputs>(entityID);
//     m_ECS.addComponent<CState>(entityID, PlayerState::STAND);
// }

void Scene_Basic::spawnObstacle(const Vec2 pos, bool movable, const int frame){
    auto entity_old = m_ECS.addEntity();
    m_ECS.addComponent<CTransform>(entity_old, pos, Vec2 {0, 0}, Vec2 {4,4}, 0, movable);
    m_ECS.addComponent<CAnimation>(entity_old, m_game->assets().getAnimation("coin"), true, 10);
    // m_ECS.getComponent<CAnimation>(entity_old).animation.setTile(Vec2{(float)(7 % 4), (float)(int)(7 / 4)});  
    m_ECS.addComponent<CBoundingBox>(entity_old, Vec2 {64, 64});

    EntityID entityId = m_EntityManager.addEntity();
    // Entity Entity = {entityId, &m_EntityManager};
}

// void Scene_Basic::spawnCoin(Vec2 pos, const size_t layer)
// {
//     auto entity = m_ECS.addEntity();
//     m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("coin"), true);
//     m_ECS.addComponent<CTransform>(entity, pos, Vec2{0,0}, Vec2{4,4}, 0, false);
//     m_ECS.addComponent<CBoundingBox>(entity, Vec2{32, 32});
// }

// void Scene_Basic::changePlayerStateTo(EntityID entity, PlayerState s) {
//     auto& prev = m_ECS.getComponent<CState>(entity).preState; 
//     if (prev != s) {
//         prev = m_ECS.getComponent<CState>(entity).state;
//         m_ECS.getComponent<CState>(entity).state = s; 
//         m_ECS.getComponent<CState>(entity).changeAnimate = true;
//     }
//     else { 
//         m_ECS.getComponent<CState>(entity).changeAnimate = false;
//     }
// }

void Scene_Basic::onEnd() {
    m_game->quit();
    // m_game->changeScene("Menu", std::make_shared<Scene_Menu>(m_game));
}

void Scene_Basic::setPaused(bool pause) {
    m_pause = pause;
}

Vec2 Scene_Basic::gridSize(){
    return m_gridSize;
}

Vec2 Scene_Basic::levelSize(){
    return m_levelSize;
}