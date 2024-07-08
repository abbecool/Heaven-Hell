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

Scene_Play::Scene_Play(Game* game)
    : Scene(game)
{
    init(m_levelPath);
}

void Scene_Play::init(const std::string& levelPath) {
    registerAction(SDLK_w, "UP");
    registerAction(SDLK_s, "DOWN");
    registerAction(SDLK_a, "LEFT");
    registerAction(SDLK_d, "RIGHT");
    registerAction(SDLK_LSHIFT, "SHIFT");
    registerAction(SDLK_LCTRL, "CTRL");
    registerAction(SDLK_ESCAPE, "QUIT");
    registerAction(SDLK_r, "RESET");
    registerAction(SDLK_p, "PAUSE");
    registerAction(SDLK_t, "TOGGLE_TEXTURE");
    registerAction(SDLK_c, "TOGGLE_COLLISION");
    loadLevel();
}

void Scene_Play::loadLevel(){

    const char* path = "assets/images/level3.png";
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
                spawnObstacle(Vec2 {64*(float)x, 64*(float)y}, Vec2 {64, 64}, false, textureIndex);
            }
            else{

                if (pixel == "grass" || pixel == "dirt") {
                    spawnBackground(Vec2 {64*(float)x,64*(float)y}, Vec2 {64,64}, false, textureIndex);
                } else {
                    spawnBackground(Vec2 {64*(float)x,64*(float)y}, Vec2 {64,64}, false, getObstacleTextureIndex(neighborCheck(pixelMatrix, "grass", x, y, WIDTH_PIX, HEIGHT_PIX)));
                }
                if (pixel == "cloud") {
                    spawnCloud(Vec2 {64*(float)x, 64*(float)y}, Vec2 {64, 64}, false, textureIndex);
                } else if (pixel == "player_God") {
                    spawnPlayer(Vec2 {64*(float)x,64*(float)y}, "God", true);
                } else if (pixel == "player_Devil") {
                    spawnPlayer(Vec2 {64*(float)x,64*(float)y}, "Devil", false);
                } else if (pixel == "key") {
                    spawnKey(Vec2 {64*(float)x,64*(float)y}, Vec2 {32,32}, "Devil", false);
                } else if (pixel == "goal") {
                    spawnGoal(Vec2 {64*(float)x,64*(float)y}, Vec2 {64,64}, false);
                // } else if (pixel == "out_of_bound_border") {
                //     spawnOutofboundBorder(Vec2 {64*(float)x,64*(float)y}, Vec2 {200,64}, false);
                } else if (pixel == "dragon") {
                    spawnDragon(Vec2 {64*(float)x,64*(float)y}, Vec2{128,128}, true, "snoring_dragon");
                } else if (pixel == "lava") {
                    spawnLava(Vec2 {64*(float)x,64*(float)y}, Vec2{64,64});
                } else if (pixel == "water") {
                    spawnWater(Vec2 {64*(float)x,64*(float)y}, Vec2{64,64}, textureIndex);
                } else if (pixel == "bridge") {
                    spawnBridge(Vec2 {64*(float)x,64*(float)y}, Vec2{64,64}, textureIndex);
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
    std::string tex = "m_texture_devil";
    if (name == "God"){
        tex = "m_texture_angel";
    }

    auto entity = m_entities.addEntity("Player", (size_t)2);
    entity->addComponent<CTransform>(pos, Vec2{0,0}, Vec2{0.25, 0.18}, 0, movable);
    entity->addComponent<CShape>(pos, Vec2{48, 48});
    entity->addComponent<CInputs>();
    entity->addComponent<CTexture>(Vec2 {0,0}, Vec2 {210, 270}, m_game->assets().getTexture(tex));
    entity->addComponent<CAnimation>(m_game->assets().getAnimation(tex), false);
    m_player = entity;
}

void Scene_Play::spawnObstacle(const Vec2 pos, const Vec2 size, bool movable, const int frame){
    auto entity = m_entities.addEntity("Obstacle", (size_t)9);
    entity->addComponent<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->addComponent<CShape>(pos, size);
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("rock_wall"));
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("rock_wall"), true);
}

