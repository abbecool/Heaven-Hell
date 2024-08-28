#include "Scene_Play.h"
#include "Scene_Menu.h"
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
#include <unordered_map>
#include <unordered_set>

Scene_Play::Scene_Play(Game* game, std::string levelPath, bool newGame)
    : Scene(game), m_levelPath(levelPath), m_newGame(newGame)
{
    init(m_levelPath);
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
    registerAction(SDLK_1, "LEVEL");
    registerAction(SDLK_2, "LEVEL5");
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
    levelSize = Vec2{ (float)WIDTH_PIX, (float)HEIGHT_PIX };
    auto format = loadedSurface->format;
    std::vector<std::vector<std::string>> pixelMatrix = createPixelMatrix(pixels, format, WIDTH_PIX, HEIGHT_PIX);

    // Unlock and free the surface
    SDL_UnlockSurface(loadedSurface);
    SDL_FreeSurface(loadedSurface);
    SDL_DestroyTexture(texture);


    // Process the pixels
    for (int y = 0; y < HEIGHT_PIX; ++y) {
        for (int x = 0; x < WIDTH_PIX; ++x) {
            const std::string& pixel = pixelMatrix[y][x];
            std::vector<bool> neighbors = neighborCheck(pixelMatrix, pixel, x, y, WIDTH_PIX, HEIGHT_PIX);
            std::vector<std::string> neighborsTags = neighborTag(pixelMatrix, pixel, x, y, WIDTH_PIX, HEIGHT_PIX);
            int textureIndex = getObstacleTextureIndex(neighbors);
            createDualGrid(pixelMatrix, x, y, HEIGHT_PIX, WIDTH_PIX);

            if (pixel == "obstacle") {
                spawnObstacle(Vec2 {64*(float)x, 64*(float)y}, false, textureIndex);
            }
            else{
                if (pixel == "grass" ) {
                    spawnGrass(Vec2 {64*(float)x,64*(float)y}, textureIndex);
                } else if (pixel == "dirt"){
                    spawnDirt(Vec2 {64*(float)x,64*(float)y}, textureIndex);
                } else {
                    if ( std::find(neighborsTags.begin(), neighborsTags.end(), "dirt") != neighborsTags.end() ){
                        spawnDirt(Vec2 {64*(float)x,64*(float)y}, getObstacleTextureIndex(neighborCheck(pixelMatrix, "dirt", x, y, WIDTH_PIX, HEIGHT_PIX)));
                    } else {
                        spawnGrass(Vec2 {64*(float)x,64*(float)y}, getObstacleTextureIndex(neighborCheck(pixelMatrix, "grass", x, y, WIDTH_PIX, HEIGHT_PIX)));
                    }   
                }
                if (pixel == "cloud") {
                    spawnCloud(Vec2 {64*(float)x, 64*(float)y}, false, textureIndex);
                } else if (pixel == "lava") {
                    spawnLava(Vec2 {64*(float)x,64*(float)y}, "Lava", textureIndex);
                } else if (pixel == "water") {
                    spawnWater(Vec2 {64*(float)x,64*(float)y}, "Water", textureIndex);
                } else if (pixel == "bridge") {
                    if ( std::find(neighborsTags.begin(), neighborsTags.end(), "water") != neighborsTags.end() ){
                        spawnWater(Vec2 {64*(float)x,64*(float)y}, "Background", getObstacleTextureIndex(neighborCheck(pixelMatrix, "water", x, y, WIDTH_PIX, HEIGHT_PIX)));
                    } else if ( std::find(neighborsTags.begin(), neighborsTags.end(), "lava") != neighborsTags.end() ){
                        spawnLava(Vec2 {64*(float)x,64*(float)y}, "Background", getObstacleTextureIndex(neighborCheck(pixelMatrix, "lava", x, y, WIDTH_PIX, HEIGHT_PIX)));
                    }
                    spawnBridge(Vec2 {64*(float)x,64*(float)y}, textureIndex);
                }
            }
        }
    }

    spawnPlayer();
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

void Scene_Play::spawnPlayer(){

    auto entity = m_entities.addEntity("Player", (size_t)3);
    size_t pos_x;
    size_t pos_y;
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

    entity->addComponent<CAnimation>(m_game->assets().getAnimation("wizIdle"), true);
    entity->addComponent<CShadow>(m_game->assets().getAnimation("shadow"), false);

    entity->addComponent<CInputs>();
    entity->addComponent<CState>(PlayerState::RUN_DOWN);

    entity->addComponent<CDamage>(m_playerConfig.DAMAGE, 6);
    entity->addComponent<CHealth>(hp, m_playerConfig.HP, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
}

void Scene_Play::spawnObstacle(const Vec2 pos, bool movable, const int frame){
    auto entity = m_entities.addEntity("Obstacle", (size_t)8);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2 {0, 0}, Vec2 {0.5,0.5}, 0, movable);
    entity->addComponent<CBoundingBox>(Vec2 {64, 64});
}

void Scene_Play::spawnCloud(const Vec2 pos, bool movable, const int frame){
    auto entity = m_entities.addEntity("Obstacle", (size_t)8);
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
    entity->addComponent<CShadow>(m_game->assets().getAnimation("shadow"), false);
    entity->addComponent<CDamage>(2, 30);

}

void Scene_Play::spawnGrass(const Vec2 pos, const int frame)
{
    auto entity = m_entities.addEntity("Background", (size_t)10);
    std::vector<int> ranArray = generateRandomArray(1, m_entities.getTotalEntities(), 0, 15);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2 {0, 0}, Vec2{0.5, 0.5}, 0, false);
}

