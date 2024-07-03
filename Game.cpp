#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <thread>

#include <SDL2/SDL.h>

#include "RandomArray.h"
#include "Scene_Menu.cpp"
#include "Assets.cpp"
#include "Animation.cpp"
#include "src/include/Game.h"

std::chrono::system_clock::time_point a = std::chrono::system_clock::now();
std::chrono::system_clock::time_point b = std::chrono::system_clock::now();

Game::Game(const std::string & config)
{
    init(config);
}

void Game::init(const std::string & path)
{

    SDL_Init(SDL_INIT_EVERYTHING);
    m_window = SDL_CreateWindow("Heaven & Hell", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_RENDERER_ACCELERATED);
    if ( NULL == m_window )
    {
        std::cout << "Could not create window: " << SDL_GetError( ) << std::endl;
    }
    m_renderer = SDL_CreateRenderer( m_window, -1 , 0);
    SDL_SetRenderDrawBlendMode( m_renderer, SDL_BLENDMODE_BLEND );

    // m_assets.loadFromFile(path);
    // changeScene("MENU", std::make_shared<Scene_Menu>(this));

    // Load texture  
    m_assets.loadFromFile(path, m_renderer);
    SDL_Texture* level = m_assets.getTexture("level2");
    levelLoader(level);
}

// std::shared_ptr<Scene> Game::currentScene() {
//     return m_sceneMap[m_currentScene];
// }

std::vector<bool> Game::neighborCheck(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height) {
    std::vector<bool> neighbors(4, false); // {top, bottom, left, right}
    neighbors[0] = (y > 0 && pixelMatrix[y - 1][x] == pixel);           // top
    neighbors[1] = (x < width - 1 && pixelMatrix[y][x + 1] == pixel);   // right
    neighbors[2] = (y < height - 1 && pixelMatrix[y + 1][x] == pixel);  // bottom
    neighbors[3] = (x > 0 && pixelMatrix[y][x - 1] == pixel);           // left
    return neighbors;
}

