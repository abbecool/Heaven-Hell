// #include "Scene.cpp"
// #include "Scene_Menu.h"
#include "Scene_Play.h"
#include "Assets.h"
#include "Game.h"
// #include "Physics.h"
// #include "Physics.cpp"
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
    loadLevel();
}

std::vector<bool> Scene_Play::neighborCheck(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height) {
    std::vector<bool> neighbors(4, false); // {top, bottom, left, right}
    neighbors[0] = (y > 0 && pixelMatrix[y - 1][x] == pixel);           // top
    neighbors[1] = (x < width - 1 && pixelMatrix[y][x + 1] == pixel);   // right
    neighbors[2] = (y < height - 1 && pixelMatrix[y + 1][x] == pixel);  // bottom
    neighbors[3] = (x > 0 && pixelMatrix[y][x - 1] == pixel);           // left
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
            } else if ((int)r == 203 && (int)g == 129 && (int)b == 56) {
                pixelMatrix[y][x] = "background";
            } else if ((int)r == 255 && (int)g == 255 && (int)b == 255) {
                pixelMatrix[y][x] = "player_God";
            } else if ((int)r == 0 && (int)g == 0 && (int)b == 0) {
                pixelMatrix[y][x] = "player_Devil";
            } else if ((int)r == 255 && (int)g == 0 && (int)b == 255) {
                pixelMatrix[y][x] = "key";
            } else if ((int)r == 255 && (int)g == 255 && (int)b == 0) {
                pixelMatrix[y][x] = "goal";
            } else if ((int)r == 0 && (int)g == 255 && (int)b == 255) {
                pixelMatrix[y][x] = "out_of_bound_border";
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

void Scene_Play::loadLevel(){

    const char* path = "assets/images/level2.png";
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
                spawnBackground(Vec2 {64*(float)x,64*(float)y}, Vec2 {64,64}, false);
                if (pixel == "player_God") {
                    spawnPlayer(Vec2 {64*(float)x,64*(float)y}, "God", true);
                } else if (pixel == "player_Devil") {
                    spawnPlayer(Vec2 {64*(float)x,64*(float)y}, "Devil", false);
                } else if (pixel == "key") {
                    spawnKey(Vec2 {64*(float)x,64*(float)(y-1)}, Vec2 {64,64}, "Devil", false);
                } else if (pixel == "goal") {
                    spawnGoal(Vec2 {64*(float)x,64*(float)y}, Vec2 {64,64}, false);
                } else if (pixel == "out_of_bound_border") {
                    spawnOutofboundBorder(Vec2 {64*(float)x,64*(float)y}, Vec2 {200,64}, false);
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
    std::string tex;
    if (name == "God")
    {
        tex = "m_texture_angel";
    }
    else
    {
        tex = "m_texture_devil";
    }

    auto entity = m_entities.addEntity("Player", (size_t)2);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, Vec2{64, 64}, 255, 0, 0, 255);
    entity->cInputs = std::make_shared<CInputs>();
    entity->cName = std::make_shared<CName>(name);
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0,0}, Vec2 {210, 270}, m_game->assets().getTexture(tex));
    entity->cTexture->texture = m_game->assets().getTexture(tex);
    m_player = entity;
}

void Scene_Play::spawnObstacle(const Vec2 pos, const Vec2 size, bool movable, const int frame){
    auto entity = m_entities.addEntity("Obstacle", (size_t)9);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 100, 100, 0, 255);
    // std::cout << entity->cShape->rect->w << std::endl;
    entity->cName = std::make_shared<CName>("Obstacle");
    std::cout << size.x << std::endl;
    entity->cTexture = std::make_shared<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("rock_wall"));
    entity->cTexture->texture = m_game->assets().getTexture("rock_wall");
}

void Scene_Play::spawnDragon(const Vec2 pos, const Vec2 size, bool movable, const std::string &ani) {
    auto entity = m_entities.addEntity("Dragon", (size_t)3);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 100, 100, 0, 255);
    entity->cName = std::make_shared<CName>("Dragon");
    entity->cAnimation = std::make_shared<CAnimation> (m_game->assets().getAnimation(ani), true);
}


void Scene_Play::spawnWorldBorder(const Vec2 pos, const Vec2 size, bool movable)
{
    auto entity = m_entities.addEntity("Border", (size_t)9);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 100, 100, 0, 255);
    entity->cName = std::make_shared<CName>("Border");
}

void Scene_Play::spawnOutofboundBorder(const Vec2 pos, const Vec2 size, bool movable)
{
    auto entity = m_entities.addEntity("Outofbound", (size_t)9);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 100, 100, 0, 255);
    entity->cName = std::make_shared<CName>("Outofbound");
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0, 0}, Vec2 {200,64}, m_game->assets().getTexture("m_texture_outofbound"));
    entity->cTexture->texture = m_game->assets().getTexture("m_texture_outofbound");
}

