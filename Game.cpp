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
    // m_assets.loadLevel();

    // Background
    spawnBackground(Vec2 {0,0}, Vec2 {1920,1080}, false);
    // spawnBackground(Vec2 {0,550}, Vec2 {1920,500}, false);
    // Goals
    spawnGoal(Vec2 {1920-75,200}, Vec2 {75,100}, false);
    spawnGoal(Vec2 {1920-75,200+550}, Vec2 {75,100}, false);
    // Players
    spawnPlayer(Vec2{0,250}, "God", true);
    spawnPlayer(Vec2{0,800}, "Devil", false);
    // Outer bound walls
    spawnWorldBorder(Vec2 {0,-5}, Vec2 {1920, 5}, false);
    spawnWorldBorder(Vec2 {-5,0}, Vec2 {5, 1080}, false);
    spawnWorldBorder(Vec2 {1920,0}, Vec2 {5,1080}, false);
    spawnWorldBorder(Vec2 {0,1080}, Vec2 {1920,5}, false);
    // World divider
    spawnOutofboundBorder(Vec2 {0,500}, Vec2 {200,64}, false);
    spawnOutofboundBorder(Vec2 {200,500}, Vec2 {200,64}, false);
    spawnOutofboundBorder(Vec2 {400,500}, Vec2 {200,64}, false);
    spawnOutofboundBorder(Vec2 {600,500}, Vec2 {200,64}, false);
    spawnOutofboundBorder(Vec2 {800,500}, Vec2 {200,64}, false);
    spawnOutofboundBorder(Vec2 {1000,500}, Vec2 {200,64}, false);
    spawnOutofboundBorder(Vec2 {1200,500}, Vec2 {200,64}, false);
    spawnOutofboundBorder(Vec2 {1400,500}, Vec2 {200,64}, false);
    spawnOutofboundBorder(Vec2 {1600,500}, Vec2 {200,64}, false);
    spawnOutofboundBorder(Vec2 {1800,500}, Vec2 {200,64}, false);
    // Standard obsticles
    spawnObstacle(Vec2 {300,0}, Vec2 {64,64}, false);
    spawnObstacle(Vec2 {300,300}, Vec2 {64,64}, false);
    spawnObstacle(Vec2 {400,0+550}, Vec2 {64,64}, false);
    spawnObstacle(Vec2 {400,400+550}, Vec2 {64,64}, false);
    const std::string & dragon = "snoring_dragon";
    spawnDragon(Vec2 {800,100}, Vec2 {256,256}, false, dragon);
    spawnDragon(Vec2 {900,650}, Vec2 {512,256}, false, dragon);
    // Unlock keys for players
    spawnKey(Vec2 {200,400}, Vec2 {64,64}, "Devil", false);
}

// std::shared_ptr<Scene> Game::currentScene() {
//     return m_sceneMap[m_currentScene];
// }

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
        for ( auto d : m_entities.getEntities("Dragon") )
        {
            d->cAnimation->animation.update();
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

    auto entity = m_entities.addEntity("Player");
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, Vec2{64, 64}, 255, 0, 0, 255);
    entity->cInputs = std::make_shared<CInputs>();
    entity->cName = std::make_shared<CName>(name);
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0,0}, Vec2 {210, 270}, m_assets.getTexture(tex));
    entity->cTexture->setPtrTexture(m_assets.getTexture(tex));
    m_player = entity;
}

void Game::spawnObstacle(const Vec2 pos, const Vec2 size, bool movable)
{
    auto entity = m_entities.addEntity("Obstacle");
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 100, 100, 0, 255);
    entity->cName = std::make_shared<CName>("Obstacle");
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0,0}, Vec2 {32, 32}, m_assets.getTexture("rock_wall"));
    entity->cTexture->setPtrTexture(m_assets.getTexture("rock_wall"));
}
void Game::spawnDragon(const Vec2 pos, const Vec2 size, bool movable, const std::string &ani) 
{
    auto entity = m_entities.addEntity("Dragon");
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 100, 100, 0, 255);
    entity->cName = std::make_shared<CName>("Dragon");
    entity->cAnimation = std::make_shared<CAnimation> (m_assets.getAnimation(ani), true);


    // auto coin = m_entities.addEntity("coinspin");
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
    auto entity = m_entities.addEntity("Border");
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 100, 100, 0, 255);
    entity->cName = std::make_shared<CName>("Border");
}