int Game::getObstacleTextureIndex(const std::vector<bool>& neighbors) {
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

void Game::levelLoader(SDL_Texture* level_texture)
{

    const char* path = "assets/images/level2.png";
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (loadedSurface == nullptr) {
        std::cerr << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
    }

    // Create a texture from the surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, loadedSurface);
    if (texture == nullptr) {
        std::cerr << "Unable to create texture from " << path << "! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(loadedSurface);
    }

    // Lock the surface to access the pixels
    SDL_LockSurface(loadedSurface);
    Uint32* pixels = (Uint32*)loadedSurface->pixels;

    const int HEIGHT_PIX = 17;
    const int WIDTH_PIX = 30;
    
    // std::map<std::string, Vec2> Gridvec;
    // neighborCheck(pixels, x, y);

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

std::vector<std::vector<std::string>> Game::createPixelMatrix(Uint32* pixels, SDL_PixelFormat* format, int width, int height) {
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

bool Game::isRunning() {
    return m_running;
}

void Game::run()
{
    auto next_frame = std::chrono::steady_clock::now();

    while (m_running)
    {
        // FPS cap
        next_frame += std::chrono::milliseconds(1000 / m_framerate); // 60Hz
        // 

        m_entities.update();
        for ( auto e : m_entities.getEntities() )
        {
            if (e->cAnimation)
            {
                e->cAnimation->animation.update();
            }
            
        }
        sUserInput();
        sMovement();
        sCollisions();
        sRender();
        m_currentFrame++;

        std::this_thread::sleep_until(next_frame);
    }
    SDL_DestroyWindow( m_window );
    SDL_Quit();
}   

// void Game::changeScene(
//     const std::string& sceneName,
//     std::shared_ptr<Scene> scene,
//     bool endCurrentScene
// ) {
//     m_currentScene = sceneName;
//     m_sceneMap[sceneName] = scene;
// }

void Game::quit() {
    m_running = false;
    // m_window.close();
}

// void Game::update() {
//     currentScene()->update();
// }

SDL_Renderer* Game::renderer(){
    return m_renderer;
}

SDL_Window* Game::window(){
    return m_window;
}

int Game::getWidth()
{
    return WIDTH;
}

int Game::getHeight()
{
    return HEIGHT;
}


// -------------------------------------------------------------------------

void Game::spawnPlayer(const Vec2 pos, const std::string name, bool movable)
{
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
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0,0}, Vec2 {210, 270}, m_assets.getTexture(tex));
    entity->cTexture->setPtrTexture(m_assets.getTexture(tex));
    m_player = entity;
}

void Game::spawnObstacle(const Vec2 pos, const Vec2 size, bool movable, const int frame)
{
    auto entity = m_entities.addEntity("Obstacle", (size_t)9);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 100, 100, 0, 255);
    entity->cName = std::make_shared<CName>("Obstacle");
    // std::cout << frame << " " << (float)(frame%4) << " " << (float)(int)(frame/4) << std::endl;
    entity->cTexture = std::make_shared<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_assets.getTexture("rock_wall"));
    entity->cTexture->setPtrTexture(m_assets.getTexture("rock_wall"));
}
void Game::spawnDragon(const Vec2 pos, const Vec2 size, bool movable, const std::string &ani) 
{
    auto entity = m_entities.addEntity("Dragon", (size_t)3);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 100, 100, 0, 255);
    entity->cName = std::make_shared<CName>("Dragon");
    entity->cAnimation = std::make_shared<CAnimation> (m_assets.getAnimation(ani), true);


    // auto coin = m_entities.addEntity("coinspin", (size_t)10);
    // coin->addComponent<CAnimation>(
    //     m_game->assets().getAnimation("CoinSpin"),
    //     false
    // );
    // coin->addComponent<CTransform>(
    //     Vec2(
    //         tile->getComponent<CTransform>().pos.x,
    //         tile->getComponent<CTransform>().pos.y - m_gridSize.y
    //         ),
    //     Vec2(0, 0),
    //     tile->getComponent<CTransform>().scale,
    //     0
    // );
    // coin->addComponent<CLifespan>(30, m_currentFrame);
}


void Game::spawnWorldBorder(const Vec2 pos, const Vec2 size, bool movable)
{
    auto entity = m_entities.addEntity("Border", (size_t)9);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 100, 100, 0, 255);
    entity->cName = std::make_shared<CName>("Border");
}

void Game::spawnOutofboundBorder(const Vec2 pos, const Vec2 size, bool movable)
{
    auto entity = m_entities.addEntity("Outofbound", (size_t)9);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 100, 100, 0, 255);
    entity->cName = std::make_shared<CName>("Outofbound");
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0, 0}, Vec2 {200,64}, m_assets.getTexture("m_texture_outofbound"));
    entity->cTexture->setPtrTexture(m_assets.getTexture("m_texture_outofbound"));
}

void Game::spawnBackground(const Vec2 pos, const Vec2 size, bool movable)
{
    auto entity = m_entities.addEntity("Background", (size_t)10);
    entity->cTransform = std::make_shared<CTransform>(pos, Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 255, 255, 255, 255);
    entity->cName = std::make_shared<CName>("Background ");

    std::vector<int> ranArray = generateRandomArray(1, m_entities.getTotalEntities(), 0, 12);
    if (ranArray[0] == 0)
        entity->cTexture = std::make_shared<CTexture>(Vec2 {0, 0}, Vec2 {64, 64}, m_assets.getTexture("m_texture_background"));
    else
        entity->cTexture = std::make_shared<CTexture>(Vec2 {0, 0}, Vec2 {2, 2}, m_assets.getTexture("m_texture_background"));
}

