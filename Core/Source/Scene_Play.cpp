#include "Scene_Menu.h"
#include "Scene_Play.h"
#include "Sprite.h"
#include "Assets.h"
#include "Game.h"
#include "Components.h"
#include "Action.h"
#include "Level_Loader.h"
#include "Camera.h"
#include "ScriptableEntity.h"
#include "player.cpp"

#include "RandomArray.h"

#include <SDL2/SDL_image.h>

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

// """
// std::map<std::string, void (Scene_Play::*)()> funcMap;
// 
// for loop dropings a map from string to spawnXXX function will be needed
// 
// """

Scene_Play::Scene_Play(Game* game, std::string levelPath, bool newGame)
    : Scene(game), m_levelPath(levelPath), m_newGame(newGame)
{
    init(m_levelPath);
    m_camera.calibrate(Vec2 {(float)width(), (float)height()}, m_levelSize, m_gridSize);
}

void Scene_Play::init(const std::string& levelPath) {
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
    loadConfig("config.txt");
    loadLevel(levelPath);
}

void Scene_Play::loadConfig(const std::string& confPath){
    std::ifstream file(confPath);
    if (!file) {
        std::cerr << "Could not load config.txt file!\n";
        exit(-1);
    }
    std::string head;
    while (file >> head) {
        if (head == "Player") {
            file >> m_playerConfig.x >> m_playerConfig.y >> m_playerConfig.SPEED >> m_playerConfig.MAXSPEED >> m_playerConfig.HP >> m_playerConfig.DAMAGE;           
        }
        else {
            std::cerr << "head to " << head << "\n";
            std::cerr << "The config file format is incorrect!\n";
            exit(-1);
        }
    }
}

// Function to save the game state to a file
void Scene_Play::saveGame(const std::string& filename) {
    std::ofstream saveFile(filename);

    if (saveFile.is_open()) {
        saveFile << "Player_pos " << (int)(m_ECS.getComponent<CTransform>(m_player).pos.x/m_gridSize.x) << " " << (int)(m_ECS.getComponent<CTransform>(m_player).pos.y/m_gridSize.y) << std::endl;
        saveFile << "Player_hp " << m_ECS.getComponent<CHealth>(m_player).HP << std::endl;
        saveFile.close();
    } else {
        std::cerr << "Unable to open file for saving!" << std::endl;
    }
}