void Scene_Play::spawnDirt(const Vec2 pos, const int frame)
{
    auto entity = m_entities.addEntity("Background", (size_t)10);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2 {0, 0}, Vec2{0.5, 0.5}, 0, false);
}

void Scene_Play::spawnGoal(const Vec2 pos, bool movable)
{
    auto entity = m_entities.addEntity("Goal", (size_t)4);
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("checkpoint_idle"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, Vec2{4,4}, 0, movable);
    entity->addComponent<CBoundingBox>(Vec2{32, 32});
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

void Scene_Play::spawnLava(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_entities.addEntity(tag, (size_t)8);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, false);
    entity->addComponent<CBoundingBox>(Vec2{64, 64});
}

void Scene_Play::spawnWater(const Vec2 pos, const std::string tag, const int frame)
{
    auto entity = m_entities.addEntity(tag, (size_t)8);
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
    entity->addComponent<CTransform>(player->getComponent<CTransform>().pos, vel, Vec2{2, 2}, angle, 400, true);
    entity->addComponent<CBoundingBox>(Vec2{12, 12});
    entity->addComponent<CDamage>(player->getComponent<CDamage>().damage, player->getComponent<CDamage>().speed); // damage speed 6 = frames between attacking
    m_entities.sort();
}

void Scene_Play::spawnCoin(Vec2 pos, const size_t layer)
{
    auto entity = m_entities.addEntity("Coin", layer);
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("coin"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2{0,0}, Vec2{4,4}, 0, false);
    entity->addComponent<CBoundingBox>(Vec2{24, 24});
    entity->addComponent<CShadow>(m_game->assets().getAnimation("shadow"), false);
}

void Scene_Play::spawnSmallEnemy(Vec2 pos, const size_t layer)
{
    auto entity = m_entities.addEntity("Enemy", layer);
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("rooter"), true);
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid, Vec2{0,0}, Vec2{4,4}, 0, 150, true);
    entity->addComponent<CBoundingBox>(Vec2{32, 48});
    entity->addComponent<CPathfind>(m_player->getComponent<CTransform>().pos);
    entity->addComponent<CShadow>(m_game->assets().getAnimation("shadow"), false);
    entity->addComponent<CHealth>(4, 4, m_game->assets().getAnimation("heart_full"), m_game->assets().getAnimation("heart_half"), m_game->assets().getAnimation("heart_empty"));
}