void Game::spawnGoal(const Vec2 pos, const Vec2 size, bool movable)
{
    auto entity = m_entities.addEntity("Goal", (size_t)4);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 20, 200, 20, 10);
    entity->cName = std::make_shared<CName>("Goal");
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0,0}, Vec2 {64,64}, m_assets.getTexture("m_texture_goal"));
    entity->cTexture->setPtrTexture(m_assets.getTexture("m_texture_goal"));
}

void Game::spawnKey(const Vec2 pos, const Vec2 size, const std::string playerToUnlock, bool movable)
{
    auto entity = m_entities.addEntity("Key", (size_t)4);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 120, 120, 20, 200);
    entity->cName = std::make_shared<CName>("Key");
    entity->cKey = std::make_shared<CKey>(playerToUnlock);
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0,0}, Vec2 {225, 225}, m_assets.getTexture("m_texture_key"));
    entity->cTexture->setPtrTexture(m_assets.getTexture("m_texture_key"));
}

void Game::spawnLava(const Vec2 pos, const Vec2 size)
{
    auto entity = m_entities.addEntity("Lava", (size_t)8);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, false);
    entity->cShape = std::make_shared<CShape>(pos, size, 255, 255, 255, 255);
    entity->cName = std::make_shared<CName>("Lava");
    // entity->cTexture = std::make_shared<CTexture>(Vec2 {0,0}, Vec2 {64, 64}, m_assets.getTexture("lava"));
    // entity->cTexture->setPtrTexture(m_assets.getTexture("lava"));
    const std::string & ani = "lava_ani";
    entity->cAnimation = std::make_shared<CAnimation> (m_assets.getAnimation(ani), true);
}

void Game::spawnWater(const Vec2 pos, const Vec2 size, const int frame)
{
    auto entity = m_entities.addEntity("Water", (size_t)8);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, false);
    entity->cShape = std::make_shared<CShape>(pos, size, 255, 255, 255, 255);
    entity->cName = std::make_shared<CName>("Water");
    entity->cTexture = std::make_shared<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_assets.getTexture("water"));
    // entity->cTexture->setPtrTexture(m_assets.getTexture("water"));
    // const std::string & ani = "water_ani";
    // entity->cAnimation = std::make_shared<CAnimation> (m_assets.getAnimation(ani), true);

}

void Game::spawnBridge(const Vec2 pos, const Vec2 size, const int frame)
{
    auto entity = m_entities.addEntity("Bridge", (size_t)8);
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, false);
    entity->cShape = std::make_shared<CShape>(pos, size, 255, 255, 255, 255);
    entity->cName = std::make_shared<CName>("Bridge");
    entity->cTexture = std::make_shared<CTexture>(Vec2 {(float)(frame%4)*32, (float)(int)(frame/4)*32}, Vec2 {32, 32}, m_assets.getTexture("bridge"));
    entity->cTexture->setPtrTexture(m_assets.getTexture("bridge"));
}



void Game::sUserInput()
{
    SDL_Event event;
    // const Uint8 *currentKeyStates = SDL_GetKeyboardState( NULL );
    if ( SDL_PollEvent( &event ) )
    {
        for (auto p : m_entities.getEntities("Player"))
        {
            if (SDL_QUIT == event.type)
            {
                m_running = false;
            }
            if(event.type == SDL_KEYDOWN)
            {   
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    m_running = false;
                }

                if (event.key.keysym.sym == SDLK_w)
                {
                    p->cInputs->up = true;
                }
                if (event.key.keysym.sym == SDLK_s)
                {
                    p->cInputs->down = true;
                }
                if (event.key.keysym.sym == SDLK_a)
                {
                    p->cInputs->left = true;
                }
                if (event.key.keysym.sym == SDLK_d)
                {
                    p->cInputs->right = true;
                }
                if (event.key.keysym.sym == SDLK_LSHIFT)
                {
                    p->cInputs->shift = true;
                }
                if (event.key.keysym.sym == SDLK_LCTRL)
                {
                    p->cInputs->ctrl = true;
                }
            }
            if(event.type == SDL_KEYUP)
            {
                if (event.key.keysym.sym == SDLK_w)
                {
                    p->cInputs->up = false;
                }
                if (event.key.keysym.sym == SDLK_s)
                {
                    p->cInputs->down = false;
                }
                if (event.key.keysym.sym == SDLK_a)
                {
                    p->cInputs->left = false;
                }
                if (event.key.keysym.sym == SDLK_d)
                {
                    p->cInputs->right = false;  
                }
                if ( event.key.keysym.sym == SDLK_LSHIFT )
                {
                    p->cInputs->shift = false;  
                }
                if ( event.key.keysym.sym == SDLK_LCTRL )
                {
                    p->cInputs->ctrl = false;  
                }
            }
        }
    }
}

