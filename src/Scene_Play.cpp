#include "Scene_Play.h"
// #include "Scene_Play1.h"
#include "Sprite.h"
#include "Assets.h"
#include "Game.h"
#include "Components.h"
#include "Action.h"

#include "RandomArray.h"

#include <SDL2/SDL_image.h>

#include <iostream>
#include <string>
#include <fstream>

Scene_Play::Scene_Play(Game* game, std::string levelPath)
    : Scene(game), m_levelPath(levelPath)
{
    init(m_levelPath);
}

void Scene_Play::init(const std::string& levelPath) {
    registerAction(SDLK_w, "UP");
    registerAction(SDLK_s, "DOWN");
    registerAction(SDLK_a, "LEFT");
    registerAction(SDLK_d, "RIGHT");
    registerAction(SDLK_SPACE, "SHOOT");
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
    registerAction(SDLK_2, "LEVEL2");
    registerAction(SDLK_3, "LEVEL3");
    registerAction(SDLK_4, "LEVEL4");
    loadLevel(levelPath);
}

Vec2 Scene_Play::gridToMidPixel(
    float gridX, 
    float gridY, 
    std::shared_ptr<Entity> entity
) {
    float offsetX, offsetY;
    auto eSize = entity->getComponent<CAnimation>().animation.getSize();
    
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
            eScale.x = 2;
            eScale.y = 2;
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
    
    offsetX = (m_gridSize.x - eSize.x * eScale.x) / 2.0;
    offsetY = (m_gridSize.y - eSize.y * eScale.y) / 2.0;

    return Vec2(
        gridX + m_gridSize.x / 2 - offsetX,
        gridY + m_gridSize.y / 2 - offsetY
    );
}

void Scene_Play::loadLevel(std::string levelPath){

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
    levelSize = Vec2{ (float)WIDTH_PIX, (float)HEIGHT_PIX };
    auto format = loadedSurface->format;
    std::vector<std::vector<std::string>> pixelMatrix = createPixelMatrix(pixels, format, WIDTH_PIX, HEIGHT_PIX);

    // Process the pixels
    for (int y = 0; y < HEIGHT_PIX; ++y) {
        for (int x = 0; x < WIDTH_PIX; ++x) {
            const std::string& pixel = pixelMatrix[y][x];
            std::vector<bool> neighbors = neighborCheck(pixelMatrix, pixel, x, y, WIDTH_PIX, HEIGHT_PIX);
            std::vector<std::string> neighborsTags = neighborTag(pixelMatrix, pixel, x, y, WIDTH_PIX, HEIGHT_PIX);
            int textureIndex = getObstacleTextureIndex(neighbors);

            if (pixel == "obstacle") {
                spawnObstacle(Vec2 {64*(float)x, 64*(float)y}, false, textureIndex);
            }
            else{

                if (pixel == "grass" ) {
                    spawnGrass(Vec2 {64*(float)x,64*(float)y}, textureIndex);
                } else if (pixel == "dirt"){
                    spawnDirt(Vec2 {64*(float)x,64*(float)y}, textureIndex);
                } else {
                    if ( std::find(neighborsTags.begin(), neighborsTags.end(), "grass") != neighborsTags.end() ){
                        spawnGrass(Vec2 {64*(float)x,64*(float)y}, getObstacleTextureIndex(neighborCheck(pixelMatrix, "grass", x, y, WIDTH_PIX, HEIGHT_PIX)));
                    } else{
                        spawnDirt(Vec2 {64*(float)x,64*(float)y}, getObstacleTextureIndex(neighborCheck(pixelMatrix, "dirt", x, y, WIDTH_PIX, HEIGHT_PIX)));
                    }
                }
                if (pixel == "cloud") {
                    spawnCloud(Vec2 {64*(float)x, 64*(float)y}, false, textureIndex);
                } else if (pixel == "player_God") {
                    spawnPlayer(Vec2 {64*(float)x,64*(float)y}, "God", true);
                } else if (pixel == "player_Devil") {
                    spawnPlayer(Vec2 {64*(float)x,64*(float)y}, "Devil", false);
                } else if (pixel == "key") {
                    spawnKey(Vec2 {64*(float)x,64*(float)y}, "Devil", false);
                } else if (pixel == "goal") {
                    spawnGoal(Vec2 {64*(float)x,64*(float)y}, false);
                } else if (pixel == "dragon") {
                    spawnDragon(Vec2 {64*(float)x,64*(float)y}, true, "snoring_dragon");
                } else if (pixel == "lava") {
                    spawnLava(Vec2 {64*(float)x,64*(float)y}, "Lava");
                } else if (pixel == "water") {
                    spawnWater(Vec2 {64*(float)x,64*(float)y}, "Water", textureIndex);
                } else if (pixel == "bridge") {
                    if ( std::find(neighborsTags.begin(), neighborsTags.end(), "water") != neighborsTags.end() ){
                        spawnWater(Vec2 {64*(float)x,64*(float)y}, "Background", getObstacleTextureIndex(neighborCheck(pixelMatrix, "water", x, y, WIDTH_PIX, HEIGHT_PIX)));
                    } else if ( std::find(neighborsTags.begin(), neighborsTags.end(), "lava") != neighborsTags.end() ){
                        spawnLava(Vec2 {64*(float)x,64*(float)y}, "Background");
                    }
                    spawnBridge(Vec2 {64*(float)x,64*(float)y}, textureIndex);
                }
            }
        }
    }


    // Unlock and free the surface
    SDL_UnlockSurface(loadedSurface);
    SDL_FreeSurface(loadedSurface);
    SDL_DestroyTexture(texture);
    
    m_entities.update();
    m_entities.sort();
}

