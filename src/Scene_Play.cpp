#include "Scene_Menu.h"
#include "Scene_Play.h"
#include "Sprite.h"
#include "Assets.h"
#include "Game.h"
#include "Components.h"
#include "Action.h"
#include "Level_Loader.h"
#include "Camera.h"

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
    registerAction(SDLK_SPACE, "SHOOT");
    registerAction(SDL_BUTTON_LEFT , "SHOOT MOUSE");
    registerAction(SDLK_LSHIFT, "SHIFT");
    registerAction(SDLK_LCTRL, "CTRL");
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_f, "CAMERA FOLLOW");
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

Vec2 Scene_Play::gridToMidPixel(float gridX, float gridY, std::shared_ptr<Entity> entity) {
    Vec2 offset;
    Vec2 grid = Vec2{gridX, gridY};
    Vec2 eSize;
    if ( entity->hasComponent<CAnimation>() ){
        eSize = entity->getComponent<CAnimation>().animation.getSize();
    } else {
        eSize = m_gridSize/2;
    }
    
    Vec2 eScale;
    switch ((int)eSize.y) {
        case 270:
            eScale.x = 0.15;
            eScale.y = 0.18;
            break;
        case 225:
            eScale.x = 0.18;
            eScale.y = 0.18;
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
            eScale.x = 1.0;
            eScale.y = 1.0;
            break;
        case 32:
            eScale.x = 2.0;
            eScale.y = 2.0;
            break;
        case 16:
            eScale.x = 4.0;
            eScale.y = 4.0;
            break;
        case 24:
            eScale.x = 2.0;
            eScale.y = 2.0;
            break; 
        default:
            eScale.x = 1.0;
            eScale.y = 1.0;
    }
    
    offset = (m_gridSize - eSize * eScale) / 2.0;

    return grid + m_gridSize / 2 - offset;
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
        saveFile << "Player_pos " << (int)(m_player->getComponent<CTransform>().pos.x/m_gridSize.x) << " " << (int)(m_player->getComponent<CTransform>().pos.y/m_gridSize.y) << std::endl;
        saveFile << "Player_hp " << m_player->getComponent<CHealth>().HP << std::endl;
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
    spawnHUD();
    spawnCoin(Vec2{64*15,64*12}, 4);
    spawnDragon(Vec2{64*16 , 64*35}, false, "snoring_dragon");
    spawnSmallEnemy(Vec2{64*15 , 64*9}, 3);
    spawnSmallEnemy(Vec2{64*10 , 64*10}, 3);
    spawnSmallEnemy(Vec2{64*20 , 64*12}, 3);
    spawnSmallEnemy(Vec2{64*13 , 64*24}, 3);
    spawnGoal(Vec2{64*23, 64*8}, false);
    spawnGoal(Vec2{64*37, 64*47}, false);

    m_entities.update();
    m_entities.sort();
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
            m_player->getComponent<CInputs>().up = true;
        }
        if (action.name() == "DOWN") {
            m_player->getComponent<CInputs>().down = true; 
        }
        if (action.name() == "LEFT") {
            m_player->getComponent<CInputs>().left = true;
        }
        if (action.name() == "RIGHT") {
            m_player->getComponent<CInputs>().right = true;
        }
        if (action.name() == "SHIFT") {
            m_player->getComponent<CInputs>().shift = true;
        }
        if (action.name() == "CTRL") {
            m_player->getComponent<CInputs>().ctrl = true;
        }
        if (action.name() == "SHOOT MOUSE"){
            m_player->getComponent<CInputs>().shoot = true;
            spawnProjectile(m_player, getMousePosition()-m_player->getComponent<CTransform>().pos+m_camera.position);
        }
    }
    else if (action.type() == "END") {
        if (action.name() == "DOWN") {
            m_player->getComponent<CInputs>().down = false;
        } if (action.name() == "UP") {
            m_player->getComponent<CInputs>().up = false;
        } if (action.name() == "LEFT") {
            m_player->getComponent<CInputs>().left = false;
        } if (action.name() == "RIGHT") {
            m_player->getComponent<CInputs>().right = false;
        } if (action.name() == "SHIFT") {
            m_player->getComponent<CInputs>().shift = false;
        } if (action.name() == "CTRL") {
            m_player->getComponent<CInputs>().ctrl = false;
        } if (action.name() == "SHOOT MOUSE") {
            // spawnProjectile(m_player, getMousePosition()-m_player->getComponent<CTransform>().pos+m_camera.position);
            m_player->getComponent<CInputs>().shoot = false;
        }
    }
}