void Scene_Play::spawnCloud(const Vec2 pos, const Vec2 size, bool movable, const int frame){
    auto entity = m_entities.addEntity("Obstacle", (size_t)9);
    entity->addComponent<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->addComponent<CShape>(pos, size);
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("cloud_sheet"));
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("cloud_sheet"), true);
}

void Scene_Play::spawnDragon(const Vec2 pos, const Vec2 size, bool movable, const std::string &ani) {
    auto entity = m_entities.addEntity("Dragon", (size_t)3);
    entity->addComponent<CTransform>(pos,Vec2 {0, 0}, Vec2 {2, 2}, 0,movable);
    entity->addComponent<CShape>(pos, size);
    entity->addComponent<CAnimation> (m_game->assets().getAnimation(ani), true);
}

void Scene_Play::spawnBackground(const Vec2 pos, const Vec2 size, bool movable, const int frame)
{
    auto entity = m_entities.addEntity("Background", (size_t)10);
    entity->addComponent<CTransform>(pos, Vec2 {0, 0}, movable);
    entity->addComponent<CShape>(pos, size);

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
}

void Scene_Play::spawnGoal(const Vec2 pos, const Vec2 size, bool movable)
{
    auto entity = m_entities.addEntity("Goal", (size_t)4);
    entity->addComponent<CTransform>(pos,Vec2 {0, 0}, Vec2{1,1}, 0, movable);
    entity->addComponent<CShape>(pos, size);
    entity->addComponent<CTexture>(Vec2 {0,0}, Vec2 {64,64}, m_game->assets().getTexture("m_texture_goal"));
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("m_texture_goal"), true);
}

void Scene_Play::spawnKey(const Vec2 pos, const Vec2 size, const std::string playerToUnlock, bool movable)
{
    auto entity = m_entities.addEntity("Key", (size_t)4);
    entity->addComponent<CTransform>(pos,Vec2 {0, 0}, Vec2 {0.15, 0.15}, 0, movable);
    entity->addComponent<CShape>(pos, size);
    // entity->cKey = std::make_shared<CKey>(playerToUnlock);
    entity->addComponent<CTexture>(Vec2 {0,0}, Vec2 {225, 225}, m_game->assets().getTexture("m_texture_key"));
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("m_texture_key"), true);
}

void Scene_Play::spawnLava(const Vec2 pos, const Vec2 size)
{
    auto entity = m_entities.addEntity("Lava", (size_t)8);
    entity->addComponent<CTransform>(pos,Vec2 {0, 0}, Vec2{1,1}, 0, false);
    entity->addComponent<CShape>(pos, size);
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("lava_ani"), true);
}

void Scene_Play::spawnWater(const Vec2 pos, const Vec2 size, const int frame)
{
    auto entity = m_entities.addEntity("Water", (size_t)8);
    entity->addComponent<CTransform>(pos,Vec2 {0, 0}, false);
    entity->addComponent<CShape>(pos, size);
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("water"));
    entity->addComponent<CAnimation> (m_game->assets().getAnimation("water"), true);
}

void Scene_Play::spawnBridge(const Vec2 pos, const Vec2 size, const int frame)
{
    auto entity = m_entities.addEntity("Bridge", (size_t)8);
    entity->addComponent<CTransform>(pos,Vec2 {0, 0}, false);
    entity->addComponent<CShape>(pos, size);
    entity->addComponent<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("bridge"));
    entity->addComponent<CAnimation>(m_game->assets().getAnimation("bridge"), true);
    // entity->addComponent<CAnimation>().animation.frames()
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
        }else if (action.name() == "RESET") { 
            m_game->changeScene("PLAY", std::make_shared<Scene_Play>(m_game));
        }
        for (auto p : m_entities.getEntities("Player")){
                if (action.name() == "UP") {
                    p->getComponent<CInputs>().up = true;
                }
                else if (action.name() == "DOWN") {
                    p->getComponent<CInputs>().down = true;
                }
                else if (action.name() == "LEFT") {
                    p->getComponent<CInputs>().left = true;
                }
                else if (action.name() == "RIGHT") {
                    p->getComponent<CInputs>().right = true;
                }
                else if (action.name() == "SHIFT") {
                    p->getComponent<CInputs>().shift = true;
                }
                else if (action.name() == "CTRL") {
                    p->getComponent<CInputs>().ctrl = true;
                }
                // else if (action.name() == "SHOOT") {
                //     if (p->getComponent<CInputs>().canShoot) {
                //         p->getComponent<CInputs>().shoot = true;
                //     }
                // }
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
            // else if (action.name() == "SHOOT") {
            //     p->getComponent<CInputs>().shoot = false;
            // }
        }
    }
}