void Scene_Play::loadLevel(const std::string& levelPath){

    const char* path = levelPath.c_str();
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (loadedSurface == nullptr) {
        std::cerr << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
    }

    // Create a texture from the surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_game->renderer() , loadedSurface);
    if (texture == nullptr) {
        std::cerr << "Unable to create texture from " << path << "! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(loadedSurface);
    }

    // Lock the surface to access the pixels
    SDL_LockSurface(loadedSurface);
    Uint32* pixels = (Uint32*)loadedSurface->pixels;
    const int HEIGHT_PIX = loadedSurface->h;
    const int WIDTH_PIX = loadedSurface->w;
    m_levelSize = Vec2{ (float)WIDTH_PIX, (float)HEIGHT_PIX };
    auto format = loadedSurface->format;
    std::vector<std::vector<std::string>> pixelMatrix = m_levelLoader.createPixelMatrix(pixels, format, WIDTH_PIX, HEIGHT_PIX);

    // Unlock and free the surface
    SDL_UnlockSurface(loadedSurface);
    SDL_FreeSurface(loadedSurface);
    SDL_DestroyTexture(texture);


    // Process the pixels
    for (int y = 0; y < HEIGHT_PIX; ++y) {
        for (int x = 0; x < WIDTH_PIX; ++x) {
            const std::string& pixel = pixelMatrix[y][x];
            std::vector<bool> neighbors = m_levelLoader.neighborCheck(pixelMatrix, pixel, x, y, WIDTH_PIX, HEIGHT_PIX);
            std::vector<std::string> neighborsTags = m_levelLoader.neighborTag(pixelMatrix, pixel, x, y, WIDTH_PIX, HEIGHT_PIX);
            int textureIndex = m_levelLoader.getObstacleTextureIndex(neighbors);
            std::unordered_map<std::string, int> tileIndex = m_levelLoader.createDualGrid(pixelMatrix, x, y, HEIGHT_PIX, WIDTH_PIX);
            spawnDualTiles(Vec2 {64*(float)x - 32, 64*(float)y - 32},  tileIndex);

            if (pixel == "obstacle") {
                spawnObstacle(Vec2 {64*(float)x, 64*(float)y}, false, textureIndex);
            }
            else{
                if (pixel == "cloud") {
                    spawnCloud(Vec2 {64*(float)x, 64*(float)y}, false, textureIndex);
                } else if (pixel == "lava") {
                    spawnLava(Vec2 {64*(float)x,64*(float)y}, "Lava", textureIndex);
                } else if (pixel == "water") {
                    spawnWater(Vec2 {64*(float)x,64*(float)y}, "Water", textureIndex);
                } else if (pixel == "bridge") {
                    if ( std::find(neighborsTags.begin(), neighborsTags.end(), "water") != neighborsTags.end() ){
                        spawnWater(Vec2 {64*(float)x,64*(float)y}, "Background", m_levelLoader.getObstacleTextureIndex(m_levelLoader.neighborCheck(pixelMatrix, "water", x, y, WIDTH_PIX, HEIGHT_PIX)));
                    } else if ( std::find(neighborsTags.begin(), neighborsTags.end(), "lava") != neighborsTags.end() ){
                        spawnLava(Vec2 {64*(float)x,64*(float)y}, "Background", m_levelLoader.getObstacleTextureIndex(m_levelLoader.neighborCheck(pixelMatrix, "lava", x, y, WIDTH_PIX, HEIGHT_PIX)));
                    }
                    spawnBridge(Vec2 {64*(float)x,64*(float)y}, textureIndex);
                }
            }
        }
    }

    spawnPlayer();
    // spawnHUD();
    spawnCoin(Vec2{64*15,64*12}, 4);
    spawnDragon(Vec2{64*52 , 64*44}, false, "snoring_dragon");
    spawnWeapon(Vec2{64*14 , 64*29});
    spawnSmallEnemy(Vec2{64*15 , 64*9}, 3);
    spawnSmallEnemy(Vec2{64*12 , 64*8}, 3);
    spawnSmallEnemy(Vec2{64*10 , 64*10}, 3);
    spawnSmallEnemy(Vec2{64*10 , 64*10}, 3);
    spawnSmallEnemy(Vec2{64*20 , 64*12}, 3);
    spawnSmallEnemy(Vec2{64*13 , 64*24}, 3);
    spawnGoal(Vec2{64*23, 64*8}, false);
    spawnGoal(Vec2{64*37, 64*47}, false);

    m_ECS.update();
    // m_ECS.sort();
}

void Scene_Play::sDoAction(const Action& action) {
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
        } else if (action.name() == "SAVE"){
            saveGame("game_save.txt");
        } else if (action.name() == "LEVEL0") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/level0.png", true));
        } else if (action.name() == "LEVEL1") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/level1.png", true));
        } else if (action.name() == "LEVEL5") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/level5.png", true));
        } else if (action.name() == "RESET") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/level0.png", true));
        }

        if (action.name() == "UP") {
            m_ECS.getComponent<CInputs>(m_player).up = true;
        }
        if (action.name() == "DOWN") {
            m_ECS.getComponent<CInputs>(m_player).down = true; 
        }
        if (action.name() == "LEFT") {
            m_ECS.getComponent<CInputs>(m_player).left = true;
        }
        if (action.name() == "RIGHT") {
            m_ECS.getComponent<CInputs>(m_player).right = true;
        }
        if (action.name() == "SHIFT") {
            m_ECS.getComponent<CInputs>(m_player).shift = true;
        }
        if (action.name() == "CTRL") {
            m_ECS.getComponent<CInputs>(m_player).ctrl = true;
        }
        // if (action.name() == "ATTACK"){
        //     if ( m_player->getComponent<CInputs>().canShoot ){
        //         m_player->getComponent<CInputs>().shoot = true;
        //         spawnProjectile(m_player->getLinkEntity(), getMousePosition()-m_player->getLinkEntity()->getComponent<CTransform>().pos+m_camera.position);
        //     }
        // }
    }
    else if (action.type() == "END") {
        if (action.name() == "DOWN") {
            m_ECS.getComponent<CInputs>(m_player).down = false;
        } if (action.name() == "UP") {
            m_ECS.getComponent<CInputs>(m_player).up = false;
        } if (action.name() == "LEFT") {
            m_ECS.getComponent<CInputs>(m_player).left = false;
        } if (action.name() == "RIGHT") {
            m_ECS.getComponent<CInputs>(m_player).right = false;
        } if (action.name() == "SHIFT") {
            m_ECS.getComponent<CInputs>(m_player).shift = false;
        } if (action.name() == "CTRL") {
            m_ECS.getComponent<CInputs>(m_player).ctrl = false;
        } if (action.name() == "ATTACK") {
            m_ECS.getComponent<CInputs>(m_player).canShoot = false;
            // if ( m_player->getLinkEntity() ){
            //     if ( m_player->getLinkEntity()->getLinkEntity()->getComponent<CProjectileState>().state == "Free" ){
            //         m_player->getLinkEntity()->getLinkEntity()->getComponent<CTransform>().isMovable = true; 
            //         m_player->getLinkEntity()->removeLinkEntity();
            //     } else {
            //         m_player->getLinkEntity()->getLinkEntity()->kill();
            //     }
            // }
        }
    }
}