void Game::sMovement()
{
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
            p->cTransform->pos += p->cTransform->vel.norm(p->cTransform->speed/m_framerate);
        }
    }
    for (auto d : m_entities.getEntities("Dragon"))
    {

        if (!(d->cTransform->vel.isnull()) && d->cTransform->isMovable )
        {
            d->cTransform->pos += d->cTransform->vel.norm(d->cTransform->speed/m_framerate);
        }
        if (d->cTransform->pos != d->cTransform->prevPos)
        {
            d->cTransform->isMovable = false;
        }
        d->cTransform->prevPos = d->cTransform->pos;
        
    }
}

void Game::sRender()
{
    SDL_RenderClear( m_renderer );
        
    for (auto e : m_entities.getEntities())
    {
        // if (e->cAnimation->animation.hasEnded() && e->tag() == "Dragon")
        // {
        //     e->cAnimation->animation.setTexture(m_assets.getTexture("sleeping_dragon"));
        // }
        
        if ( e->cTransform && e->cShape)
        {
            e->cShape->setPosition( e->cTransform->pos );
            if (e->cTexture)
            {
                SDL_RenderCopy(m_renderer, e->cTexture->getPtrTexture(), e->cTexture->getPtrRect(), e->cShape->getPtrRect());
            }
            if (e->cAnimation)
            {
                SDL_RenderCopy(m_renderer, e->cAnimation->animation.getTexture(), e->cAnimation->animation.getPtrRect(), e->cShape->getPtrRect());
            }            
        }
    }
    SDL_RenderPresent( m_renderer );
}

void Game::sCollisions()
{ 
    for ( auto p : m_entities.getEntities("Player") )
    {
        for ( auto o : m_entities.getEntities("Outofbound") )
        {   
            if (isCollided(p,o))
            {
                p->movePosition(Overlap(p,o));
            }
        }
        for ( auto o : m_entities.getEntities("Obstacle") )
        {   
            if (isCollided(p,o))
            {
                p->movePosition(Overlap(p,o));
            }
        }
        for ( auto d : m_entities.getEntities("Dragon") )
        {   
            if (isCollided(p,d))
            {
                p->movePosition(Overlap(p,d));
                d->cAnimation = std::make_shared<CAnimation> (m_assets.getAnimation("waking_dragon"), false);
                d->cTransform->vel = Vec2{ -64*4, 0 };
                d->cShape->setSize(Vec2(384, 192));
            }
        }
        for ( auto b : m_entities.getEntities("Border") )
        {   
            if (isCollided(p,b))
            {
                p->movePosition(Overlap(p,b));
            }
        }
        for ( auto k : m_entities.getEntities("Key") )
        {
            if (isStandingIn(p,k))
            {
                k->kill();
                m_entities.getEntities("Player")[1]->cTransform->isMovable = true;
            }
        }
        for ( auto w : m_entities.getEntities("Water") )
        {
            if (isStandingIn(p,w))
            {
                p->cTransform->isMovable = false;
            }
        }
        for ( auto l : m_entities.getEntities("Lava") )
        {
            if (isStandingIn(p,l))
            {
                p->cTransform->isMovable = false;
            }
        }
    }

    for ( auto g : m_entities.getEntities("Goal") ) 
    {
        if ( isStandingIn(m_entities.getEntities("Player")[0],g) )
        {
            g->setColor(0, 255, 0, 255);
            m_entities.getEntities("Player")[0]->cTransform->isMovable = false;
        }
        else if ( isStandingIn(m_entities.getEntities("Player")[1],g) )
        {
            g->setColor(0, 255, 0, 255);
            m_entities.getEntities("Player")[1]->cTransform->isMovable = false;
        }
    }
}