void Scene_Play::update() {
    m_entities.update();
    if (!m_pause) {
        sMovement();
        sCollision();
        sStatus();
        sAnimation();
        m_currentFrame++;
    }
    sRender();
}

void Scene_Play::sMovement() {
    for (auto e : m_entities.getEntities()){    
        auto &transform = e->getComponent<CTransform>(); 

        if ( e == m_player ){
            transform.vel = { 0,0 };

            if (e->getComponent<CInputs>().up){
                transform.vel.y--;
            } if (e->getComponent<CInputs>().down){
                transform.vel.y++;
            } if (e->getComponent<CInputs>().left){
                transform.vel.x--;
            } if (e->getComponent<CInputs>().right){
                transform.vel.x++;
            } if (e->getComponent<CInputs>().shift){
                transform.tempo = 0.5f;
            } else if (e->getComponent<CInputs>().ctrl){
                transform.tempo = 3.0f;
            } else{
                transform.tempo = 1.0f;
            }
        }
        
        if ( e->tag() == "Dragon"){

            if (!(transform.vel.isnull()) && transform.isMovable ){
                transform.pos += transform.vel.norm(transform.tempo*transform.speed/m_game->framerate());
            } if (transform.pos != transform.prevPos){
                transform.isMovable = false;
            }
        }

        if (e->hasComponent<CPathfind>()) {
            Vec2& target = e->getComponent<CPathfind>().target;
            if ((target - transform.pos).length() < 64*2) {
                transform.vel = target - transform.pos;
            } else {
                transform.vel = Vec2 {0,0};
            }
            target = m_player->getComponent<CTransform>().pos;
        }


        transform.prevPos = transform.pos;
        if (!(transform.vel.isnull()) && transform.isMovable ){
            transform.pos += transform.vel.norm(transform.tempo*transform.speed/m_game->framerate());
        }
        if ( e->hasComponent<CKnockback>() ){
            transform.pos += m_physics.knockback(e->getComponent<CKnockback>());
            if ( e->getComponent<CKnockback>().duration == 0 ){
                e->removeComponent<CKnockback>();
            }
        }
        if ( e == m_player ){
            m_camera.update(transform.pos);
        }
        // if ( m_camera.position.x-m_gridSize.x > transform.pos.x || m_camera.position.x+width()+m_gridSize.x < transform.pos.x || m_camera.position.y-m_gridSize.y > transform.pos.y || m_camera.position.y+height()+m_gridSize.y < transform.pos.y ) {
            // e->setInCamera(false);
        // } else {
        //     e->setInCamera(true);
        // }
    }
}

