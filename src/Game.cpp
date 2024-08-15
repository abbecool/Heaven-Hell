#include <SDL2/SDL.h>

#include <iostream>
#include <fstream>
#include <algorithm>
// #include <SDL_ttf.h>
#include <chrono>
#include <ctime>
#include <thread>


// #include "Scene_Menu.cpp"
#include "Game.h"
// #include "Scene.cpp"
#include "Assets.h"
#include "Scene_Play.h"
// #include "Action.cpp"
// #include "Animation.cpp"

std::chrono::system_clock::time_point a = std::chrono::system_clock::now();
std::chrono::system_clock::time_point b = std::chrono::system_clock::now();

Game::Game(const std::string & config)
{
    init(config);
}

void Game::init(const std::string & path){

    SDL_Init(SDL_INIT_EVERYTHING);
    m_window = SDL_CreateWindow("Heaven & Hell", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_RENDERER_ACCELERATED);
    if ( NULL == m_window )
    {
        std::cout << "Could not create window: " << SDL_GetError( ) << std::endl;
    }
    m_renderer = SDL_CreateRenderer( m_window, -1 , SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode( m_renderer, SDL_BLENDMODE_BLEND );

    m_assets.loadFromFile(path, m_renderer);
    changeScene("PLAY", std::make_shared<Scene_Play>(this, "assets/images/level0.png"));
}

std::shared_ptr<Scene> Game::currentScene() {
    return m_sceneMap[m_currentScene];
}

void Game::changeScene(
    const std::string& sceneName,
    std::shared_ptr<Scene> scene,
    bool endCurrentScene
) {
    m_currentScene = sceneName;
    m_sceneMap[sceneName] = scene;
}

bool Game::isRunning() {
    return m_running;
}

int Game::framerate(){
    return m_framerate;
}

void Game::run()
{
    auto next_frame = std::chrono::steady_clock::now();

    while (isRunning())
    {
        // FPS cap
        next_frame += std::chrono::milliseconds(1000 / m_framerate); // 60Hz
        // 

        SDL_RenderClear( m_renderer );

        update();
        sUserInput();
        SDL_RenderPresent( m_renderer );
        m_currentFrame++;

        std::this_thread::sleep_until(next_frame);
    }
    SDL_DestroyWindow( m_window );
    SDL_Quit();
}   

void Game::quit() {
    m_running = false;
    // m_window.close();
}

void Game::update() {
    currentScene()->update();
}

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

void Game::sUserInput()
{
    SDL_Event event;
    while ( SDL_PollEvent( &event ) )
    {
        if (SDL_QUIT == event.type)
            {
                quit();
            }
        if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
        { 
            if (currentScene()->getActionMap().find(event.key.keysym.sym)
                == currentScene()->getActionMap().end()) {
                continue;
            }
            // determine start or end action by whether it was key press or release
            const std::string actionType = (event.type == SDL_KEYDOWN) ? "START" : "END";

            // std::cout << actionType << std::endl;
            // look up the action and send the action to the scene
            currentScene()->doAction(Action(currentScene()->getActionMap().at(event.key.keysym.sym), actionType));
        }
        // for (auto p : m_entities.getEntities("Player"))
        // {
        //     if (SDL_QUIT == event.type)
        //     {
        //         m_running = false;
        //     }
        //     if(event.type == SDL_KEYDOWN)
        //     {   
        //         if (event.key.keysym.sym == SDLK_ESCAPE)
        //         {
        //             m_running = false;
        //         }

        //         if (event.key.keysym.sym == SDLK_w)
        //         {
        //             p->cInputs->up = true;
        //         }
        //         if (event.key.keysym.sym == SDLK_s)
        //         {
        //             p->cInputs->down = true;
        //         }
        //         if (event.key.keysym.sym == SDLK_a)
        //         {
        //             p->cInputs->left = true;
        //         }
        //         if (event.key.keysym.sym == SDLK_d)
        //         {
        //             p->cInputs->right = true;
        //         }
        //         if (event.key.keysym.sym == SDLK_LSHIFT)
        //         {
        //             p->cInputs->shift = true;
        //         }
        //         if (event.key.keysym.sym == SDLK_LCTRL)
        //         {
        //             p->cInputs->ctrl = true;
        //         }
        //     }
        //     if(event.type == SDL_KEYUP)
        //     {
        //         if (event.key.keysym.sym == SDLK_w)
        //         {
        //             p->cInputs->up = false;
        //         }
        //         if (event.key.keysym.sym == SDLK_s)
        //         {
        //             p->cInputs->down = false;
        //         }
        //         if (event.key.keysym.sym == SDLK_a)
        //         {
        //             p->cInputs->left = false;
        //         }
        //         if (event.key.keysym.sym == SDLK_d)
        //         {
        //             p->cInputs->right = false;  
        //         }
        //         if ( event.key.keysym.sym == SDLK_LSHIFT )
        //         {
        //             p->cInputs->shift = false;  
        //         }
        //         if ( event.key.keysym.sym == SDLK_LCTRL )
        //         {
        //             p->cInputs->ctrl = false;  
        //         }
        //     }
        // }
    }
}

// void Game::sRender()
// {
        
//     for (auto e : m_entities.getEntities())
//     {
//         // if (e->cAnimation->animation.hasEnded() && e->tag() == "Dragon")
//         // {
//         //     e->cAnimation->animation.setTexture(m_assets.getTexture("sleeping_dragon"));
//         // }
        
//         if ( e->cTransform && e->cShape)
//         {
//             e->cShape->setPosition( e->cTransform->pos );
//             if (e->cTexture)
//             {
//                 SDL_RenderCopy(m_renderer, e->cTexture->getPtrTexture(), e->cTexture->getPtrRect(), e->cShape->getPtrRect());
//             }
//             if (e->cAnimation)
//             {
//                 SDL_RenderCopy(m_renderer, e->cAnimation->animation.getTexture(), e->cAnimation->animation.getPtrRect(), e->cShape->getPtrRect());
//             }            
//         }
//     }
// }

// void Game::sCollisions()
// { 
//     for ( auto p : m_entities.getEntities("Player") )
//     {
//         for ( auto o : m_entities.getEntities("Outofbound") )
//         {   
//             if (isCollided(p,o))
//             {
//                 p->movePosition(Overlap(p,o));
//             }
//         }
//         for ( auto o : m_entities.getEntities("Obstacle") )
//         {   
//             if (isCollided(p,o))
//             {
//                 p->movePosition(Overlap(p,o));
//             }
//         }
//         for ( auto d : m_entities.getEntities("Dragon") )
//         {   
//             if (isCollided(p,d))
//             {
//                 p->movePosition(Overlap(p,d));
//                 d->cAnimation = std::make_shared<CAnimation> (m_assets.getAnimation("waking_dragon"), false);
//                 d->cTransform->vel = Vec2{ -64*4, 0 };
//                 // d->cShape->setSize(Vec2(384, 192));
//             }
//         }
//         for ( auto b : m_entities.getEntities("Border") )
//         {   
//             if (isCollided(p,b))
//             {
//                 p->movePosition(Overlap(p,b));
//             }
//         }
//         for ( auto k : m_entities.getEntities("Key") )
//         {
//             if (isStandingIn(p,k))
//             {
//                 k->kill();
//                 m_entities.getEntities("Player")[1]->cTransform->isMovable = true;
//             }
//         }
//         for ( auto w : m_entities.getEntities("Water") )
//         {
//             if (isStandingIn(p,w))
//             {
//                 p->cTransform->isMovable = false;
//             }
//         }
//         for ( auto l : m_entities.getEntities("Lava") )
//         {
//             if (isStandingIn(p,l))
//             {
//                 p->cTransform->isMovable = false;
//             }
//         }
//     }

//     for ( auto g : m_entities.getEntities("Goal") ) 
//     {
//         if ( isStandingIn(m_entities.getEntities("Player")[0],g) )
//         {
//             g->setColor(0, 255, 0, 255);
//             m_entities.getEntities("Player")[0]->cTransform->isMovable = false;
//         }
//         else if ( isStandingIn(m_entities.getEntities("Player")[1],g) )
//         {
//             g->setColor(0, 255, 0, 255);
//             m_entities.getEntities("Player")[1]->cTransform->isMovable = false;
//         }
//     }
// }

// bool Game::isCollided(std::shared_ptr<Entity> entity1, std::shared_ptr<Entity> entity2)
// {
//     if (entity1->id() == entity2->id())
//     {
//         return false;
//     }

//     bool x_overlap =    (entity1->cTransform->pos.x + entity1->cShape->size.x > entity2->cTransform->pos.x) && 
//                         (entity2->cTransform->pos.x + entity2->cShape->size.x > entity1->cTransform->pos.x);
//     bool y_overlap =    (entity1->cTransform->pos.y + entity1->cShape->size.y > entity2->cTransform->pos.y) && 
//                         (entity2->cTransform->pos.y + entity2->cShape->size.y > entity1->cTransform->pos.y);

//     return (x_overlap && y_overlap);
// }

// bool Game::isStandingIn(std::shared_ptr<Entity> entity1, std::shared_ptr<Entity> entity2)
// {
//     if (entity1->id() == entity2->id())
//     {
//         return false;
//     }

//     bool x_center_overlap = (entity1->cTransform->pos.x + entity1->cShape->size.x/2 > entity2->cTransform->pos.x) && 
//                             (entity2->cTransform->pos.x + entity2->cShape->size.x/2 > entity1->cTransform->pos.x);
//     bool y_overlap =    (entity1->cTransform->pos.y + entity1->cShape->size.y <= entity2->cTransform->pos.y + entity2->cShape->size.y) &&
//                         (entity1->cTransform->pos.y + entity1->cShape->size.y > entity2->cTransform->pos.y);

//     return (x_center_overlap && y_overlap);
// }

// Vec2 Game::Overlap(std::shared_ptr<Entity> p, std::shared_ptr<Entity> o)
// {
//     Vec2 delta          =   ( (p->cTransform->pos       + p->cShape->size/2) - (o->cTransform->pos      + o->cShape->size/2) ).abs_elem();
//     Vec2 prevDelta      =   ( (p->cTransform->prevPos   + p->cShape->size/2) - (o->cTransform->prevPos  + o->cShape->size/2) ).abs_elem();
//     Vec2 overlap        =   p->cShape->size/2 + o->cShape->size/2 - delta;
//     Vec2 prevOverlap    =   p->cShape->size/2 + o->cShape->size/2 - prevDelta;

//     Vec2 move = { 0, 0 };
//     if (prevOverlap.y > 0)
//     {
//         if ((p->cTransform->pos.x + p->cShape->size.x/2) > (o->cTransform->pos.x + o->cShape->size.x/2))
//         {
//             move += Vec2 { overlap.x, 0 };
//         }
//         if ((p->cTransform->pos.x + p->cShape->size.x/2) < (o->cTransform->pos.x + o->cShape->size.x/2))
//         {
//             move -= Vec2 { overlap.x, 0 };
//         }
//     }

//     if (prevOverlap.x > 0)
//     {
//         if ((p->cTransform->pos.y + p->cShape->size.y/2) > (o->cTransform->pos.y + o->cShape->size.y/2))
//         {
//             move +=  Vec2 { 0, overlap.y };
//         }
//         if ((p->cTransform->pos.y + p->cShape->size.y/2) < (o->cTransform->pos.y + o->cShape->size.y/2))
//         {
//             move -=  Vec2 { 0, overlap.y };
//         }
//     }

//     return move;
// }

const Assets& Game::assets() const{
    return m_assets;
}

void Game::setPaused(bool paused)
{
    m_paused = paused;
}