void Scene_Play::update() {
    m_entities.update();
    if (!m_pause) {
        sMovement();
        sCollision();
        m_currentFrame++;
    }
    sAnimation();
    sRender();
}

void Scene_Play::sMovement() {
    for (auto p : m_entities.getEntities("Player"))
    {    
        p->getComponent<CTransform>().vel = { 0,0 };

        if (p->getComponent<CInputs>().up)
        {
            p->getComponent<CTransform>().vel.y = -1;
        }
        if (p->getComponent<CInputs>().down)
        {
            p->getComponent<CTransform>().vel.y = 1;
        }
        if (p->getComponent<CInputs>().left)
        {
            p->getComponent<CTransform>().vel.x = -1;
        }
        if (p->getComponent<CInputs>().right)
        {
            p->getComponent<CTransform>().vel.x = 1;
        }

        if (p->getComponent<CInputs>().shift)
        {
            p->getComponent<CTransform>().speed = 0.5*m_speed;
        }
        else if (p->getComponent<CInputs>().ctrl)
        {
            p->getComponent<CTransform>().speed = 2*m_speed;
        }
        else
        {
            p->getComponent<CTransform>().speed = m_speed;
        }
        
        
        p->getComponent<CTransform>().prevPos = p->getComponent<CTransform>().pos;

        if (!(p->getComponent<CTransform>().vel.isnull()) && p->getComponent<CTransform>().isMovable )
        {
            p->getComponent<CTransform>().pos += p->getComponent<CTransform>().vel.norm(p->getComponent<CTransform>().speed/m_game->framerate());
        }
    }
    for (auto d : m_entities.getEntities("Dragon"))
    {

        if (!(d->getComponent<CTransform>().vel.isnull()) && d->getComponent<CTransform>().isMovable )
        {
            d->getComponent<CTransform>().pos += d->getComponent<CTransform>().vel.norm(d->getComponent<CTransform>().speed/m_game->framerate());
        }
        if (d->getComponent<CTransform>().pos != d->getComponent<CTransform>().prevPos)
        {
            d->getComponent<CTransform>().isMovable = false;
        }
        d->getComponent<CTransform>().prevPos = d->getComponent<CTransform>().pos;
        
    }
}