void Scene_Play::sCollision() {
// ------------------------------- Player collisions -------------------------------------------------------------------------

    for ( auto o : m_entities.getEntities("Obstacle") ) 
    {
        if ( m_physics.isCollided(m_player,o) )
        {
            m_player->movePosition(m_physics.overlap(m_player,o));
        }
    }

    for ( auto e : m_entities.getEntities("Enemy")){
        if ( m_physics.isCollided(m_player, e) )
        {
            e->movePosition(m_physics.overlap(e,m_player));
            m_player->takeDamage(e, m_currentFrame);
            m_camera.startShake(4, 200);
        }
    }

    for ( auto c : m_entities.getEntities("Coin") ) 
    {
        if ( m_physics.isCollided(m_player,c) )
        {
            c->kill();
        }
    }

    for ( auto w : m_entities.getEntities("Water") ) 
    {
        if ( m_physics.isStandingIn(m_player,w) )
        {
            m_player->getComponent<CHealth>().HP = 0;
        }
    }

    for ( auto l : m_entities.getEntities("Lava") ) 
    {
        if ( m_physics.isStandingIn(m_player,l) )
        {
            m_player->getComponent<CHealth>().HP = 0;
        }
    }

    for ( auto d : m_entities.getEntities("Dragon") ) 
    {
        if ( m_physics.isCollided(m_player,d) && d->hasComponent<CDamage>() )
        {
            m_player->movePosition(m_physics.overlap(m_player,d)*15);
            m_player->takeDamage(d, m_currentFrame);
            d->addComponent<CAnimation>(m_game->assets().getAnimation("waking_dragon"), false);
        }
    }

    for ( auto g : m_entities.getEntities("Goal") ) 
    {
        if ( m_physics.isCollided(m_player,g) )
        {
            if ( g->getComponent<CAnimation>().animation.getName() != "checkpoint_wave" ) {
            g->addComponent<CAnimation>(m_game->assets().getAnimation("checkpoint_wave"), true);
            saveGame("game_save.txt");
            } 
        }
    }


// ------------------------------- Enemy collisions -------------------------------------------------------------------------
    for ( auto e : m_entities.getEntities("Enemy") )
    {   
        for ( auto e1 : m_entities.getEntities("Enemy") ) 
        {
            if ( m_physics.isCollided(e,e1) ) 
            {
                e->movePosition(m_physics.overlap(e,e1));
            }
        }
    }

// ------------------------------- Projectile collisions -------------------------------------------------------------------------

    for ( auto p : m_entities.getEntities("Projectile") )
    {
        for ( auto o :  m_entities.getEntities("Obstacle") )
        {
            if (m_physics.isCollided(p,o))
            {
                if ( p->getComponent<CTransform>().isMovable )
                {
                    p->addComponent<CAnimation>(m_game->assets().getAnimation("fireball_explode"), false);
                    p->getComponent<CTransform>().isMovable = false;
                }
            }
        }

        for ( auto e :  m_entities.getEntities("Enemy") )
        {
            if (m_physics.isCollided(p,e))
            {
                if (e->hasComponent<CHealth>() && p->hasComponent<CDamage>())
                {
                    e->takeDamage(p, m_currentFrame);
                }
                if ( p->getComponent<CTransform>().isMovable )
                {
                    p->addComponent<CAnimation>(m_game->assets().getAnimation("fireball_explode"), false);
                    p->getComponent<CTransform>().isMovable = false;
                    p->removeComponent<CDamage>();
                }
                if ( e->getComponent<CHealth>().HP <= 0 )
                {
                    spawnCoin(e->getComponent<CTransform>().pos, 4);
                    e->kill();
                }
            }
        }

        for ( auto d :  m_entities.getEntities("Dragon") )
        {
            if (m_physics.isCollided(p,d))
            {
                if (d->hasComponent<CHealth>() && p->hasComponent<CDamage>())
                {
                    d->takeDamage(p, m_currentFrame);
                    d->addComponent<CAnimation>(m_game->assets().getAnimation("waking_dragon"), false);
                }
                if ( p->getComponent<CTransform>().isMovable )
                {
                    p->addComponent<CAnimation>(m_game->assets().getAnimation("fireball_explode"), false);
                    p->getComponent<CTransform>().isMovable = false;
                    p->removeComponent<CDamage>();
                }
                if ( d->getComponent<CHealth>().HP <= 0 )
                {
                    d->kill();
                }
            }
        }
    }
}

void Scene_Play::sStatus() {
    for ( auto p : m_entities.getEntities("Player") ){
        if ( p->getComponent<CHealth>().HP <= 0 ){
                m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/level0.png", true));
        }
    }
}

void Scene_Play::sAnimation() {
    for ( auto e : m_entities.getEntities() ){
        if ( e->hasComponent<CState>() ){
            if( e->getComponent<CTransform>().vel.isnull() ) {
                changePlayerStateTo(e, PlayerState::STAND);
            } else if( e->getComponent<CTransform>().vel.mainDir().x > 0 ) {
                changePlayerStateTo(e, PlayerState::RUN_RIGHT);
            } else if(e->getComponent<CTransform>().vel.mainDir().x < 0) {
                changePlayerStateTo(e, PlayerState::RUN_LEFT);
            } else if(e->getComponent<CTransform>().vel.mainDir().y > 0) {
                changePlayerStateTo(e, PlayerState::RUN_DOWN);
            } else if(e->getComponent<CTransform>().vel.mainDir().y < 0) {
                changePlayerStateTo(e, PlayerState::RUN_UP);
            }

            // // change player animation
            if (e->getComponent<CState>().changeAnimate) {
                std::string entityName = e->getComponent<CName>().name;
                switch (e->getComponent<CState>().state) {
                    case PlayerState::STAND:
                        e->getComponent<CAnimation>().animation.setRow(0);
                        break;
                    case PlayerState::RUN_RIGHT:
                        e->getComponent<CAnimation>().animation.setRow(2);
                        break;
                    case PlayerState::RUN_DOWN:
                        e->getComponent<CAnimation>().animation.setRow(1);
                        break;
                    case PlayerState::RUN_LEFT:
                        e->getComponent<CAnimation>().animation.setRow(4);
                        break;
                    case PlayerState::RUN_UP:
                        e->getComponent<CAnimation>().animation.setRow(3);
                        break;
                }
            }
        }
        
        if ( e->hasComponent<CProjectileState>() ) {
            if ( e->getComponent<CProjectileState>().state == "Create" ) {
                if ( e->getComponent<CAnimation>().animation.hasEnded() ) {
                    e->addComponent<CAnimation>(m_game->assets().getAnimation("fireball"), true);
                    e->getComponent<CTransform>().isMovable = true;
                }
                if ( m_player->getComponent<CInputs>().shoot ) {
                    e->getComponent<CTransform>().pos = m_player->getComponent<CTransform>().pos;
                    e->getComponent<CTransform>().vel = getMousePosition()-m_player->getComponent<CTransform>().pos+m_camera.position;
                    e->getComponent<CTransform>().angle = e->getComponent<CTransform>().vel.angle();
                } else {
                    e->getComponent<CProjectileState>().state = "Free";
                }
            }
        }
        
        if ( e->hasComponent<CAnimation>() ){
            if (e->getComponent<CAnimation>().animation.hasEnded() && !e->getComponent<CAnimation>().repeat) {
                if (e->tag() == "Dragon") {
                    e->addComponent<CAnimation>(m_game->assets().getAnimation("snoring_dragon"), true);
                } else {
                    e->kill();
                }
            }
            if (e->hasComponent<CAnimation>()) {
                e->getComponent<CAnimation>().animation.update();
            }
        }
    }
}