void Scene_Play::spawnPlayer(const Vec2 pos, const std::string name, bool movable){

    auto entity = m_entities.addEntity("Player", (size_t)3);
    std::string tex = "devil";
    if (name == "God"){
        tex = "angelS";
        m_player = entity;
    }

    entity->addComponent<CTexture>(Vec2 {0,0}, Vec2 {64, 64}, m_game->assets().getTexture(tex));
    entity->addComponent<CAnimation>(m_game->assets().getAnimation(tex), false);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2{0,0}, Vec2{4, 4}, 0, movable);
    entity->addComponent<CBoundingBox>(Vec2 {26, 32});
    entity->addComponent<CInputs>();
    entity->addComponent<CState>(PlayerState::RUN_DOWN);
    entity->addComponent<CHealth>(6, 6, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
}

void Scene_Play::spawnObstacle(const Vec2 pos, bool movable, const int frame){
    auto entity = m_entities.addEntity("Obstacle", (size_t)9);
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("rock_wall"));
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("rock_wall"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2 {0, 0}, Vec2 {0.5,0.5}, 0, movable);
    entity->addComponent<CBoundingBox>(Vec2 {64, 64});
}

void Scene_Play::spawnCloud(const Vec2 pos, bool movable, const int frame){
    auto entity = m_entities.addEntity("Obstacle", (size_t)9);
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("cloud_sheet"));
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("cloud_sheet"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2 {0.5,0.5}, 0, movable);
    entity->addComponent<CBoundingBox>(Vec2 {64, 64});
}

void Scene_Play::spawnDragon(const Vec2 pos, bool movable, const std::string &ani) {
    auto entity = m_entities.addEntity("Dragon", (size_t)2);
    entity->addComponent<CAnimation>(m_game->assets().getAnimation(ani), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2 {2, 2}, 0, movable);
    entity->addComponent<CHealth>(10, 10, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    entity->addComponent<CBoundingBox>(Vec2{96, 96});
}

void Scene_Play::spawnGrass(const Vec2 pos, const int frame)
{
    auto entity = m_entities.addEntity("Background", (size_t)10);
    std::vector<int> ranArray = generateRandomArray(1, m_entities.getTotalEntities(), 0, 15);
    if (ranArray[0] == 0){
        entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("flower_sheet"));
        entity->addComponent<CAnimation> (m_game->assets().getAnimation("flower_sheet"), true);
    }
    else{
        entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("gras_sheet"));
        entity->addComponent<CAnimation> (m_game->assets().getAnimation("gras_sheet"), true);
    }

    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2 {0, 0}, Vec2{0.5, 0.5}, 0, false);
}

void Scene_Play::spawnDirt(const Vec2 pos, const int frame)
{
    auto entity = m_entities.addEntity("Background", (size_t)10);
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("dirt_sheet"));
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("dirt_sheet"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2 {0, 0}, Vec2{0.5, 0.5}, 0, false);
}