void Scene_Play::update() {
    m_ECS.update();
    m_pause = m_camera.update(m_ECS.getComponent<CTransform>(m_player).pos, m_pause);
    // m_pause = m_camera.update(Vec2{0,0}, m_pause);
    if (!m_pause) {
        sMovement();
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
        sCollision();
        sStatus();
        sAnimation();
    }
    sRender();
}

void Scene_Play::sMovement() {
    // TODO: make a view reference
    auto view = m_ECS.view<CTransform, CInputs>();
    for (auto e : view){    
        auto &transform = m_ECS.getComponent<CTransform>(e);
        auto &inputs = m_ECS.getComponent<CInputs>(e);

        if ( e == m_player ){
            transform.vel = { 0,0 };
            if (inputs.up){
                transform.vel.y--;
            } if (inputs.down){
                transform.vel.y++;
            } if (inputs.left){
                transform.vel.x--;
            } if (inputs.right){
                transform.vel.x++;
            } if (inputs.shift){
                transform.tempo = 0.5f;
            } else if (inputs.ctrl){
                transform.tempo = 3.0f;
            } else{
                transform.tempo = 1.0f;
            }
        }
        
        // if ( m_ECS.getComponent<CName>(e).name == "Dragon"){

        //     if (!(transform.vel.isnull()) && transform.isMovable ){
        //         transform.pos += transform.vel.norm(transform.tempo*transform.speed/m_game->framerate());
        //     } if (transform.pos != transform.prevPos){
        //         transform.isMovable = false;
        //     }
        // }

        // if (m_ECS.hasComponent<CPathfind>(e)) {
        //     Vec2& target = m_ECS.getComponent<CPathfind>(e).target;
        //     if ((target - transform.pos).length() < 64*2) {
        //         transform.vel = target - transform.pos;
        //     } else {
        //         transform.vel = Vec2 {0,0};
        //     }
        //     target = m_ECS.getComponent<CTransform>(m_player).pos;
        // }

        transform.prevPos = transform.pos;
        if (!(transform.vel.isnull()) && transform.isMovable ){
            transform.pos += transform.vel.norm(transform.tempo*transform.speed/m_game->framerate());
        }
        // if ( m_ECS.hasComponent<CKnockback>(e) ){
        //     transform.pos += m_physics.knockback(m_ECS.getComponent<CKnockback>(e));
        //     if ( m_ECS.getComponent<CKnockback>(e).duration == 0 ){
        //         m_ECS.removeComponent<CKnockback>(e);
        //     }
        // }

        // if ( m_ECS.getLinkEntity() ){
        //     m_ECS.getLinkEntity()->getComponent<CTransform>().pos = transform.pos;
        //     if (m_ECS.getLinkEntity()->tag() == "Projectile"){
        //         m_ECS.getLinkEntity()->getComponent<CTransform>().vel = getMousePosition()-m_player->getLinkEntity()->getComponent<CTransform>().pos+m_camera.position;
        //         m_ECS.getLinkEntity()->getComponent<CTransform>().angle = m_ECS.getLinkEntity()->getComponent<CTransform>().vel.angle();
        //     }
        // }
        // if ( m_camera.position.x-m_gridSize.x > transform.pos.x || m_camera.position.x+width()+m_gridSize.x < transform.pos.x || m_camera.position.y-m_gridSize.y > transform.pos.y || m_camera.position.y+height()+m_gridSize.y < transform.pos.y ) {
            // e->setInCamera(false);
        // } else {
        //     e->setInCamera(true);
        // }
    }
}