void Scene_Play::sRender() {
    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
    SDL_RenderClear(m_game->renderer());

    if (m_drawTextures){
        for (auto e : m_entities.getEntities()){        
            if ( e->hasComponent<CTransform>() && e->hasComponent<CAnimation>() && e->inCamera()){
                
                auto& transform = e->getComponent<CTransform>();
                auto& animation = e->getComponent<CAnimation>().animation;

                // Adjust the entity's position based on the camera position
                Vec2 adjustedPos = transform.pos - m_camera.position;
                if (cameraZoom != 1) {

                }

                if ( e->hasComponent<CShadow>() ){
                    auto& shadow = e->getComponent<CShadow>();

                    // Set the destination rectangle for rendering
                    shadow.animation.setScale(transform.scale*cameraZoom);
                    shadow.animation.setAngle(transform.angle);
                    shadow.animation.setDestRect(adjustedPos - shadow.animation.getDestSize()/2);

                    SDL_RenderCopyEx(
                        m_game->renderer(), 
                        shadow.animation.getTexture(), 
                        shadow.animation.getSrcRect(), 
                        shadow.animation.getDestRect(),
                        shadow.animation.getAngle(),
                        NULL,
                        SDL_FLIP_NONE
                    );
                } 

                animation.setScale(transform.scale*cameraZoom);
                animation.setAngle(transform.angle);
                animation.setDestRect(adjustedPos - animation.getDestSize()/2);

                SDL_RenderCopyEx(
                    m_game->renderer(), 
                    animation.getTexture(), 
                    animation.getSrcRect(), 
                    animation.getDestRect(),
                    animation.getAngle(),
                    NULL,
                    SDL_FLIP_NONE
                );
                if (e->hasComponent<CHealth>() && e != m_player){
                    if ( (int)m_currentFrame - e->getComponent<CHealth>().damage_frame < e->getComponent<CHealth>().heart_frames) {

                        Animation animation;
                        auto hearts = float(e->getComponent<CHealth>().HP)/2;

                        for (int i = 1; i <= e->getComponent<CHealth>().HP_max/2; i++)
                        {   
                            if ( hearts >= i ){
                                animation = e->getComponent<CHealth>().animation_full;
                            } else if ( i-hearts == 0.5f ){
                                animation = e->getComponent<CHealth>().animation_half;
                            } else{
                                animation = e->getComponent<CHealth>().animation_empty;
                            }

                            animation.setScale(Vec2{2, 2}*cameraZoom);
                            animation.setDestRect(Vec2{
                                adjustedPos.x + (float)(i-1-(float)e->getComponent<CHealth>().HP_max/4)*animation.getSize().x*animation.getScale().x, 
                                adjustedPos.y - e->getComponent<CAnimation>().animation.getSize().y * e->getComponent<CAnimation>().animation.getScale().y / 2
                            });
                            
                            SDL_RenderCopyEx(
                                m_game->renderer(), 
                                animation.getTexture(), 
                                nullptr, 
                                animation.getDestRect(),
                                0,
                                NULL,
                                SDL_FLIP_NONE
                            );
                        }
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
        auto hearts = float(m_player->getComponent<CHealth>().HP)/2;

        for (int i = 1; i <= m_player->getComponent<CHealth>().HP_max/2; i++)
        {   
            if ( hearts >= i ){
                animation = m_player->getComponent<CHealth>().animation_full;
            } else if ( i-hearts == 0.5f ){
                animation = m_player->getComponent<CHealth>().animation_half;
            } else{
                animation = m_player->getComponent<CHealth>().animation_empty;
            }

            animation.setScale(Vec2{4, 4});
            animation.setDestRect(Vec2{(float)(i-1)*animation.getSize().x*animation.getScale().x, 0});
            
            SDL_RenderCopyEx(
                m_game->renderer(), 
                animation.getTexture(), 
                nullptr, 
                animation.getDestRect(),
                0,
                NULL,
                SDL_FLIP_NONE
            );
        }
    }

    if (m_drawCollision){
        for (auto e : m_entities.getEntities()){      
            if ( e->hasComponent<CTransform>() && e->hasComponent<CBoundingBox>() ){
                auto& transform = e->getComponent<CTransform>();
                auto& box = e->getComponent<CBoundingBox>();

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
}

void Scene_Play::spawnHUD(){
    auto entity = m_entities.addEntity("HUD", 1);
    Vec2 posHUD = Vec2{0, 0};
    entity->addComponent<CTransform>(posHUD);
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("wizIdle"), true);

}


void Scene_Play::spawnPlayer(){

    auto entity = m_entities.addEntity("Player", 3);
    int pos_x;
    int pos_y;
    int hp;
    m_player = entity;
    if (m_newGame){
        pos_x = m_playerConfig.x;
        pos_y = m_playerConfig.y;
        hp = m_playerConfig.HP;
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
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);

    entity->addComponent<CTransform>(midGrid, Vec2{0,0}, Vec2{4, 4}, 0, m_playerConfig.SPEED, true);
    entity->addComponent<CBoundingBox>(Vec2 {32, 32});

    entity->addComponent<CName>("wiz");
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("wiz"), true);
    entity->addComponent<CShadow>(m_game->assets().getAnimation("shadow"), false);

    entity->addComponent<CInputs>();
    entity->addComponent<CState>(PlayerState::STAND);

    entity->addComponent<CDamage>(m_playerConfig.DAMAGE, 180);
    entity->addComponent<CHealth>(hp, m_playerConfig.HP, 60, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
}

void Scene_Play::spawnObstacle(const Vec2 pos, bool movable, const int frame){
    auto entity = m_entities.addEntity("Obstacle", 8);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2 {0, 0}, Vec2 {0.5,0.5}, 0, movable);
    entity->addComponent<CBoundingBox>(Vec2 {64, 64});
}

void Scene_Play::spawnCloud(const Vec2 pos, bool movable, const int frame){
    auto entity = m_entities.addEntity("Obstacle", 8);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2 {0.5,0.5}, 0, movable);
    entity->addComponent<CBoundingBox>(Vec2 {64, 64});
}

void Scene_Play::spawnDragon(const Vec2 pos, bool movable, const std::string &ani) {
    auto entity = m_entities.addEntity("Dragon", 2);
    entity->addComponent<CAnimation>(m_game->assets().getAnimation(ani), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2 {2, 2}, 0, movable);
    entity->addComponent<CHealth>((int)10, (int)10, 30, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    entity->getComponent<CHealth>().HPType = {""};
    entity->addComponent<CBoundingBox>(Vec2{96, 96});
    entity->addComponent<CShadow>(m_game->assets().getAnimation("shadow"), false);
    entity->addComponent<CDamage>(2, 30);
}

void Scene_Play::spawnGrass(const Vec2 pos, const int frame)
{
    auto entity = m_entities.addEntity("Background", 10);
    std::vector<int> ranArray = generateRandomArray(1, m_entities.getTotalEntities(), 0, 15);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2 {0, 0}, Vec2{0.5, 0.5}, 0, false);
}

void Scene_Play::spawnDirt(const Vec2 pos, const int frame)
{
    auto entity = m_entities.addEntity("Background", 10);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2 {0, 0}, Vec2{0.5, 0.5}, 0, false);
}

void Scene_Play::spawnGoal(const Vec2 pos, bool movable)
{
    auto entity = m_entities.addEntity("Goal", 4);
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("checkpoint_idle"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2{4,4}, 0, movable);
    entity->addComponent<CBoundingBox>(Vec2{32, 32});
}

void Scene_Play::spawnKey(const Vec2 pos, const std::string playerToUnlock, bool movable)
{
    auto entity = m_entities.addEntity("Key", 4);
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("m_texture_key"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2 {1, 1}, 0, movable);
    entity->addComponent<CBoundingBox>(Vec2{32, 32});
}

void Scene_Play::spawnLava(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_entities.addEntity(tag, 8);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, false);
    entity->addComponent<CBoundingBox>(Vec2{64, 64});
}

void Scene_Play::spawnWater(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_entities.addEntity(tag, 8);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, false);
    entity->addComponent<CBoundingBox>(Vec2{64, 64});
}

void Scene_Play::spawnBridge(const Vec2 pos, const int frame)
{
    auto entity = m_entities.addEntity("Bridge", 7);
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("bridge"), true);
    entity->getComponent<CAnimation>().animation.setTile(Vec2{(float)(frame % 4), (float)(int)(frame / 4)});
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2{2,2}, 0, false);
    entity->addComponent<CBoundingBox>(Vec2{64, 64});
}