bool Game::isCollided(std::shared_ptr<Entity> entity1, std::shared_ptr<Entity> entity2)
{
    if (entity1->getId() == entity2->getId())
    {
        return false;
    }

    bool x_overlap =    (entity1->cTransform->pos.x + entity1->cShape->size.x > entity2->cTransform->pos.x) && 
                        (entity2->cTransform->pos.x + entity2->cShape->size.x > entity1->cTransform->pos.x);
    bool y_overlap =    (entity1->cTransform->pos.y + entity1->cShape->size.y > entity2->cTransform->pos.y) && 
                        (entity2->cTransform->pos.y + entity2->cShape->size.y > entity1->cTransform->pos.y);

    return (x_overlap && y_overlap);
}

bool Game::isStandingIn(std::shared_ptr<Entity> entity1, std::shared_ptr<Entity> entity2)
{
    if (entity1->getId() == entity2->getId())
    {
        return false;
    }

    bool x_center_overlap = (entity1->cTransform->pos.x + entity1->cShape->size.x/2 > entity2->cTransform->pos.x) && 
                            (entity2->cTransform->pos.x + entity2->cShape->size.x/2 > entity1->cTransform->pos.x);
    bool y_overlap =    (entity1->cTransform->pos.y + entity1->cShape->size.y <= entity2->cTransform->pos.y + entity2->cShape->size.y) &&
                        (entity1->cTransform->pos.y + entity1->cShape->size.y > entity2->cTransform->pos.y);

    return (x_center_overlap && y_overlap);
}

Vec2 Game::Overlap(std::shared_ptr<Entity> p, std::shared_ptr<Entity> o)
{
    Vec2 delta          =   ( (p->cTransform->pos       + p->cShape->size/2) - (o->cTransform->pos      + o->cShape->size/2) ).abs_elem();
    Vec2 prevDelta      =   ( (p->cTransform->prevPos   + p->cShape->size/2) - (o->cTransform->prevPos  + o->cShape->size/2) ).abs_elem();
    Vec2 overlap        =   p->cShape->size/2 + o->cShape->size/2 - delta;
    Vec2 prevOverlap    =   p->cShape->size/2 + o->cShape->size/2 - prevDelta;

    Vec2 move = { 0, 0 };
    if (prevOverlap.y > 0)
    {
        if ((p->cTransform->pos.x + p->cShape->size.x/2) > (o->cTransform->pos.x + o->cShape->size.x/2))
        {
            move += Vec2 { overlap.x, 0 };
        }
        if ((p->cTransform->pos.x + p->cShape->size.x/2) < (o->cTransform->pos.x + o->cShape->size.x/2))
        {
            move -= Vec2 { overlap.x, 0 };
        }
    }

    if (prevOverlap.x > 0)
    {
        if ((p->cTransform->pos.y + p->cShape->size.y/2) > (o->cTransform->pos.y + o->cShape->size.y/2))
        {
            move +=  Vec2 { 0, overlap.y };
        }
        if ((p->cTransform->pos.y + p->cShape->size.y/2) < (o->cTransform->pos.y + o->cShape->size.y/2))
        {
            move -=  Vec2 { 0, overlap.y };
        }
    }

    return move;
}

void Game::setPaused(bool paused)
{
    m_paused = paused;
}