void Scene_Play::sCollision() {
// ------------------------------- Player collisions -------------------------------------------------------------------------

    Entity player = {m_player, &m_ECS};
    auto &view = m_ECS.view<CBoundingBox>();
    for (auto e : view ){
        Entity entity = {e, &m_ECS};
        if ( m_physics.isCollided(player, entity) )
        {
            player.getComponent<CTransform>().pos += m_physics.overlap(player,entity);
        }
    }
//     for ( auto o : m_entities.getEntities("Obstacle") ) 
//     {
        // if ( m_physics.isCollided(m_player,o) )
//         {
//             m_player->movePosition(m_physics.overlap(m_player,o));
//         }
//     }

//     for ( auto w : m_entities.getEntities("Weapon") ) 
//     {
//         if ( m_physics.isCollided(m_player,w) )
//         {
//             m_player->setLinkEntity(w);
//         }
//     }

//     for ( auto e : m_entities.getEntities("Enemy")){
//         if ( m_physics.isCollided(m_player, e) )
//         {
//             e->movePosition(m_physics.overlap(e,m_player));
//             if ( m_player->takeDamage(e, m_currentFrame) ){
//                 m_camera.startShake(5, 50);
//             }
//         }
//     }

//     for ( auto c : m_entities.getEntities("Coin") ) 
//     {
//         if ( m_physics.isCollided(m_player,c) ) 
//         {
//             c->kill();
//         }
//     }

//     for ( auto w : m_entities.getEntities("Water") ) 
//     {
//         if ( m_physics.isStandingIn(m_player,w) )
//         {
//             m_player->getComponent<CHealth>().HP = 0;
//         }
//     }

//     for ( auto l : m_entities.getEntities("Lava") ) 
//     {
//         if ( m_physics.isStandingIn(m_player,l) )
//         {
//             m_player->getComponent<CHealth>().HP = 0;
//         }
//     }

//     for ( auto d : m_entities.getEntities("Dragon") ) 
//     {
//         if ( m_physics.isCollided(m_player,d) && d->hasComponent<CDamage>() )
//         {
//             m_player->movePosition(m_physics.overlap(m_player,d)*15);
//             m_player->takeDamage(d, m_currentFrame);
//             d->addComponent<CAnimation>(m_game->assets().getAnimation("waking_dragon"), false);
//         }
//     }

//     for ( auto g : m_entities.getEntities("Goal") ) 
//     {
//         if ( m_physics.isCollided(m_player,g) )
//         {
//             if ( g->getComponent<CAnimation>().animation.getName() != "checkpoint_wave" ) {
//             g->addComponent<CAnimation>(m_game->assets().getAnimation("checkpoint_wave"), true);
//             saveGame("game_save.txt");
//             } 
//         }
//     }


// // ------------------------------- Enemy collisions -------------------------------------------------------------------------
//     for ( auto e : m_entities.getEntities("Enemy") )
//     {   
//         for ( auto e1 : m_entities.getEntities("Enemy") ) 
//         {
//             if ( m_physics.isCollided(e,e1) ) 
//             {
//                 e->movePosition(m_physics.overlap(e,e1));
//             }
//         }
//     }

// // ------------------------------- Projectile collisions -------------------------------------------------------------------------

//     for ( auto p : m_entities.getEntities("Projectile") )
//     {
//         for ( auto o :  m_entities.getEntities("Obstacle") )
//         {
//             if (m_physics.isCollided(p,o))
//             {
//                 if ( p->getComponent<CTransform>().isMovable )
//                 {
//                     p->addComponent<CAnimation>(m_game->assets().getAnimation("fireball_explode"), false);
//                     p->getComponent<CTransform>().isMovable = false;
//                 }
//             }
//         }

//         for ( auto e :  m_entities.getEntities("Enemy") )
//         {
//             if (m_physics.isCollided(p,e))
//             {
//                 if (e->hasComponent<CHealth>() && p->hasComponent<CDamage>())
//                 {
//                     e->takeDamage(p, m_currentFrame);
//                 }
//                 if ( p->getComponent<CTransform>().isMovable )
//                 {
//                     p->addComponent<CAnimation>(m_game->assets().getAnimation("fireball_explode"), false);
//                     p->getComponent<CTransform>().isMovable = false;
//                     p->removeComponent<CDamage>();
//                 }
//                 if ( e->getComponent<CHealth>().HP <= 0 )
//                 {
//                     spawnCoin(e->getComponent<CTransform>().pos, 4);
//                     e->kill();
//                 }
//             }
//         }

//         for ( auto d :  m_entities.getEntities("Dragon") )
//         {
//             if (m_physics.isCollided(p,d))
//             {
//                 if (d->hasComponent<CHealth>() && p->hasComponent<CDamage>())
//                 {
//                     d->takeDamage(p, m_currentFrame);
//                     d->addComponent<CAnimation>(m_game->assets().getAnimation("waking_dragon"), false);
//                 }
//                 if ( p->getComponent<CTransform>().isMovable )
//                 {
//                     p->addComponent<CAnimation>(m_game->assets().getAnimation("fireball_explode"), false);
//                     p->getComponent<CTransform>().isMovable = false;
//                     p->removeComponent<CDamage>();
//                 }
//                 if ( d->getComponent<CHealth>().HP <= 0 )
//                 {
//                     d->kill();
//                 }
//             }
//         }
//     }
}