void Scene_Play::spawnGoal(const Vec2 pos, bool movable)
{
    auto entity = m_entities.addEntity("Goal", (size_t)4);
    entity->addComponent<CTexture>(Vec2 {0,0}, Vec2 {64,64}, m_game->assets().getTexture("m_texture_goal"));
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("m_texture_goal"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2{1,1}, 0, movable);
    entity->addComponent<CBoundingBox>(Vec2{64, 64});
}

void Scene_Play::spawnKey(const Vec2 pos, const std::string playerToUnlock, bool movable)
{
    auto entity = m_entities.addEntity("Key", (size_t)4);
    entity->addComponent<CTexture>(Vec2 {0,0}, Vec2 {64, 64}, m_game->assets().getTexture("m_texture_key"));
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("m_texture_key"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2 {1, 1}, 0, movable);
    entity->addComponent<CBoundingBox>(Vec2{32, 32});
}

void Scene_Play::spawnLava(const Vec2 pos, const std::string tag)
{
    auto entity = m_entities.addEntity("Lava", (size_t)8);
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("lava_ani"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2{1,1}, 0, false);
    entity->addComponent<CBoundingBox>(Vec2{64, 64});
}

void Scene_Play::spawnWater(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_entities.addEntity(tag, (size_t)8);
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("water"));
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("water"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, false);
    entity->addComponent<CBoundingBox>(Vec2{64, 64});
}

void Scene_Play::spawnBridge(const Vec2 pos, const int frame)
{
    auto entity = m_entities.addEntity("Bridge", (size_t)7);
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("bridge"));
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("bridge"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, false);
    entity->addComponent<CBoundingBox>(Vec2{64, 64});
}

void Scene_Play::spawnProjectile(std::shared_ptr<Entity> player, Vec2 vel)
{
    auto entity = m_entities.addEntity("Projectile", (size_t)1);
    entity->addComponent<CTexture>(Vec2 {0,0}, Vec2 {32, 32}, m_game->assets().getTexture("fireball"));
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("fireball"), true);
    float angle = vel.angle();
    entity->addComponent<CTransform>(player->getComponent<CTransform>().pos+vel, vel, Vec2{2, 2}, angle, 400, true);
    entity->addComponent<CBoundingBox>(Vec2{16, 16});
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
            cameraZoom = cameraZoom*1.25;
        } else if (action.name() == "ZOOM OUT"){
            cameraZoom = cameraZoom*0.8;
        } else if (action.name() == "CAMERA FOLLOW"){
            cameraFollow = !cameraFollow;
        } 

        else if (action.name() == "LEVEL0") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/level0.png"));
        }else if (action.name() == "LEVEL1") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/level1.png"));
        }else if (action.name() == "LEVEL2") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/level2.png"));
        }else if (action.name() == "LEVEL3") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/level3.png"));
        }else if (action.name() == "LEVEL4") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/level4.png"));
        }
        
        else if (action.name() == "RESET") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/level3.png"));
        }
        for (auto p : m_entities.getEntities("Player")){
                if (action.name() == "UP") {
                    p->getComponent<CInputs>().up = true;
                }
                if (action.name() == "DOWN") {
                    p->getComponent<CInputs>().down = true; 
                }
                if (action.name() == "LEFT") {
                    p->getComponent<CInputs>().left = true;
                }
                if (action.name() == "RIGHT") {
                    p->getComponent<CInputs>().right = true;
                }
                if (action.name() == "SHIFT") {
                    p->getComponent<CInputs>().shift = true;
                }
                if (action.name() == "CTRL") {
                    p->getComponent<CInputs>().ctrl = true;
                }
                if (action.name() == "SHOOT") {
                    if (p->getComponent<CInputs>().canShoot) {
                        p->getComponent<CInputs>().shoot = true;
                        if ( p->getComponent<CTransform>().vel.isnull() ){
                            switch ( p->getComponent<CState>().state ){
                                case PlayerState::RUN_RIGHT:
                                    spawnProjectile(p, Vec2{1,0});
                                    break;
                                case PlayerState::RUN_LEFT:
                                    spawnProjectile(p, Vec2{-1,0});
                                    break;
                                case PlayerState::RUN_UP:
                                    spawnProjectile(p, Vec2{0,-1});
                                    break;
                                case PlayerState::RUN_DOWN:
                                    spawnProjectile(p, Vec2{0,1});
                                    break;
                            default:
                                break;
                            }
                        } else{
                            spawnProjectile(p, p->getComponent<CTransform>().vel);
                        }
                    }
                }
        }
    }
    else if (action.type() == "END") {
        for (auto p : m_entities.getEntities("Player")){
            if (action.name() == "DOWN") {
                p->getComponent<CInputs>().down = false;
            }
            else if (action.name() == "UP") {
                p->getComponent<CInputs>().up = false;
            }
            else if (action.name() == "LEFT") {
                p->getComponent<CInputs>().left = false;
            }
            else if (action.name() == "RIGHT") {
                p->getComponent<CInputs>().right = false;
            }
            else if (action.name() == "SHIFT") {
                p->getComponent<CInputs>().shift = false;
            }
            else if (action.name() == "CTRL") {
                p->getComponent<CInputs>().ctrl = false;
            }
            else if (action.name() == "SHOOT") {
                p->getComponent<CInputs>().shoot = false;
            }
        }
    }
}