void Scene_Play::spawnProjectile(std::shared_ptr<Entity> player, Vec2 vel)
{
    auto entity = m_entities.addEntity("Projectile", 1);
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("fireball_create"), false);
    entity->addComponent<CTransform>(player->getComponent<CTransform>().pos, vel, Vec2{2, 2}, vel.angle(), 400, false);
    entity->addComponent<CBoundingBox>(Vec2{12, 12});
    entity->addComponent<CDamage>(player->getComponent<CDamage>().damage, player->getComponent<CDamage>().speed); // damage speed 6 = frames between attacking
    entity->getComponent<CDamage>().damageType = {"Fire", "Explosive"};
    entity->addComponent<CProjectileState>("Create");
    m_entities.sort();
}

void Scene_Play::spawnCoin(Vec2 pos, const size_t layer)
{
    auto entity = m_entities.addEntity("Coin", layer);
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("coin"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2{0,0}, Vec2{4,4}, 0, false);
    entity->addComponent<CBoundingBox>(Vec2{32, 32});
    entity->addComponent<CShadow>(m_game->assets().getAnimation("shadow"), false);
}

void Scene_Play::spawnSmallEnemy(Vec2 pos, const size_t layer)
{
    auto entity = m_entities.addEntity("Enemy", layer);
    entity->addComponent<CName>("rooter");
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("rooter"), true);
    entity->addComponent<CState>(PlayerState::STAND);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2{0,0}, Vec2{4,4}, 0, 150, true);
    entity->addComponent<CBoundingBox>(Vec2{32, 48});
    entity->addComponent<CPathfind>(m_player->getComponent<CTransform>().pos, m_player);
    entity->addComponent<CShadow>(m_game->assets().getAnimation("shadow"), false);
    entity->addComponent<CHealth>(4, 4, 30, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    entity->getComponent<CHealth>().HPType = {"Grass", "Organic"};
    entity->addComponent<CDamage>(1, 60);
}

void Scene_Play::spawnDualTiles(const Vec2 pos, std::unordered_map<std::string, int> tileTextureMap)
{   
    for (const auto& [tileKey, textureIndex] : tileTextureMap) {
        std::string tile = tileKey;
        size_t layer = 10;
        
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

        auto entity = m_entities.addEntity("DualTile", layer);
        entity->addComponent<CAnimation>(m_game->assets().getAnimation(tile + "_dual_sheet"), true);
        entity->getComponent<CAnimation>().animation.setTile(Vec2{(float)(textureIndex % 4), (float)(int)(textureIndex / 4)});                           
        Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
        entity->addComponent<CTransform>(midGrid, Vec2{0, 0}, Vec2{4, 4}, 0, false);
        entity->addComponent<CName>(tile);
    }
}

void Scene_Play::changePlayerStateTo(std::shared_ptr<Entity> entity, PlayerState s) {
    auto& prev = entity->getComponent<CState>().preState; 
    if (prev != s) {
        prev = entity->getComponent<CState>().state;
        entity->getComponent<CState>().state = s; 
        entity->getComponent<CState>().changeAnimate = true;
    }
    else { 
        entity->getComponent<CState>().changeAnimate = false;
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