void Scene_Play::sStatus() {
    // if ( m_player->getLinkEntity() ){
    //     m_player->getComponent<CInputs>().canShoot = true;
    // } else {
    //     m_player->getComponent<CInputs>().canShoot = false;
    // }
    if ( m_ECS.getComponent<CHealth>(m_player).HP <= 0 ){
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/level0.png", true));
    }
}

void Scene_Play::sAnimation() {
    auto view = m_ECS.view<CAnimation, CState>();
    for ( auto e : view ){
        if( m_ECS.getComponent<CTransform>(e).vel.isnull() ) {
            changePlayerStateTo(e, PlayerState::STAND);
        } else if(m_ECS.getComponent<CTransform>(e).vel.mainDir().x > 0 ) {
            changePlayerStateTo(e, PlayerState::RUN_RIGHT);
        } else if(m_ECS.getComponent<CTransform>(e).vel.mainDir().x < 0) {
            changePlayerStateTo(e, PlayerState::RUN_LEFT);
        } else if(m_ECS.getComponent<CTransform>(e).vel.mainDir().y > 0) {
            changePlayerStateTo(e, PlayerState::RUN_DOWN);
        } else if(m_ECS.getComponent<CTransform>(e).vel.mainDir().y < 0) {
            changePlayerStateTo(e, PlayerState::RUN_UP);
        }
        // // change player animation
        if (view.getComponent<CState>(e).changeAnimate) {
            view.getComponent<CAnimation>(e).animation.setRow((int)view.getComponent<CState>(e).state);
        }
    }

    // auto view1 = m_ECS.view<CAnimation, CProjectileState>();
    // for ( auto e : view1 ) {
        // if ( view1.getComponent<CProjectileState>(e).state == "Create" ) {
        //     if ( view1.getComponent<CAnimation>(e).animation.hasEnded() ) {
        //         view1.getComponent<CProjectileState>(e).state = "Free";
        //         // view1.getComponent<CAnimation>(e).animation = m_game->assets().getAnimation("fireball");
        //         m_camera.startShake(4, 100);
        //     }
        // }
    // }

    auto &view2 = m_ECS.view<CAnimation>();
    for ( auto e : view2 ){
        // auto& animation = m_ECS.getComponent<CAnimation>(e);
        auto& animation = view2.getComponent(e);
        if (animation.animation.hasEnded() && !animation.repeat) {
            // if (m_ECS.getComponent<CName>(e).name == "Dragon") {
            //     m_ECS.addComponent<CAnimation>(e, m_game->assets().getAnimation("snoring_dragon"), true);
            // } else {
            //     // e->kill();
            // }
        }
        animation.animation.update();
    }
    // }
}