void Scene_Play::spawnBackground(const Vec2 pos, const Vec2 size, bool movable)
{
    auto entity = m_entities.addEntity("Background", (size_t)10);
    entity->cTransform = std::make_shared<CTransform>(pos, Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 255, 255, 255, 255);
    entity->cName = std::make_shared<CName>("Background ");

    std::vector<int> ranArray = generateRandomArray(1, m_entities.getTotalEntities(), 0, 12);
    if (ranArray[0] == 0)
        entity->cTexture = std::make_shared<CTexture>(Vec2 {0, 0}, Vec2 {64, 64}, m_game->assets().getTexture("m_texture_background"));
    else
        entity->cTexture = std::make_shared<CTexture>(Vec2 {0, 0}, Vec2 {2, 2}, m_game->assets().getTexture("m_texture_background"));
}

void Scene_Play::spawnGoal(const Vec2 pos, const Vec2 size, bool movable)
{
    auto entity = m_entities.addEntity("Goal", (size_t)4);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 20, 200, 20, 10);
    entity->cName = std::make_shared<CName>("Goal");
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0,0}, Vec2 {64,64}, m_game->assets().getTexture("m_texture_goal"));
    entity->cTexture->texture = m_game->assets().getTexture("m_texture_goal");
}

void Scene_Play::spawnKey(const Vec2 pos, const Vec2 size, const std::string playerToUnlock, bool movable)
{
    auto entity = m_entities.addEntity("Key", (size_t)4);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 120, 120, 20, 200);
    entity->cName = std::make_shared<CName>("Key");
    entity->cKey = std::make_shared<CKey>(playerToUnlock);
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0,0}, Vec2 {225, 225}, m_game->assets().getTexture("m_texture_key"));
    entity->cTexture->texture = m_game->assets().getTexture("m_texture_key");
}

void Scene_Play::spawnLava(const Vec2 pos, const Vec2 size)
{
    auto entity = m_entities.addEntity("Lava", (size_t)8);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, false);
    entity->cShape = std::make_shared<CShape>(pos, size, 255, 255, 255, 255);
    entity->cName = std::make_shared<CName>("Lava");
    // entity->cTexture = std::make_shared<CTexture>(Vec2 {0,0}, Vec2 {64, 64}, m_game->assets().getTexture("lava"));
    // entity->cTexture->texture = m_game->assets().getTexture("lava");
    const std::string & ani = "lava_ani";
    entity->cAnimation = std::make_shared<CAnimation> (m_game->assets().getAnimation(ani), true);
}

void Scene_Play::spawnWater(const Vec2 pos, const Vec2 size, const int frame)
{
    auto entity = m_entities.addEntity("Water", (size_t)8);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, false);
    entity->cShape = std::make_shared<CShape>(pos, size, 255, 255, 255, 255);
    entity->cName = std::make_shared<CName>("Water");
    entity->cTexture = std::make_shared<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("water"));
    // entity->cTexture->texture = m_game->assets().getTexture("water");
    // const std::string & ani = "water_ani";
    // entity->cAnimation = std::make_shared<CAnimation> (m_game->assets().getAnimation(ani), true);

}

void Scene_Play::spawnBridge(const Vec2 pos, const Vec2 size, const int frame)
{
    auto entity = m_entities.addEntity("Bridge", (size_t)8);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, false);
    entity->cShape = std::make_shared<CShape>(pos, size, 255, 255, 255, 255);
    entity->cName = std::make_shared<CName>("Bridge");
    entity->cTexture = std::make_shared<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_game->assets().getTexture("bridge"));
    entity->cTexture->texture = m_game->assets().getTexture("bridge");
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
        p->cTransform->vel = { 0,0 };

        if (p->cInputs->up)
        {
            p->cTransform->vel.y = -1;
        }
        if (p->cInputs->down)
        {
            p->cTransform->vel.y = 1;
        }
        if (p->cInputs->left)
        {
            p->cTransform->vel.x = -1;
        }
        if (p->cInputs->right)
        {
            p->cTransform->vel.x = 1;
        }

        if (p->cInputs->shift)
        {
            p->cTransform->speed = 0.5*m_speed;
        }
        else if (p->cInputs->ctrl)
        {
            p->cTransform->speed = 2*m_speed;
        }
        else
        {
            p->cTransform->speed = m_speed;
        }
        
        
        p->cTransform->prevPos = p->cTransform->pos;

        if (!(p->cTransform->vel.isnull()) && p->cTransform->isMovable )
        {
            p->cTransform->pos += p->cTransform->vel.norm(p->cTransform->speed/m_game->framerate());
        }
    }
    for (auto d : m_entities.getEntities("Dragon"))
    {

        if (!(d->cTransform->vel.isnull()) && d->cTransform->isMovable )
        {
            d->cTransform->pos += d->cTransform->vel.norm(d->cTransform->speed/m_game->framerate());
        }
        if (d->cTransform->pos != d->cTransform->prevPos)
        {
            d->cTransform->isMovable = false;
        }
        d->cTransform->prevPos = d->cTransform->pos;
        
    }
}

