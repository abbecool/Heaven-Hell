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
    registerAction(SDLK_r, "RESET");
    registerAction(SDLK_p, "PAUSE");
    registerAction(SDLK_t, "TOGGLE_TEXTURE");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    registerAction(SDLK_0, "LEVEL0");
    registerAction(SDLK_1, "LEVEL1");
    registerAction(SDLK_2, "LEVEL2");
    registerAction(SDLK_3, "LEVEL3");
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

    const int HEIGHT_PIX = 17;
    const int WIDTH_PIX = 30;
    
    std::vector<std::vector<std::string>> pixelMatrix = createPixelMatrix(pixels, loadedSurface->format, WIDTH_PIX, HEIGHT_PIX);

    // Process the pixels
    for (int y = 0; y < HEIGHT_PIX; ++y) {
        for (int x = 0; x < WIDTH_PIX; ++x) {
            const std::string& pixel = pixelMatrix[y][x];
            std::vector<bool> neighbors = neighborCheck(pixelMatrix, pixel, x, y, WIDTH_PIX, HEIGHT_PIX);
            int textureIndex = getObstacleTextureIndex(neighbors);

            if (pixel == "obstacle") {
                spawnObstacle(Vec2 {64*(float)x, 64*(float)y}, false, textureIndex);
            }
            else{

                if (pixel == "grass" || pixel == "dirt") {
                    spawnBackground(Vec2 {64*(float)x,64*(float)y}, false, textureIndex);
                } else {
                    spawnBackground(Vec2 {64*(float)x,64*(float)y}, false, getObstacleTextureIndex(neighborCheck(pixelMatrix, "grass", x, y, WIDTH_PIX, HEIGHT_PIX)));
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
                    spawnLava(Vec2 {64*(float)x,64*(float)y});
                } else if (pixel == "water") {
                    spawnWater(Vec2 {64*(float)x,64*(float)y}, textureIndex);
                } else if (pixel == "bridge") {
                    spawnBridge(Vec2 {64*(float)x,64*(float)y}, textureIndex);
                }
                else{

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

    auto entity = m_entities.addEntity("Player", (size_t)2);
    std::string tex = "m_texture_devil";
    // std::string tex = "Archer_idle";
    if (name == "God"){
        tex = "idas_angel_down";
        m_player = entity;
    }

    entity->addComponent<CTexture>(Vec2 {0,0}, Vec2 {64, 64}, m_game->assets().getTexture(tex));
    entity->addComponent<CAnimation>(m_game->assets().getAnimation(tex), false);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2{0,0}, Vec2{1, 1}, 0, movable);
    entity->addComponent<CBoundingBox>(Vec2 {32, 48});
    entity->addComponent<CInputs>();
    entity->addComponent<CState>(PlayerState::RUN_DOWN);
    entity->addComponent<CHealth>(10, 10, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
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
    auto entity = m_entities.addEntity("Dragon", (size_t)3);
    entity->addComponent<CAnimation>(m_game->assets().getAnimation(ani), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2 {2, 2}, 0, movable);
    entity->addComponent<CHealth>(20, 20, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
    entity->addComponent<CBoundingBox>(Vec2{128, 128});
}

void Scene_Play::spawnBackground(const Vec2 pos, bool movable, const int frame)
{
    auto entity = m_entities.addEntity("Background", (size_t)10);
    if (pos.y >= 1080/2){
        entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("dirt_sheet"));
        entity->addComponent<CAnimation> (m_game->assets().getAnimation("dirt_sheet"), true);
    }
    else{
        std::vector<int> ranArray = generateRandomArray(1, m_entities.getTotalEntities(), 0, 15);
        if (ranArray[0] == 0){
            entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("flower_sheet"));
            entity->addComponent<CAnimation> (m_game->assets().getAnimation("flower_sheet"), true);
        }
        else{
            entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("gras_sheet"));
            entity->addComponent<CAnimation> (m_game->assets().getAnimation("gras_sheet"), true);
        }
    }
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2 {0, 0}, Vec2{0.5, 0.5}, 0, movable);
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

void Scene_Play::spawnLava(const Vec2 pos)
{
    auto entity = m_entities.addEntity("Lava", (size_t)8);
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("lava_ani"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2{1,1}, 0, false);
    entity->addComponent<CBoundingBox>(Vec2{64, 64});
}

void Scene_Play::spawnWater(const Vec2 pos, const int frame)
{
    auto entity = m_entities.addEntity("Water", (size_t)8);
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("water"));
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("water"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, false);
    entity->addComponent<CBoundingBox>(Vec2{64, 64});
}

void Scene_Play::spawnBridge(const Vec2 pos, const int frame)
{
    auto entity = m_entities.addEntity("Bridge", (size_t)8);
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("bridge"));
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("bridge"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, false);
    entity->addComponent<CBoundingBox>(Vec2{64, 64});
}