void Scene_Play::sRender() {
    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
    SDL_RenderClear(m_game->renderer());

    if (m_drawTextures){
        //auto viewSorted = m_ECS.view_sorted<CAnimation>();
        auto view = m_ECS.view<CAnimation, CTransform>();
         //for (auto e : view){
        for (auto e : view){
                
            auto& transform = m_ECS.getComponent<CTransform>(e);
            auto& animation = m_ECS.getComponent<CAnimation>(e).animation;

            // Adjust the entity's position based on the camera position
            Vec2 adjustedPos = transform.pos - m_camera.position;
            // if (cameraZoom != 1) {

            // }

            if ( m_ECS.hasComponent<CShadow>(e) ){
                auto& shadow = m_ECS.getComponent<CShadow>(e);

                // Set the destination rectangle for rendering
                shadow.animation.setScale(transform.scale*cameraZoom);
                shadow.animation.setAngle(transform.angle);
                shadow.animation.setDestRect(adjustedPos - shadow.animation.getDestSize()/2);
                
                spriteRender(shadow.animation);
            } 

            animation.setScale(transform.scale*cameraZoom);
            animation.setAngle(transform.angle);
            animation.setDestRect(adjustedPos - animation.getDestSize()/2);
            
            spriteRender(animation);

            if (m_ECS.hasComponent<CHealth>(e) && e != m_player){
                if ( (int)m_currentFrame - m_ECS.getComponent<CHealth>(e).damage_frame < m_ECS.getComponent<CHealth>(e).heart_frames) {

                    Animation animation;
                    auto hearts = float(m_ECS.getComponent<CHealth>(e).HP)/2;

                    for (int i = 1; i <= m_ECS.getComponent<CHealth>(e).HP_max/2; i++)
                    {   
                        if ( hearts >= i ){
                            animation = m_ECS.getComponent<CHealth>(e).animation_full;
                        } else if ( i-hearts == 0.5f ){
                            animation = m_ECS.getComponent<CHealth>(e).animation_half;
                        } else{
                            animation = m_ECS.getComponent<CHealth>(e).animation_empty;
                        }

                        animation.setScale(Vec2{2, 2}*cameraZoom);
                        animation.setDestRect(Vec2{
                            adjustedPos.x + (float)(i-1-(float)m_ECS.getComponent<CHealth>(e).HP_max/4)*animation.getSize().x*animation.getScale().x, 
                            adjustedPos.y - m_ECS.getComponent<CAnimation>(e).animation.getSize().y * m_ECS.getComponent<CAnimation>(e).animation.getScale().y / 2
                        });
                        spriteRender(animation);
                    }
                }
            }
        }
        // Attemp to render coin balance in corner.

        SDL_Rect texRect;
        texRect.x = 0;
        texRect.y = 96;
        texRect.h = 128;
        texRect.w = 128;

        SDL_RenderCopyEx(
            m_game->renderer(), 
            m_game->assets().getTexture("coin_front"), 
            nullptr, 
            &texRect,
            0,
            NULL,
            SDL_FLIP_NONE
        );

        Animation animation;
        auto hearts = float(m_ECS.getComponent<CHealth>(m_player).HP)/2;

        for (int i = 1; i <= m_ECS.getComponent<CHealth>(m_player).HP_max/2; i++)
        {   
            if ( hearts >= i ){
                animation = m_ECS.getComponent<CHealth>(m_player).animation_full;
            } else if ( i-hearts == 0.5f ){
                animation = m_ECS.getComponent<CHealth>(m_player).animation_half;
            } else{
                animation = m_ECS.getComponent<CHealth>(m_player).animation_empty;
            }

            animation.setScale(Vec2{4, 4});
            animation.setDestRect(Vec2{(float)(i-1)*animation.getSize().x*animation.getScale().x, 0});

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
    // SDL_Rect rect;
    // rect.x = 0;
    // rect.y = 0;
    // rect.w = width();
    // rect.h = height();

    // SDL_SetRenderDrawColor(m_game->renderer(), 20, 0, 50, 100);
    // SDL_RenderFillRect(m_game->renderer(), &rect);
}

void Scene_Play::spriteRender(Animation &animation){
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

void Scene_Play::spawnPlayer(){

    auto entityID = m_ECS.addEntity();
    m_player = entityID;

    int pos_x = 0;
    int pos_y = 0;
    int hp;
    if (m_newGame){
        pos_x = (int)m_playerConfig.x;
        pos_y = (int)m_playerConfig.y;
        hp = (int)m_playerConfig.HP;
    } else {
        std::ifstream file("game_save.txt");
        if (!file) {
            std::cerr << "Could not load game_save.txt file!\n";
            exit(-1);
        }
        std::string head;
        while (file >> head) {
            if (head == "Player_pos") {
                file >> pos_x >> pos_y;
            }
            if (head == "Player_hp") {
                file >> hp;
            }
        }
    }
    Vec2 pos = Vec2{64*(float)pos_x, 64*(float)pos_y};
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entityID);

    m_ECS.addComponent<CTransform>(entityID, midGrid, Vec2{0,0}, Vec2{4, 4}, 0.0f, m_playerConfig.SPEED, true);
    m_ECS.addComponent<CBoundingBox>(entityID, Vec2 {32, 32});

    m_ECS.addComponent<CName>(entityID, "wiz");
    m_ECS.addComponent<CAnimation>(entityID, m_game->assets().getAnimation("demon"), true, 3);
    m_ECS.addComponent<CShadow>(entityID, m_game->assets().getAnimation("shadow"), false);

    m_ECS.addComponent<CInputs>(entityID);
    m_ECS.addComponent<CState>(entityID, PlayerState::STAND);

    m_ECS.addComponent<CDamage>(entityID, m_playerConfig.DAMAGE, 180);
    m_ECS.addComponent<CHealth>(entityID, hp, m_playerConfig.HP, 60, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    // entity->addComponent<CWeapon>(m_game->assets().getAnimation("staff"), 1, 10, 64);
    // m_ECS.addComponent<CScript>(entityID).Bind<PlayerController>();
}

void Scene_Play::spawnWeapon(Vec2 pos){
    auto entity = m_ECS.addEntity();

    m_ECS.addComponent<CTransform>(entity, pos, Vec2{0,0}, Vec2{4, 4}, 0.0f, 0.0f, true);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2 {32, 32});
    m_ECS.addComponent<CName>(entity, "staff");
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("staff"), true, 2);
    m_ECS.addComponent<CDamage>(entity, 1, 180, std::unordered_set<std::string> {"Fire", "Explosive"});
}

void Scene_Play::spawnObstacle(const Vec2 pos, bool movable, const int frame){
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2 {0.5,0.5}, 0.0f, movable);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2 {64, 64});
}