void Scene_Play::sCollision() {
    for ( auto p : m_entities.getEntities("Player") )
    {
        for ( auto o : m_entities.getEntities("Outofbound") )
        {   
            if (m_physics.isCollided(p,o))
            {
                p->movePosition(m_physics.Overlap(p,o));
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
                p->movePosition(m_physics.Overlap(p,d));
                d->cAnimation = std::make_shared<CAnimation> (m_game->assets().getAnimation("waking_dragon"), false);
                d->cTransform->vel = Vec2{ -64*4, 0 };
                // d->cShape->setSize(Vec2(384, 192));
            }
        }
        for ( auto b : m_entities.getEntities("Border") )
        {   
            if (m_physics.isCollided(p,b))
            {
                p->movePosition(m_physics.Overlap(p,b));
            }
        }
        for ( auto k : m_entities.getEntities("Key") )
        {
            if (m_physics.isStandingIn(p,k))
            {
                k->kill();
                m_entities.getEntities("Player")[1]->cTransform->isMovable = true;
            }
        }
        for ( auto w : m_entities.getEntities("Water") )
        {
            if (m_physics.isStandingIn(p,w))
            {
                p->cTransform->isMovable = false;
            }
        }
        for ( auto l : m_entities.getEntities("Lava") )
        {
            if (m_physics.isStandingIn(p,l))
            {
                p->cTransform->isMovable = false;
            }
        }
    }

    for ( auto g : m_entities.getEntities("Goal") ) 
    {
        if ( m_physics.isStandingIn(m_entities.getEntities("Player")[0],g) )
        {
            g->setColor(0, 255, 0, 255);
            m_entities.getEntities("Player")[0]->cTransform->isMovable = false;
        }
        else if ( m_physics.isStandingIn(m_entities.getEntities("Player")[1],g) )
        {
            g->setColor(0, 255, 0, 255);
            m_entities.getEntities("Player")[1]->cTransform->isMovable = false;
        }
    }
}

void Scene_Play::sDoAction(const Action& action) {
    // if (action.type() == "START") {
    //     if (action.name() == "TOGGLE_TEXTURE") {
    //         m_drawTextures = !m_drawTextures; 
    //     }
    //     else if (action.name() == "TOGGLE_COLLISION") { 
    //         m_drawCollision = !m_drawCollision; 
    //     }
    //     else if (action.name() == "TOGGLE_GRID") { 
    //         m_drawDrawGrid = !m_drawDrawGrid; 
    //     }
    //     else if (action.name() == "PAUSE") { 
    //         setPaused(!m_pause);
    //     }
    //     else if (action.name() == "QUIT") { 
    //         onEnd();
    //     }
    //     else if (action.name() == "JUMP") {
    //         if (m_player->getComponent<CInput>().canJump) {
    //             m_player->getComponent<CInput>().up = true;
    //         }
    //     }
    //     else if (action.name() == "DOWN") {
    //         m_player->getComponent<CInput>().down = true;
    //     }
    //     else if (action.name() == "LEFT") {
    //         m_player->getComponent<CInput>().left = true;
    //     }
    //     else if (action.name() == "RIGHT") {
    //         m_player->getComponent<CInput>().right = true;
    //     }
    //     else if (action.name() == "SHOOT") {
    //         if (m_player->getComponent<CInput>().canShoot) {
    //             m_player->getComponent<CInput>().shoot = true;
    //         }
    //     }
    // }
    // else if (action.type() == "END") {
    //     if (action.name() == "JUMP") {
    //         m_player->getComponent<CInput>().up = false;
    //     }
    //     else if (action.name() == "DOWN") {
    //         m_player->getComponent<CInput>().down = false;
    //     }
    //     else if (action.name() == "LEFT") {
    //         m_player->getComponent<CInput>().left = false;
    //     }
    //     else if (action.name() == "RIGHT") {
    //         m_player->getComponent<CInput>().right = false;
    //     }
    //     else if (action.name() == "SHOOT") {
    //         m_player->getComponent<CInput>().shoot = false;
    //     }
    // }
}

void Scene_Play::sAnimation() {
    for ( auto e : m_entities.getEntities() )
        {
            if (e->cAnimation)
            {
                e->cAnimation->animation.update();
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

void Scene_Play::onEnd() {
    // todo: when the scene ends, change back to the MENU scene
    // use m_game->changeScene(correct params);
    // m_game->changeScene( "MENU", std::make_shared<Scene_Menu>(m_game));
    m_game->quit();
}

void Scene_Play::sRender() {
    for (auto e : m_entities.getEntities())
    {        
        if ( e->cTransform && e->cShape)
        {
            e->cShape->pos = e->cTransform->pos;
            SDL_Rect* shape;
            shape->x = e->cShape->pos.x;
            shape->y = e->cShape->pos.y;
            shape->w = e->cShape->size.x;
            shape->h = e->cShape->size.y;

            if (e->cTexture)
            {
                SDL_RenderCopy(m_game->renderer(), e->cTexture->texture, nullptr, shape);
            }
            if (e->cAnimation)
            {
                SDL_RenderCopy(m_game->renderer(), e->cAnimation->animation.getTexture(), e->cAnimation->animation.getPtrRect(), shape);
            }            
        }
    }
}

void Scene_Play::setPaused(bool pause) {
    m_pause = pause;
}