void Scene_Play::spawnProjectile(std::shared_ptr<Entity> player)
{
    auto entity = m_entities.addEntity("Projectile", (size_t)1);
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("heart_full"), true);
    // Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    // entity->addComponent<CTransform>(pos,Vec2 {10, 0}, 800, true);
    if (player->getComponent<CTransform>().vel.isnull()){
        entity->addComponent<CTransform>(player->getComponent<CTransform>().pos, Vec2 {1,0}, 800*0.25, true);
    }else{
        entity->addComponent<CTransform>(player->getComponent<CTransform>().pos+player->getComponent<CTransform>().vel, player->getComponent<CTransform>().vel, 800, true);
    }

    entity->addComponent<CBoundingBox>(Vec2{32, 32});
    // std::cout << "projectile" << std::endl;
    // m_entities.sort();
}

void Scene_Play::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "TOGGLE_TEXTURE") {
            m_drawTextures = !m_drawTextures; 
        }
        else if (action.name() == "TOGGLE_COLLISION") { 
            m_drawCollision = !m_drawCollision; 
        }
        else if (action.name() == "TOGGLE_GRID") { 
            m_drawDrawGrid = !m_drawDrawGrid; 
        }
        else if (action.name() == "PAUSE") { 
            setPaused(!m_pause);
        }
        else if (action.name() == "QUIT") { 
            onEnd();
        }
        else if (action.name() == "LEVEL0") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/level0.png"));
        }else if (action.name() == "LEVEL1") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/level1.png"));
        }else if (action.name() == "LEVEL2") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/level2.png"));
        }else if (action.name() == "LEVEL3") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/level3.png"));
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
                        spawnProjectile(p);
                        // spawnProjectile(p->getComponent<CTransform>().pos);
                    }
                }
        }
    }
    else if (action.type() == "END") {
        for (auto p : m_entities.getEntities("Player")){
            if (action.name() == "UP") {
                p->getComponent<CInputs>().up = false;
            }
            if (action.name() == "DOWN") {
                p->getComponent<CInputs>().down = false;
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
        if ( e->tag() == "Player" ){
            e->getComponent<CTransform>().vel = { 0,0 };

            if (e->getComponent<CInputs>().up)
            {
                e->getComponent<CTransform>().vel.y--;
            }
            if (e->getComponent<CInputs>().down)
            {
                e->getComponent<CTransform>().vel.y++;
            }
            if (e->getComponent<CInputs>().left)
            {
                e->getComponent<CTransform>().vel.x--;
            }
            if (e->getComponent<CInputs>().right)
            {
                e->getComponent<CTransform>().vel.x++;
            }

            if (e->getComponent<CInputs>().shift)
            {
                e->getComponent<CTransform>().speed = 0.5*m_speed;
            }
            else if (e->getComponent<CInputs>().ctrl)
            {
                e->getComponent<CTransform>().speed = 2*m_speed;
            }
            else
            {
                e->getComponent<CTransform>().speed = m_speed;
            }
        }
        
        if ( e->tag() == "Dragon"){

            if (!(e->getComponent<CTransform>().vel.isnull()) && e->getComponent<CTransform>().isMovable )
            {
                e->getComponent<CTransform>().pos += e->getComponent<CTransform>().vel.norm(e->getComponent<CTransform>().speed/m_game->framerate());
            }
            if (e->getComponent<CTransform>().pos != e->getComponent<CTransform>().prevPos)
            {
                e->getComponent<CTransform>().isMovable = false;
            }
        }
        e->getComponent<CTransform>().prevPos = e->getComponent<CTransform>().pos;
        if (!(e->getComponent<CTransform>().vel.isnull()) && e->getComponent<CTransform>().isMovable )
        {
            e->getComponent<CTransform>().pos += e->getComponent<CTransform>().vel.norm(e->getComponent<CTransform>().speed/m_game->framerate());
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
                p->getComponent<CHealth>().HP--;
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
                // p->getComponent<CTransform>().isMovable = false;
                p->getComponent<CHealth>().HP = 0;
            }
        }
        for ( auto l : m_entities.getEntities("Lava") )
        {
            if (m_physics.isStandingIn(p,l))
            {
                // p->getComponent<CTransform>().isMovable = false;
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
                // std::cout << p->getComponent<CTransform>().pos.y << std::endl;
                // p->getComponent<CTransform>().isMovable = false;
            }
        }
        for ( auto d : m_entities.getEntities("Dragon") )
        {   
            if (m_physics.isCollided(p,d))
            {
                p->getComponent<CTransform>().isMovable = false;
                if (d->hasComponent<CHealth>()){
                    d->getComponent<CHealth>().HP--;
                    if ( d->getComponent<CHealth>().HP <= 0 ){
                        d->kill();
                    }
                }
            }
        }
    }

    for ( auto g : m_entities.getEntities("Goal") ) 
    {
        if ( m_physics.isCollided(m_entities.getEntities("Player")[0],g) )
        {
            m_entities.getEntities("Player")[0]->getComponent<CTransform>().isMovable = false;
        }
        else if ( m_physics.isCollided(m_entities.getEntities("Player")[1],g) )
        {
            m_entities.getEntities("Player")[1]->getComponent<CTransform>().isMovable = false;
        }
    }
}