void Scene_Play::spawnCloud(const Vec2 pos, bool movable, const int frame){
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, Vec2 {0.5,0.5}, 0.0f, movable);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2 {64, 64});
}

void Scene_Play::spawnDragon(const Vec2 pos, bool movable, const std::string &ani) {
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation(ani), true, 3);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, Vec2 {2, 2}, 0.0f, movable);
    m_ECS.addComponent<CHealth>(entity, (int)10, (int)10, 30, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    m_ECS.getComponent<CHealth>(entity).HPType = {""};
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{96, 96});
    m_ECS.addComponent<CShadow>(entity, m_game->assets().getAnimation("shadow"), false);
    m_ECS.addComponent<CDamage>(entity, 2, 30);
}

void Scene_Play::spawnGrass(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    // std::vector<int> ranArray = generateRandomArray(1, m_ECS.getNumEntities(), 0, 15);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2{0.5, 0.5}, 0.0f, false);
}

void Scene_Play::spawnDirt(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2 {0, 0}, Vec2{0.5, 0.5}, 0.0f, false);
}

void Scene_Play::spawnGoal(const Vec2 pos, bool movable)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity,m_game->assets().getAnimation("checkpoint_idle"), true, 3);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, Vec2{4,4}, 0.0f, movable);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{32, 32});
}

void Scene_Play::spawnKey(const Vec2 pos, const std::string playerToUnlock, bool movable)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("m_texture_key"), true, 3);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, Vec2 {1, 1}, 0.0f, movable);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{32, 32});
}

void Scene_Play::spawnLava(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, false);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{64, 64});
}

void Scene_Play::spawnWater(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_ECS.addEntity();
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, false);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{64, 64});
}

void Scene_Play::spawnBridge(const Vec2 pos, const int frame)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("bridge"), true, 3);
    m_ECS.getComponent<CAnimation>(entity).animation.setTile(Vec2{(float)(frame % 4), (float)(int)(frame / 4)});
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid,Vec2 {0, 0}, Vec2{2,2}, 0.0f, false);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{64, 64});
}