void Scene_Play::update() {
    m_entities.update();
    if (!m_pause) {
        sMovement();
        sCollision();
        sStatus();
        m_currentFrame++;
    }
    sAnimation();
    sRender();
}

void Scene_Play::sMovement() {
    for (auto e : m_entities.getEntities()){    
        auto &transform = e->getComponent<CTransform>(); 
        if ( e->tag() == "Player" ){
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
                transform.tempo = 1.5f;
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
        transform.prevPos = transform.pos;
        if (!(transform.vel.isnull()) && transform.isMovable ){
            transform.pos += transform.vel.norm(transform.tempo*transform.speed/m_game->framerate());
        }
    }
}

void Scene_Play::sCollision() {
    for ( auto p : m_entities.getEntities("Player") )
    {
        for ( auto o : m_entities.getEntities("Obstacle") )
        {   
            if (m_physics.isCollided(p,o))
            {
                p->movePosition(m_physics.Overlap(p,o));
            }
        }
        for ( auto d : m_entities.getEntities("Dragon") )
        {   
            if (m_physics.isCollided(p,d))
            {
                p->movePosition(m_physics.Overlap(p,d)*15);
                p->takeDamage(1, m_currentFrame);
                d->getComponent<CAnimation>().animation = m_game->assets().getAnimation("waking_dragon");

            }
        }
        for ( auto k : m_entities.getEntities("Key") )
        {
            if (m_physics.isCollided(p,k))
            {
                k->kill();
                m_entities.getEntities("Player")[1]->getComponent<CTransform>().isMovable = true;
            }
        }
        for ( auto w : m_entities.getEntities("Water") )
        {
            if (m_physics.isStandingIn(p,w))
            {
                p->getComponent<CHealth>().HP = 0;
            }
        }
        for ( auto l : m_entities.getEntities("Lava") )
        {
            if (m_physics.isStandingIn(p,l))
            {
                p->getComponent<CHealth>().HP = 0;
            }
        }
    }

    for ( auto p : m_entities.getEntities("Projectile") ){
        for ( auto o : m_entities.getEntities("Obstacle") )
        {   
            if (m_physics.isCollided(p,o))
            {
                p->kill();
            }
        }
        for ( auto d : m_entities.getEntities("Dragon") )
        {   
            if (m_physics.isCollided(p,d))
            {
                p->kill();
                if (d->hasComponent<CHealth>()){
                    d->takeDamage(1, m_currentFrame);
                    if ( d->getComponent<CHealth>().HP <= 0 ){
                        d->kill();
                    }
                }
            }
        }
    }

    // for ( auto g : m_entities.getEntities("Goal") ) 
    // {
    //     if ( m_physics.isCollided(m_entities.getEntities("Player")[0],g) )
    //     {
    //         m_entities.getEntities("Player")[0]->getComponent<CTransform>().isMovable = false;
    //     }
    //     else if ( m_physics.isCollided(m_entities.getEntities("Player")[1],g) )
    //     {
    //         m_entities.getEntities("Player")[1]->getComponent<CTransform>().isMovable = false;
    //     }
    // }
}

void Scene_Play::sStatus() {
    for ( auto p : m_entities.getEntities("Player") ){
        if ( p->getComponent<CHealth>().HP <= 0 ){
                m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/level3.png"));
        }
    }
}

void Scene_Play::sAnimation() {
    for ( auto e : m_entities.getEntities() ){
        if (e->tag() == "Player"){
            if( e->getComponent<CTransform>().vel.x > 0 ) {
                changePlayerStateTo(PlayerState::RUN_RIGHT);
            }
            else if(e->getComponent<CTransform>().vel.x < 0) {
                changePlayerStateTo(PlayerState::RUN_LEFT);
            }
            else if(e->getComponent<CTransform>().vel.y > 0) {
                changePlayerStateTo(PlayerState::RUN_DOWN);
            }
            else if(e->getComponent<CTransform>().vel.y < 0) {
                changePlayerStateTo(PlayerState::RUN_UP);
            }
        }
        
        // // change player animation
        if (e->getComponent<CState>().changeAnimate) {
            std::string aniName;
            switch (e->getComponent<CState>().state) {
                case PlayerState::STAND:
                    aniName = "angelS";
                    break;
                case PlayerState::RUN_RIGHT:
                    aniName = "angelE";
                    break;
                case PlayerState::RUN_DOWN:
                    aniName = "angelS";
                    break;
                case PlayerState::RUN_LEFT:
                    aniName = "angelW";
                    break;
                case PlayerState::RUN_UP:
                    aniName = "angelN";
                    break;
                case PlayerState::RUN_RIGHT_DOWN:
                    aniName = "right_down";
                    break;
                case PlayerState::RUN_LEFT_DOWN:
                    aniName = "left_down";
                    break;
                case PlayerState::RUN_LEFT_UP:
                    aniName = "left_up";
                    break;
                case PlayerState::RUN_RIGHT_UP:
                    aniName = "right_up";
                    break;
                case PlayerState::RIGHT_SHOOT:
                    aniName = "angelE";
                    break;
            }
            e->addComponent<CAnimation>(m_game->assets().getAnimation(aniName), false);
            // e->setScale(e->getComponent<CTransform>().scale);
        }
        if (e->hasComponent<CAnimation>())
        {
            e->getComponent<CAnimation>().animation.update();
        }
        
    }


    // for (auto e : m_entityManager.getEntities()) {
    //     if (e->getComponent<CAnimation>().animation.hasEnded() &&
    //         !e->getComponent<CAnimation>().repeat) {
    //         e->destroy();
    //     }
    //     if (e->hasComponent<CAnimation>()) {
    //         e->getComponent<CAnimation>().animation.update();
    //     }
    // }
    // call entity->getComponent<CAnimation>().animation.update()
    // if the animation is not repeated, and it has ended, destroy the entity
}

void Scene_Play::sRender() {
    // Define the screen width and height (you might want to replace these with your actual values)
    int screenWidth = 1920;  // Width of your window
    int screenHeight = 1080; // Height of your window

    // Calculate the camera's position centered on the player
    if (cameraFollow){
        cameraPos = m_player->getComponent<CTransform>().pos - Vec2(screenWidth / 2, screenHeight / 2);
        if (cameraPos.x + (float)screenWidth > m_gridSize.x*levelSize.x){ cameraPos.x = m_gridSize.x*levelSize.x - (float)screenWidth;}     // right wall
        if (cameraPos.x < 0){cameraPos.x = 0;}      // left wall 
        if (cameraPos.y + (float)screenHeight > m_gridSize.y*levelSize.y){ cameraPos.y = m_gridSize.y*levelSize.y - (float)screenHeight;}     // bottom wall
        if (cameraPos.y < 0){ cameraPos.y = 0;}     // top wall
    } else{
        cameraPos = Vec2{0,0};
    }

    // Clear the screen with black
    SDL_SetRenderDrawColor(m_game->renderer(), 0, 0, 0, 255);
    SDL_RenderClear(m_game->renderer());

    if (m_drawTextures){
        for (auto e : m_entities.getEntities()){        
            if ( e->hasComponent<CTransform>() && e->hasComponent<CAnimation>()){

                auto& transform = e->getComponent<CTransform>();
                auto& animation = e->getComponent<CAnimation>().animation;

                SDL_Rect texRect;
                texRect.x = e->getComponent<CTexture>().pos.x;
                texRect.y = e->getComponent<CTexture>().pos.y;
                texRect.w = e->getComponent<CTexture>().size.x;
                texRect.h = e->getComponent<CTexture>().size.y;

                // Adjust the entity's position based on the camera position
                Vec2 adjustedPos = transform.pos - cameraPos;

                // Set the destination rectangle for rendering
                if (e->tag() == "Player"){
                    animation.setScale(transform.scale*cameraZoom);
                } else{
                    animation.setScale(transform.scale);
                }
                animation.setDestRect(adjustedPos - animation.getDestSize()/2);
                animation.setAngle(transform.angle);
                
                if (animation.frames() == 1){
                    SDL_RenderCopyEx(
                        m_game->renderer(), 
                        animation.getTexture(), 
                        &texRect, 
                        animation.getDestRect(),
                        animation.getAngle(),
                        NULL,
                        SDL_FLIP_NONE
                    );
                } 
                else {
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
                if (e->hasComponent<CHealth>()){
                    if ( e->getComponent<CHealth>().HP != e->getComponent<CHealth>().HP_max && (int)m_currentFrame - e->getComponent<CHealth>().damage_frame < e->getComponent<CHealth>().heart_frames ){
                        auto& animation_full = e->getComponent<CHealth>().animation_full;
                        auto& animation_half = e->getComponent<CHealth>().animation_half;
                        auto& animation_empty = e->getComponent<CHealth>().animation_empty;
                        Animation animation;
                        auto hearts = float(e->getComponent<CHealth>().HP)/2;

                        for (int i = 1; i <= e->getComponent<CHealth>().HP_max/2; i++)
                        {   
                            if ( hearts >= i ){
                                animation = animation_full;
                            } else if ( i-hearts == 0.5f ){
                                animation = animation_half;
                            } else{
                                animation = animation_empty;
                            }

                            animation.setScale(Vec2{0.5, 0.5});
                            animation.setDestRect(Vec2{
                                adjustedPos.x + (i - 1 - e->getComponent<CHealth>().HP_max / 4) * animation.getSize().x * animation.getScale().x, 
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
    }

    if (m_drawCollision){
        for (auto e : m_entities.getEntities()){      
            if ( e->hasComponent<CTransform>() && e->hasComponent<CBoundingBox>() ){
                auto& transform = e->getComponent<CTransform>();
                auto& box = e->getComponent<CBoundingBox>();

                // Adjust the collision box position based on the camera position
                SDL_Rect collisionRect;
                collisionRect.x = static_cast<int>(transform.pos.x - box.halfSize.x - cameraPos.x);
                collisionRect.y = static_cast<int>(transform.pos.y - box.halfSize.y - cameraPos.y);
                collisionRect.w = static_cast<int>(box.size.x);
                collisionRect.h = static_cast<int>(box.size.y);

                SDL_SetRenderDrawColor(m_game->renderer(), 255, 255, 255, 255);
                SDL_RenderDrawRect(m_game->renderer(), &collisionRect);
            }
        }
    }
}

void Scene_Play::onEnd() {
    m_game->quit();
}

void Scene_Play::setPaused(bool pause) {
    m_pause = pause;
}

void Scene_Play::changePlayerStateTo(PlayerState s) {
    auto& prev = m_player->getComponent<CState>().preState; 
    if (prev != s) {
        prev = m_player->getComponent<CState>().state;
        m_player->getComponent<CState>().state = s; 
        m_player->getComponent<CState>().changeAnimate = true;
    }
    else { 
        m_player->getComponent<CState>().changeAnimate = false;
    }
}

std::vector<bool> Scene_Play::neighborCheck(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height) {
    std::vector<std::string> friendlyPixels(1, "");
    std::vector<bool> neighbors(4, false); // {top, bottom, left, right}
    if ( pixel == "grass" || pixel == "dirt"){
        friendlyPixels = {"grass", "dirt", "key", "goal", "player_God", "player_Devil", "dragon", "water"};
    } else if ( pixel == "water" ){
        friendlyPixels = {"water", "bridge"};
    } else if (pixel == "lava"){
        friendlyPixels = {"lava", "bridge"};
    }
    else{
        friendlyPixels = {pixel};
    }
    for ( auto pix : friendlyPixels){
        if(!neighbors[0]){neighbors[0] = (y > 0 && pixelMatrix[y - 1][x] == pix);}           // top
        if(!neighbors[1]){neighbors[1] = (x < width - 1 && pixelMatrix[y][x + 1] == pix);}   // right
        if(!neighbors[2]){neighbors[2] = (y < height - 1 && pixelMatrix[y + 1][x] == pix);}  // bottom
        if(!neighbors[3]){neighbors[3] = (x > 0 && pixelMatrix[y][x - 1] == pix);}           // left
    }
    return neighbors;
}

std::vector<std::string> Scene_Play::neighborTag(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height) {
    
    std::vector<std::string> neighborsTags(4, "nan"); // {top, bottom, left, right}
    if(y > 0){neighborsTags[0] = pixelMatrix[y - 1][x];}            // top
    if(x < width - 1){neighborsTags[1] = pixelMatrix[y][x + 1];}    // right
    if(y < height - 1){neighborsTags[2] = pixelMatrix[y + 1][x];}   // bottom
    if(x > 0 ){neighborsTags[3] = pixelMatrix[y][x - 1];}           // left

    return neighborsTags;
}

int Scene_Play::getObstacleTextureIndex(const std::vector<bool>& neighbors) {
    int numObstacles = std::count(neighbors.begin(), neighbors.end(), true);
    if (numObstacles == 1) {
        if (neighbors[0]) return 12;    // Top
        if (neighbors[1]) return 1;     // Right
        if (neighbors[2]) return 4;     // Bottom
        if (neighbors[3]) return 3;     // Left
    } else if (numObstacles == 2) {
        if (!neighbors[0] && !neighbors[1]) return 7;  // Top & Right
        if (!neighbors[1] && !neighbors[2]) return 15; // Right & Bottom
        if (!neighbors[2] && !neighbors[3]) return 13; // Bottom & Left
        if (!neighbors[3] && !neighbors[0]) return 5;  // Left & Top
        if (!neighbors[0] && !neighbors[2]) return 2;  // Top & Bottom
        if (!neighbors[1] && !neighbors[3]) return 8; // Right & Left
    } else if (numObstacles == 3) {
        if (!neighbors[0]) return 6;    // Top is not an obstacle
        if (!neighbors[1]) return 11;   // Right is not an obstacle
        if (!neighbors[2]) return 14;   // Bottom is not an obstacle
        if (!neighbors[3]) return 9;    // Left is not an obstacle
    } else if (numObstacles == 4) {
        return 10; // All neighbors are obstacles
    }
    return 0; // No neighbors are obstacles
}

std::vector<std::vector<std::string>> Scene_Play::createPixelMatrix(Uint32* pixels, SDL_PixelFormat* format, int width, int height) {
    std::vector<std::vector<std::string>> pixelMatrix(height, std::vector<std::string>(width, ""));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Uint32 pixel = pixels[y * width + x];

            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, format, &r, &g, &b, &a);

            if ((int)r == 192 && (int)g == 192 && (int)b == 192) {
                pixelMatrix[y][x] = "obstacle";
            } else if ((int)r == 200 && (int)g == 240 && (int)b == 255) {
                pixelMatrix[y][x] = "cloud";
            } else if ((int)r == 203 && (int)g == 129 && (int)b == 56) {
                pixelMatrix[y][x] = "dirt";
            } else if ((int)r == 0 && (int)g == 255 && (int)b == 0) {
                pixelMatrix[y][x] = "grass";
            } else if ((int)r == 255 && (int)g == 255 && (int)b == 255) {
                pixelMatrix[y][x] = "player_God";
            } else if ((int)r == 0 && (int)g == 0 && (int)b == 0) {
                pixelMatrix[y][x] = "player_Devil";
            } else if ((int)r == 255 && (int)g == 0 && (int)b == 255) {
                pixelMatrix[y][x] = "key";
            } else if ((int)r == 255 && (int)g == 255 && (int)b == 0) {
                pixelMatrix[y][x] = "goal";
            // } else if ((int)r == 0 && (int)g == 255 && (int)b == 255) {
            //     pixelMatrix[y][x] = "out_of_bound_border";
            } else if ((int)r == 9 && (int)g == 88 && (int)b == 9) {
                pixelMatrix[y][x] = "dragon";
            } else if ((int)r == 255 && (int)g == 0 && (int)b == 0) {
                pixelMatrix[y][x] = "lava";
            } else if ((int)r == 0 && (int)g == 0 && (int)b == 255) {
                pixelMatrix[y][x] = "water";
            } else if ((int)r == 179 && (int)g == 0 && (int)b == 255) {
                pixelMatrix[y][x] = "bridge";
            } else {
                pixelMatrix[y][x] = "unknown";
            }
        }
    }
    return pixelMatrix;
}