void Scene_Play::spawnDualTile(const Vec2 pos, std::string tile, const int frame)
{   
    size_t layer = 10;
    if (tile == "water"){layer=layer-1;}
    if (tile == "lava"){layer=layer-1;}
    if (tile == "cloud"){layer=layer-2;}
    if (tile == "obstacle"){
        layer = layer-2;
        tile = std::string("mountain");}
    auto entity = m_entities.addEntity("DualTile", layer);
    entity->addComponent<CAnimation> (m_game->assets().getAnimation(tile+"_dual_sheet"), true);
    
    float size;
    float scale;
    size_t rows = 4;
    if (entity->getComponent<CAnimation>().animation.getSize().x == 64){
        size = 16;
        scale = 1;
    } else{
        size = 32;
        scale = 0.5;
    }
    
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*size, (float)(int)(frame/rows)*size}, Vec2 {size, size}, m_game->assets().getTexture(tile+"_dual_sheet"));
    Vec2 midGrid = gridToMidPixel(pos.x, pos.y, entity);
    entity->addComponent<CTransform>(midGrid,Vec2 {0, 0}, false);
    entity->getComponent<CTransform>().scale = Vec2{scale,scale};
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
        } else if (action.name() == "SAVE"){
            saveGame("game_save.txt");
        } else if (action.name() == "RESET") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game, "assets/images/levels/level0.png", true));
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
                if (action.name() == "SHOOT MOUSE"){
                    if (p->getComponent<CInputs>().canShoot) {
                        p->getComponent<CInputs>().shoot = true;
                        spawnProjectile(p, getMousePosition()-p->getComponent<CTransform>().pos+cameraPos);
                    }
                }
                if (action.name() == "SHOOT") {
                    if (p->getComponent<CInputs>().canShoot) {
                        p->getComponent<CInputs>().shoot = true;
                    }
                }
        }
    }
    else if (action.type() == "END") {
        for (auto p : m_entities.getEntities("Player")){
            if (action.name() == "DOWN") {
                p->getComponent<CInputs>().down = false;
            } if (action.name() == "UP") {
                p->getComponent<CInputs>().up = false;
            } if (action.name() == "LEFT") {
                p->getComponent<CInputs>().left = false;
            } if (action.name() == "RIGHT") {
                p->getComponent<CInputs>().right = false;
            } if (action.name() == "SHIFT") {
                p->getComponent<CInputs>().shift = false;
            } if (action.name() == "CTRL") {
                p->getComponent<CInputs>().ctrl = false;
            } if (action.name() == "SHOOT") {
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
                        case PlayerState::STAND:
                            spawnProjectile(p, Vec2{0,1});
                            break;
                    default:
                        spawnProjectile(p, Vec2{0,1});
                        break;
                    }
                } else{
                    spawnProjectile(p, p->getComponent<CTransform>().vel);
                    // spawnProjectile(p, getMousePosition()-p->getComponent<CTransform>().pos+cameraPos);

                }
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
        sAnimation();
        m_currentFrame++;
    }
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

        if (e->hasComponent<CPathfind>()) {
            Vec2& target = e->getComponent<CPathfind>().target;
            if ((target - transform.pos).length() < 64*5) {
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
    }
}