void Scene_Play::spawnProjectile(EntityID creator, Vec2 vel)
{
    auto entity = m_ECS.addEntity();
    // creator->setLinkEntity(entity);
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("fireball_create"), false, 3);
    m_ECS.addComponent<CTransform>(entity, m_ECS.getComponent<CTransform>(creator).pos, vel, Vec2{2, 2}, vel.angle(), 400.0f, false);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{12, 12});
    m_ECS.addComponent<CDamage>(entity, m_ECS.getComponent<CDamage>(creator).damage, m_ECS.getComponent<CDamage>(creator).speed); // damage speed 6 = frames between attacking
    m_ECS.getComponent<CDamage>(entity).damageType = {"Fire", "Explosive"};
    m_ECS.addComponent<CProjectileState>(entity, "Create");
    // m_entities.sort();
}

void Scene_Play::spawnCoin(Vec2 pos, const size_t layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("coin"), true, 3);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{4,4}, 0.0f, false);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{32, 32});
    m_ECS.addComponent<CShadow>(entity, m_game->assets().getAnimation("shadow"), false);
}

void Scene_Play::spawnSmallEnemy(Vec2 pos, const size_t layer)
{
    auto entity = m_ECS.addEntity();
    m_ECS.addComponent<CName>(entity, "rooter");
    m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation("rooter"), true, 3);
    m_ECS.addComponent<CState>(entity, PlayerState::STAND);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0,0}, Vec2{4,4}, 0.0f, 150.0f, true);
    m_ECS.addComponent<CBoundingBox>(entity, Vec2{32, 48});
    // m_ECS.addComponent<CPathfind>(entity, m_ECS.getComponent<CTransform>(m_player).pos, m_player);
    m_ECS.addComponent<CShadow>(entity, m_game->assets().getAnimation("shadow"), false);
    m_ECS.addComponent<CHealth>(entity, 4, 4, 30, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    m_ECS.getComponent<CHealth>(entity).HPType = {"Grass", "Organic"};
    m_ECS.addComponent<CDamage>(entity, 1, 60);
}

void Scene_Play::spawnDualTiles(const Vec2 pos, std::unordered_map<std::string, int> tileTextureMap)
{   
    for (const auto& [tileKey, textureIndex] : tileTextureMap) {
        std::string tile = tileKey;
        int layer = 10;
        
        if (tile == "water") {
            layer = layer - 1;
        } else if (tile == "lava") {
            layer = layer - 1;
        } else if (tile == "cloud") {
            layer = layer - 2;
        } else if (tile == "obstacle") {
            layer = layer - 2;
            tile = "mountain";  // Change the tile name for "obstacle"
        }

        // auto entity = m_entities.addEntity("DualTile", layer);
        EntityID entity = m_ECS.addEntity();
        m_ECS.addComponent<CAnimation>(entity, m_game->assets().getAnimation(tile + "_dual_sheet"), true, 10);
        m_ECS.getComponent<CAnimation>(entity).animation.setTile(Vec2{(float)(textureIndex % 4), (float)(int)(textureIndex / 4)});                           
        Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
        m_ECS.addComponent<CTransform>(entity, midGrid, Vec2{0, 0}, Vec2{4, 4}, 0.0f, false);
        m_ECS.addComponent<CName>(entity, tile);
    }
}

void Scene_Play::changePlayerStateTo(EntityID entity, PlayerState s) {
    auto& prev = m_ECS.getComponent<CState>(entity).preState; 
    if (prev != s) {
        prev = m_ECS.getComponent<CState>(entity).state;
        m_ECS.getComponent<CState>(entity).state = s; 
        m_ECS.getComponent<CState>(entity).changeAnimate = true;
    }
    else { 
        m_ECS.getComponent<CState>(entity).changeAnimate = false;
    }
}

void Scene_Play::onEnd() {
    m_game->changeScene("Menu", std::make_shared<Scene_Menu>(m_game));
}

void Scene_Play::setPaused(bool pause) {
    m_pause = pause;
}

Vec2 Scene_Play::gridSize(){
    return m_gridSize;
}

Vec2 Scene_Play::levelSize(){
    return m_levelSize;
}

Vec2 Scene_Play::gridToMidPixel(float gridX, float gridY, EntityID entity) {
    Vec2 offset;
    Vec2 grid = Vec2{gridX, gridY};
    Vec2 eSize;
    if ( m_ECS.hasComponent<CAnimation>(entity) ){
        eSize = m_ECS.getComponent<CAnimation>(entity).animation.getSize();
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