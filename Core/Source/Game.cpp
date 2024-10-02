#include <SDL2/SDL.h>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <thread>


#include "Game.h"
#include "Assets.h"
#include "Scene_Menu.h"
std::chrono::system_clock::time_point a = std::chrono::system_clock::now();
std::chrono::system_clock::time_point b = std::chrono::system_clock::now();

Game::Game(const std::string & pathImages, const std::string & pathText)
{
    init(pathImages, pathText);
}

void Game::init(const std::string & pathImages, const std::string & pathText){

    SDL_Init(SDL_INIT_EVERYTHING);
    m_window = SDL_CreateWindow("Heaven & Hell", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetWindowPosition(m_window, 0, 0);
    if ( NULL == m_window )
    {
        std::cout << "Could not create window: " << SDL_GetError( ) << std::endl;
    }
    m_renderer = SDL_CreateRenderer( m_window, -1 , SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode( m_renderer, SDL_BLENDMODE_BLEND );
    TTF_Init();

    m_assets.loadFromFile(pathImages, pathText, m_renderer);
    changeScene("Menu", std::make_shared<Scene_Menu>(this));
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
    std::chrono::steady_clock::time_point current_frame = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point next_frame;
    std::chrono::steady_clock::time_point last_fps_update = current_frame;

    int frame_count = 0;
    double accumulated_frame_time = 0.0;

    while (isRunning())
    {
        // FPS cap
        current_frame = std::chrono::steady_clock::now();
        next_frame = current_frame + std::chrono::milliseconds(1000 / m_framerate); // 60Hz
        // 

        SDL_RenderClear( m_renderer );

        update();
        sUserInput();
        SDL_RenderPresent( m_renderer );
        m_currentFrame++;

        //  std::this_thread::sleep_until(next_frame);

        auto frame_time = std::chrono::steady_clock::now() - current_frame;
        auto frame_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(frame_time).count();

        accumulated_frame_time += frame_time_ms;
        frame_count++;

        // Check if one second has passed
        if (std::chrono::steady_clock::now() - last_fps_update >= std::chrono::seconds(1))
        {
            double average_frame_time = accumulated_frame_time / frame_count;
            double average_fps = 1000.0 / average_frame_time;

            // Print the average FPS followed by a carriage return
            std::cout << "FPS: " << average_fps << ", Entities: " << currentScene()->m_ECS.getNumEntities() << "." << "\r";
            std::cout.flush();  // Ensure the output is displayed immediately

            // Reset counters for the next second
            accumulated_frame_time = 0.0;
            frame_count = 0;
            last_fps_update = std::chrono::steady_clock::now();
        }

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

            // look up the action and send the action to the scene
            currentScene()->doAction(Action(currentScene()->getActionMap().at(event.key.keysym.sym), actionType));
        }
        if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP){
            // Map the mouse button to an action, if needed (assuming you have mappings)
            if (currentScene()->getActionMap().find(event.button.button) == currentScene()->getActionMap().end()) {
                continue;
            }
            // determine start or end action by whether it was key press or release
            const std::string actionType = (event.type == SDL_MOUSEBUTTONDOWN ) ? "START" : "END";

            // look up the action and send the action to the scene
            currentScene()->doAction(Action(currentScene()->getActionMap().at(event.button.button), actionType));
        }
        if (event.type == SDL_MOUSEMOTION){
            currentScene()->updateMousePosition(Vec2{float(event.motion.x),float(event.motion.y)});
        }
    }
}

Assets& Game::assets(){
    return m_assets;
}

void Game::setPaused(bool paused)
{
    m_paused = paused;
}