void Game::spawnOutofboundBorder(const Vec2 pos, const Vec2 size, bool movable)
{
    auto entity = m_entities.addEntity("Outofbound");
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 100, 100, 0, 255);
    entity->cName = std::make_shared<CName>("Outofbound");
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0, 0}, Vec2 {200,64}, m_assets.getTexture("m_texture_outofbound"));
    entity->cTexture->setPtrTexture(m_assets.getTexture("m_texture_outofbound"));
}

void Game::spawnBackground(const Vec2 pos, const Vec2 size, bool movable)
{
    int tex_size = 64;
    int num_x = size.x/tex_size+1;
    int num_y = size.y/tex_size+1;

    std::vector<int> ranArray = generateRandomArray(num_x*num_y, 1337, 0, 15);

    for (int i_x = 0; i_x < num_x; i_x++)       
    {
        for (int i_y = 0; i_y < num_y; i_y++)
        {
            auto entity = m_entities.addEntity("Background"+std::to_string(i_x)+std::to_string(i_y));
            entity->cTransform = std::make_shared<CTransform>(Vec2{float(i_x*tex_size), float(i_y*tex_size)}+pos, Vec2 {0, 0}, movable);
            entity->cShape = std::make_shared<CShape>(Vec2{float(i_x*tex_size), float(i_y*tex_size)}+pos, Vec2 {float(tex_size), float(tex_size)}, 255, 255, 255, 255);
            entity->cName = std::make_shared<CName>("Background " + i_x+i_y);
            if (ranArray[num_y*i_x+i_y] == 1)
            {
                entity->cTexture = std::make_shared<CTexture>(Vec2 {0, 0}, Vec2 {64, 64}, m_assets.getTexture("m_texture_background"));
            }
            else
            {
                entity->cTexture = std::make_shared<CTexture>(Vec2 {0, 0}, Vec2 {4, 4}, m_assets.getTexture("m_texture_background"));
            }
            entity->cTexture->setPtrTexture(m_assets.getTexture("m_texture_background"));
        }   
    }
}

void Game::spawnGoal(const Vec2 pos, const Vec2 size, bool movable)
{
    auto entity = m_entities.addEntity("Goal");
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 20, 200, 20, 10);
    entity->cName = std::make_shared<CName>("Goal");
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0,0}, Vec2 {64,64}, m_assets.getTexture("m_texture_goal"));
    entity->cTexture->setPtrTexture(m_assets.getTexture("m_texture_goal"));
}

void Game::spawnKey(const Vec2 pos, const Vec2 size, const std::string playerToUnlock, bool movable)
{
    auto entity = m_entities.addEntity("Key");
    entity->cTransform = std::make_shared<CTransform>(pos,Vec2 {0, 0}, movable);
    entity->cShape = std::make_shared<CShape>(pos, size, 120, 120, 20, 200);
    entity->cName = std::make_shared<CName>("Key");
    entity->cKey = std::make_shared<CKey>(playerToUnlock);
    entity->cTexture = std::make_shared<CTexture>(Vec2 {0,0}, Vec2 {225, 225}, m_assets.getTexture("m_texture_key"));
    entity->cTexture->setPtrTexture(m_assets.getTexture("m_texture_key"));
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
}

void Game::sRender()
{
    SDL_RenderClear( m_renderer );
        
    for (auto e : m_entities.getEntities())
    {
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
                // d->cTransform->isMovable = true;
                d->cTransform->pos = Vec2(800-128, 100);
                d->cShape->setSize(Vec2(512, 256));
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
            if (isCollided(p,k))
            {
                k->kill();
                m_entities.getEntities("Player")[1]->cTransform->isMovable = true;
            }
        }
    }

    for ( auto g : m_entities.getEntities("Goal") ) 
    {
        if ( isCollided(m_entities.getEntities("Player")[0],g) )
        {
            g->setColor(0, 255, 0, 255);
            m_entities.getEntities("Player")[0]->cTransform->isMovable = false;
        }
        else if ( isCollided(m_entities.getEntities("Player")[1],g) )
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

    bool x_overlap = (entity1->cTransform->pos.x + entity1->cShape->size.x > entity2->cTransform->pos.x) && (entity2->cTransform->pos.x + entity2->cShape->size.x > entity1->cTransform->pos.x);
    bool y_overlap = (entity1->cTransform->pos.y + entity1->cShape->size.y > entity2->cTransform->pos.y) && (entity2->cTransform->pos.y + entity2->cShape->size.y > entity1->cTransform->pos.y);

    return (x_overlap && y_overlap);
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