void Scene_Play::sCollision() {
    
    auto p = m_player;
    for ( auto g : m_entities.getEntities("Goal") )
    {   
        if (m_physics.isCollided(p,g))
        {
            if (g->getComponent<CAnimation>().animation.getName() != "checkpoint_wave"){
                g->addComponent<CAnimation>(m_game->assets().getAnimation("checkpoint_wave"), true);
                saveGame("game_save.txt");
            }
        }
    }
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
            if (d->hasComponent<CDamage>()){
                p->movePosition(m_physics.Overlap(p,d)*15);
                p->takeDamage(d->getComponent<CDamage>().damage, m_currentFrame);
                d->addComponent<CAnimation>(m_game->assets().getAnimation("waking_dragon"), false);
            }
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
    for ( auto c : m_entities.getEntities("Coin") ){
        if (m_physics.isCollided(p,c))
        {
            c->kill();
        }
    }
    for ( auto e : m_entities.getEntities("Enemy") )
    {   
        if (m_physics.isCollided(e,p))
        {
            e->movePosition(m_physics.Overlap(e,p));
        }
        for ( auto e1 : m_entities.getEntities("Enemy") )
        {   if (e != e1) {
                if (m_physics.isCollided(e,e1))
                {
                    e->movePosition(m_physics.Overlap(e,e1));
                }
            }
        }
        for ( auto w : m_entities.getEntities("Water") )
        {   
            if (m_physics.isCollided(e,w))
            {
                e->movePosition(m_physics.Overlap(e,w));
            }
        }
    }

    for ( auto p : m_entities.getEntities("Projectile") ){
        for ( auto o : m_entities.getEntities("Obstacle") )
        {   
            if (m_physics.isCollided(p,o))
            {
                if ( p->getComponent<CTransform>().isMovable ){
                    p->addComponent<CAnimation>(m_game->assets().getAnimation("fireball_explode"), false);
                    p->getComponent<CTransform>().isMovable = false;
                }
            }
        }
        for ( auto d : m_entities.getEntities("Dragon") )
        {   
            if (m_physics.isCollided(p,d))
            {
                if (d->hasComponent<CHealth>() && p->hasComponent<CDamage>()){
                    d->takeDamage(p->getComponent<CDamage>().damage, m_currentFrame);
                    d->addComponent<CAnimation>(m_game->assets().getAnimation("waking_dragon"), false);
                }
                if ( p->getComponent<CTransform>().isMovable ){
                    p->addComponent<CAnimation>(m_game->assets().getAnimation("fireball_explode"), false);
                    p->getComponent<CTransform>().isMovable = false;
                    p->removeComponent<CDamage>();
                }
                if ( d->getComponent<CHealth>().HP <= 0 ){
                    d->kill();
                }
            }
        }
        for ( auto e : m_entities.getEntities("Enemy") )
        {   
            if (m_physics.isCollided(p,e))
            {
                if (e->hasComponent<CHealth>() && p->hasComponent<CDamage>()){
                    e->takeDamage(p->getComponent<CDamage>().damage, m_currentFrame);
                }
                if ( p->getComponent<CTransform>().isMovable ){
                    p->addComponent<CAnimation>(m_game->assets().getAnimation("fireball_explode"), false);
                    p->getComponent<CTransform>().isMovable = false;
                    p->removeComponent<CDamage>();
                }
                if ( e->getComponent<CHealth>().HP <= 0 ){
                    spawnCoin(e->getComponent<CTransform>().pos, 4);
                    e->kill();
                }
            }
        }
    }

    for ( auto e : m_entities.getEntities("Enemy") ){
        for ( auto o : m_entities.getEntities("Obstacle") )
        {   
            if (m_physics.isCollided(e,o))
            {
                e->movePosition(m_physics.Overlap(e,o));
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
    if( m_player->getComponent<CTransform>().vel.isnull() ) {
        changePlayerStateTo(PlayerState::STAND);
    } else if( m_player->getComponent<CTransform>().vel.x > 0 ) {
        changePlayerStateTo(PlayerState::RUN_RIGHT);
    } else if(m_player->getComponent<CTransform>().vel.x < 0) {
        changePlayerStateTo(PlayerState::RUN_LEFT);
    } else if(m_player->getComponent<CTransform>().vel.y > 0) {
        changePlayerStateTo(PlayerState::RUN_DOWN);
    } else if(m_player->getComponent<CTransform>().vel.y < 0) {
        changePlayerStateTo(PlayerState::RUN_UP);
    }

    // // change player animation
    if (m_player->getComponent<CState>().changeAnimate) {
        std::string aniName;
        switch (m_player->getComponent<CState>().state) {
            case PlayerState::STAND:
                aniName = "wizIdle";
                break;
            case PlayerState::RUN_RIGHT:
                aniName = "wizWalkE";
                break;
            case PlayerState::RUN_DOWN:
                aniName = "wizWalkS";
                break;
            case PlayerState::RUN_LEFT:
                aniName = "wizWalkW";
                break;
            case PlayerState::RUN_UP:
                aniName = "wizWalkN";
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
        m_player->addComponent<CAnimation>(m_game->assets().getAnimation(aniName), true);
    }
    for ( auto e : m_entities.getEntities() ){
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

                // Adjust the entity's position based on the camera position
                Vec2 adjustedPos = transform.pos - cameraPos;

                if ( e->hasComponent<CShadow>() ){
                    auto& shadow = e->getComponent<CShadow>();

                    // Set the destination rectangle for rendering
                    shadow.animation.setScale(transform.scale*cameraZoom);
                    shadow.animation.setDestRect(adjustedPos - shadow.animation.getDestSize()/2);
                    shadow.animation.setAngle(transform.angle);

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
                SDL_Rect texRect;
                texRect.x = e->getComponent<CTexture>().pos.x;
                texRect.y = e->getComponent<CTexture>().pos.y;
                texRect.w = e->getComponent<CTexture>().size.x;
                texRect.h = e->getComponent<CTexture>().size.y;


                // Set the destination rectangle for rendering
                animation.setScale(transform.scale*cameraZoom);
                animation.setDestRect(adjustedPos - animation.getDestSize()/2);
                animation.setAngle(transform.angle);

                if (e->tag() == "DualTile"){
                    animation.setDestSize(Vec2{(float)64, (float)64});
                }

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
                } else {
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
        // Attemp to render coin balance in corner.
        // SDL_RenderCopyEx(
        //             m_game->renderer(), 
        //             m_game->assets().getTexture("coin"), 
        //             nullptr, 
        //             nullptr,
        //             0,
        //             NULL,
        //             SDL_FLIP_NONE
        //         );
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
    m_game->changeScene("Menu", std::make_shared<Scene_Menu>(m_game));
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

void Scene_Play::createDualGrid(std::vector<std::vector<std::string>> pixelMatrix, int x, int y, const int HEIGHT_PIX, const int WIDTH_PIX) {
    std::vector<std::string> tileQ = std::vector<std::string>(4, "");
    int textureIndex;
    tileQ[1] = pixelMatrix[y][x];   //Q4
    if (x>0)        {tileQ[0] = pixelMatrix[y][x-1];}    else {tileQ[0] = pixelMatrix[y][x];}  // Q3
    if (y>0)        {tileQ[2] = pixelMatrix[y-1][x];}    else {tileQ[2] = pixelMatrix[y][x];}  // Q1
    if (x>0 && y>0) {tileQ[3] = pixelMatrix[y-1][x-1];}  else {tileQ[3] = pixelMatrix[y][x];}  // Q2

    std::unordered_map<std::string, std::unordered_set<std::string>> friendlyNeighbors = {
        {"grass", {"key", "goal", "player_God", "dragon"}},
        {"dirt", {"key", "goal", "player_Devil", "dragon"}}
    };
            
    for (std::string tile : {"grass", "dirt", "water", "lava", "cloud", "obstacle", "bridge"})
    {
        if ( std::find(tileQ.begin(), tileQ.end(), "bridge") != tileQ.end() ){

            if ( std::find(tileQ.begin(), tileQ.end(), "water") != tileQ.end() ){
                std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [](const std::string& str) {
                    return (str == "bridge" ) ? "water" : str;
                });
            } else if ( std::find(tileQ.begin(), tileQ.end(), "lava") != tileQ.end() ){
                    std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [](const std::string& str) {
                    return (str == "bridge" ) ? "lava" : str;
                });
            } else {
                std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [](const std::string& str) {
                    return (str == "bridge" ) ? "" : str;
                });
            }
        }
        
         // Transform the vector based on friendly neighbors for the current tile type
        std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [&](const std::string& str) {
            // Check if the current string is a friendly neighbor for the current tile
            if (friendlyNeighbors[tile].count(str)) {
                return tile; // Replace it with the current tile
            }
            return str; // Keep the original if it's not a friendly neighbor
        });

        int numTiles = std::count(tileQ.begin(), tileQ.end(), tile);
        std::unordered_set<std::string> uniqueStrings(tileQ.begin(), tileQ.end());
        if (numTiles > 0){
            if (numTiles == 4) {
                textureIndex = 6; // All quadrants are tiles
            } else if (numTiles == 3) {
                if (tileQ[0] != tile) textureIndex = 10;
                if (tileQ[1] != tile) textureIndex = 7;
                if (tileQ[2] != tile) textureIndex = 2;
                if (tileQ[3] != tile) textureIndex = 5;
            } else if (numTiles == 2) {
                if (tileQ[0] != tile && tileQ[1] != tile) textureIndex = 9;
                if (tileQ[1] != tile && tileQ[2] != tile) textureIndex = 11;
                if (tileQ[2] != tile && tileQ[3] != tile) textureIndex = 3;
                if (tileQ[3] != tile && tileQ[0] != tile) textureIndex = 1;
                if (tileQ[0] != tile && tileQ[2] != tile) textureIndex = 4;
                if (tileQ[1] != tile && tileQ[3] != tile) textureIndex = 14; 
                if (uniqueStrings.size() == 3 && (tile == "grass")){
                    if (tileQ[0] == tile && tileQ[1] == tile) textureIndex = 19;
                    if (tileQ[1] == tile && tileQ[2] == tile) textureIndex = 17;
                    if (tileQ[2] == tile && tileQ[3] == tile) textureIndex = 16;
                    if (tileQ[3] == tile && tileQ[0] == tile) textureIndex = 18;
                }
            } if (numTiles == 1) {
                if (tileQ[0] == tile) textureIndex = 0;
                if (tileQ[1] == tile) textureIndex = 13;
                if (tileQ[2] == tile) textureIndex = 8;
                if (tileQ[3] == tile) textureIndex = 15;
                if (uniqueStrings.size() == 3 && (tile == "grass")){
                    if (tileQ[0] == tile && tileQ[2] == tileQ[3]) textureIndex = 20;
                    if (tileQ[0] == tile && tileQ[1] == tileQ[2]) textureIndex = 25;

                    if (tileQ[1] == tile && tileQ[0] == tileQ[3]) textureIndex = 23;
                    if (tileQ[1] == tile && tileQ[2] == tileQ[3]) textureIndex = 24;

                    if (tileQ[2] == tile && tileQ[0] == tileQ[1]) textureIndex = 22;
                    if (tileQ[2] == tile && tileQ[0] == tileQ[3]) textureIndex = 27;

                    if (tileQ[3] == tile && tileQ[1] == tileQ[2]) textureIndex = 21;
                    if (tileQ[3] == tile && tileQ[0] == tileQ[1]) textureIndex = 26;
                }
            }
            if (tile != ""){
                spawnDualTile(Vec2 {64*(float)x-32, 64*(float)y-32}, tile, textureIndex);
            }
        }
    }
}
// 1  Q4
// 0  Q3
// 2  Q1
// 3  Q2