void Scene_Play::sStatus() {
    if ( m_player->getComponent<CHealth>().HP <= 0 ){
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/level3.png"));
    }
}

void Scene_Play::sAnimation() {
    for ( auto e : m_entities.getEntities() ){
        if (e->tag() == "Player"){
            if(e->getComponent<CTransform>().vel.y < 0) {
                changePlayerStateTo(PlayerState::RUN_UP);
            }
            else if(e->getComponent<CTransform>().vel.y > 0) {
                changePlayerStateTo(PlayerState::RUN_DOWN);
            }
            else if(e->getComponent<CTransform>().vel.x < 0) {
                changePlayerStateTo(PlayerState::RUN_LEFT);
            }
            else if(e->getComponent<CTransform>().vel.x > 0) {
                changePlayerStateTo(PlayerState::RUN_RIGHT);
            }
            else if(e->getComponent<CTransform>().vel.x > 0) {
                changePlayerStateTo(PlayerState::RUN_RIGHT);
            }
            else if(e->getComponent<CInputs>().shoot) {
                changePlayerStateTo(PlayerState::RIGHT_SHOOT);
            }
            else {
                changePlayerStateTo(PlayerState::STAND);
            }
        }
        
        // // change player animation
        if (e->getComponent<CState>().changeAnimate) {
            std::string aniName;
            switch (e->getComponent<CState>().state) {
                case PlayerState::STAND:
                    aniName = "idas_angel_down";
                    break;
                case PlayerState::RUN_RIGHT:
                    aniName = "idas_angel_right";
                    break;
                case PlayerState::RUN_DOWN:
                    aniName = "idas_angel_down";
                    break;
                case PlayerState::RUN_LEFT:
                    aniName = "idas_angel_left";
                    break;
                case PlayerState::RUN_UP:
                    aniName = "idas_angel_up";
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
                    aniName = "idas_angel_right_shoot";
                    break;
            }
            e->addComponent<CAnimation>(m_game->assets().getAnimation(aniName), false);
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

                animation.setScale(transform.scale);
                animation.setDestRect(transform.pos - animation.getDestSize()/2);
                animation.setAngle(transform.angle);
                
                if (animation.frames() == 1){
                    SDL_RenderCopyEx(
                        m_game->renderer(), 
                        animation.getTexture(), 
                        &texRect, 
                        animation.getDestRect(),
                        0,
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
                        0,
                        NULL,
                        SDL_FLIP_NONE
                    );
                } 
                if (e->hasComponent<CHealth>()){
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

                        animation.setScale(Vec2 {1,1});
                        animation.setDestRect(Vec2{e->getComponent<CTransform>().pos.x+(i-1)*animation.getSize().x, e->getComponent<CTransform>().pos.y});
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

    if (m_drawCollision){
        for (auto e : m_entities.getEntities()){      
            if ( e->hasComponent<CTransform>() && e->hasComponent<CBoundingBox>() ){
                auto& transform = e->getComponent<CTransform>();
                auto& box = e->getComponent<CBoundingBox>();

                SDL_Rect collisionRect;
                collisionRect.x = static_cast<int>(transform.pos.x - box.halfSize.x);
                collisionRect.y = static_cast<int>(transform.pos.y - box.halfSize.y);
                collisionRect.w = static_cast<int>(box.size.x);
                collisionRect.h = static_cast<int>(box.size.y);

                SDL_SetRenderDrawColor(m_game->renderer(), 255, 255, 255, 255);
                SDL_RenderDrawRect(m_game->renderer(), &collisionRect);
            }
        }
    }
    // if (e->hasComponent<CHealth>()){
        // auto& animation_full = m_player->getComponent<CHealth>().animation_full;
        // auto& animation_half = m_player->getComponent<CHealth>().animation_half;
        // auto& animation_empty = m_player->getComponent<CHealth>().animation_empty;
        // Animation animation;
        // auto hearts = float(m_player->getComponent<CHealth>().HP)/2;

        // for (int i = 1; i <= m_player->getComponent<CHealth>().HP_max/2; i++)
        // {   
        //     if ( hearts >= i ){
        //         animation = animation_full;
        //     } else if ( i-hearts == 0.5f ){
        //         animation = animation_half;
        //     } else{
        //         animation = animation_empty;
        //     }

        //     animation.setScale(Vec2 {1,1});
        //     animation.setDestRect(Vec2{16+(i-1)*animation.getSize().x,16});
        //     SDL_RenderCopyEx(
        //         m_game->renderer(), 
        //         animation.getTexture(), 
        //         nullptr, 
        //         animation.getDestRect(),
        //         0,
        //         NULL,
        //         SDL_FLIP_NONE
        //     );
        // }

    // }
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
        friendlyPixels = {"grass", "dirt", "key", "goal", "player_God", "player_Devil", "dragon"};
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