void Scene_Play::sCollision() {
    for ( auto p : m_entities.getEntities("Player") )
    {
        // for ( auto o : m_entities.getEntities("Outofbound") )
        // {   
        //     if (m_physics.isCollided(p,o))
        //     {
        //         p->movePosition(m_physics.Overlap(p,o));
        //     }
        // }
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
                p->movePosition(m_physics.Overlap(p,d));
                d->addComponent<CAnimation>(m_game->assets().getAnimation("waking_dragon"), false);
                // d->getComponent<CTransform>().vel = Vec2{ -64*4, 0 };
                // d->cShape->setSize(Vec2(384, 192));
            }
        }
        // for ( auto b : m_entities.getEntities("Border") )
        // {   
        //     if (m_physics.isCollided(p,b))
        //     {
        //         p->movePosition(m_physics.Overlap(p,b));
        //     }
        // }
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
                p->getComponent<CTransform>().isMovable = false;
            }
        }
        for ( auto l : m_entities.getEntities("Lava") )
        {
            if (m_physics.isStandingIn(p,l))
            {
                p->getComponent<CTransform>().isMovable = false;
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

void Scene_Play::sAnimation() {
    for ( auto e : m_entities.getEntities() ){
        if (e->hasComponent<CAnimation>())
        {
            e->getComponent<CAnimation>().animation.update();
        }
        
    }
    // if(m_player->getComponent<CTransform>().velocity.y != 0) {
    //     m_player->getComponent<CInput>().canJump = false;
    //     if (m_player->getComponent<CInput>().shoot) {
    //         changePlayerStateTo(PlayerState::AIRSHOOT);
    //     }
    //     else {
    //         changePlayerStateTo(PlayerState::AIR);
    //     }
    // }
    // else {
    //     if (m_player->getComponent<CTransform>().velocity.x != 0) {
    //         if (m_player->getComponent<CInput>().shoot) {
    //             changePlayerStateTo(PlayerState::RUNSHOOT);
    //         }
    //         else {
    //             changePlayerStateTo(PlayerState::RUN);
    //         }
    //     }
    //     else {
    //         if (m_player->getComponent<CInput>().shoot) {
    //             changePlayerStateTo(PlayerState::STANDSHOOT);
    //         }
    //         else {
    //             changePlayerStateTo(PlayerState::STAND);
    //         }
    //     }
    // }
    
    // // change player animation
    // if (m_player->getComponent<CState>().changeAnimate) {
    //     std::string aniName;
    //     switch (m_player->getComponent<CState>().state) {
    //         case PlayerState::STAND:
    //             aniName = "Stand";
    //             break;
    //         case PlayerState::AIR:
    //             aniName = "Air";
    //             break;
    //         case PlayerState::RUN:
    //             aniName = "Run";
    //             break;
    //         case PlayerState::STANDSHOOT:
    //             aniName = "StandShoot";
    //             break;
    //         case PlayerState::AIRSHOOT:
    //             aniName = "AirShoot";
    //             break;
    //         case PlayerState::RUNSHOOT:
    //             aniName = "RunShoot";
    //             break;
    //     }
    //     m_player->addComponent<CAnimation>(
    //             m_game->assets().getAnimation(aniName), true
    //             );
    // }

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
            if ( e->hasComponent<CTransform>() && e->hasComponent<CShape>()){

                auto& transform = e->getComponent<CTransform>();
                auto& shape = e->getComponent<CShape>();

                SDL_Rect texRect;
                texRect.x = e->getComponent<CTexture>().pos.x;
                texRect.y = e->getComponent<CTexture>().pos.y;
                texRect.w = e->getComponent<CTexture>().size.x;
                texRect.h = e->getComponent<CTexture>().size.y;

                if (e->hasComponent<CAnimation>()){
                    auto& animation = e->getComponent<CAnimation>().animation;

                    animation.setDestRect(transform.pos.x, transform.pos.y, shape.size.x, shape.size.y);
                    animation.setScale(transform.scale);
                    animation.setAngle(transform.angle);
                    // animation.setSrcRect();

                    if (animation.frames() == 1){
                        SDL_RenderCopy( m_game->renderer(), 
                                        animation.getTexture(), 
                                        &texRect, 
                                        animation.getDestRect()
                                        );
                    } 
                    else {
                        SDL_RenderCopy( m_game->renderer(), 
                                        animation.getTexture(), 
                                        animation.getSrcRect(), 
                                        animation.getDestRect()
                                        );
                    }
                }            
            }
        }
    }

    if (m_drawCollision){
        for (auto e : m_entities.getEntities()){        
            if ( e->hasComponent<CTransform>() && e->hasComponent<CShape>() && e->hasComponent<CAnimation>()){
                e->getComponent<CShape>().pos = e->getComponent<CTransform>().pos;

                auto& transform = e->getComponent<CTransform>();
                auto& shape = e->getComponent<CShape>();

                SDL_Rect collisionRect;
                collisionRect.x = static_cast<int>(transform.pos.x);
                collisionRect.y = static_cast<int>(transform.pos.y);
                collisionRect.w = static_cast<int>(shape.size.x);
                collisionRect.h = static_cast<int>(shape.size.y);

                SDL_SetRenderDrawColor(m_game->renderer(), 255, 255, 255, 255);
                SDL_RenderDrawRect(m_game->renderer(), &collisionRect);
            }
        }
    }
}

void Scene_Play::onEnd() {
    // m_game->changeScene( "MENU", std::make_shared<Scene_Menu>(m_game));
    m_game->quit();
}

void Scene_Play::setPaused(bool pause) {
    m_pause = pause;
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