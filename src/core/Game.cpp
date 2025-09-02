#include <SDL.h>
#include <SDL_image.h>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <thread>

#include "Game.h"
#include "assets/Assets.h"
#include "scenes/Scene_Menu.h"

Game::Game(const std::string & pathImages, const std::string & pathText)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");    

    m_window = SDL_CreateWindow(
        "Heaven & Hell", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        m_width, m_height, 
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    SDL_SetWindowIcon(
        m_window, 
        IMG_Load("assets/images/wizard_profile_pic.png")
    );
    if ( NULL == m_window )
    {
        std::cout << "Window nullprt: " << SDL_GetError( ) << std::endl;
    }
    SDL_SetWindowPosition(m_window, 0, 30);
    
    current_frame = steady_clock::now();
    last_fps_update = current_frame;
    
    m_renderer = SDL_CreateRenderer(m_window, -1 , SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    TTF_Init();
    Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096);
    m_assets.loadFromFile(pathImages, pathText, m_renderer);
    

    SDL_GetCurrentDisplayMode(0, &DM);
    updateResolution(int(DM.h / VIRTUAL_HEIGHT)-1);
    // updateResolution(1);
    changeScene("MENU", std::make_shared<Scene_Menu>(this));
}

void Game::updateResolution(int scale)
{
    setScale(scale);
    setWidth(scale*VIRTUAL_WIDTH);
    setHeight(scale*VIRTUAL_HEIGHT);
    SDL_SetWindowSize(m_window, scale*VIRTUAL_WIDTH, scale*VIRTUAL_HEIGHT);
}

void Game::ToggleFullscreen(){
    Uint32 flags = SDL_GetWindowFlags(m_window);
    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
        updateResolution(int(DM.h / VIRTUAL_HEIGHT)-1);
        SDL_SetWindowFullscreen(m_window, 0); // Exit fullscreen
    } else {
        updateResolution(int(DM.h / VIRTUAL_HEIGHT));
        SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP); // Enter fullscreen
    }
}

std::shared_ptr<Scene> Game::currentScene() {
    return m_sceneMap[m_currentScene];
}

void Game::changeScene(
    const std::string& sceneName, 
    std::shared_ptr<Scene> scene, 
    bool endCurrentScene)
    {
    if (endCurrentScene) {
        m_sceneMap.erase(m_currentScene);
    }
    m_currentScene = sceneName;
    if (m_sceneMap.find(sceneName) == m_sceneMap.end()) {
        m_sceneMap[sceneName] = scene;
    }
}

bool Game::isRunning() {
    return m_running;
}

int Game::framerate(){
    return m_framerate;
}

void Game::run()
{

    while (isRunning())
    {
        SDL_RenderClear( m_renderer );
        update();
        sUserInput();
        SDL_RenderPresent( m_renderer );

        FrametimeHandler(); // caps the framerate and prints the theoretical unlimited FPS.

    }
    SDL_DestroyWindow( m_window );
    SDL_Quit();
}   

void Game::FrametimeHandler()
{
    m_currentFrame++;

    nanoseconds frame_time = steady_clock::now() - current_frame;
    int64_t frame_time_ms = std::chrono::duration_cast<nanoseconds>(frame_time).count();
    
    accumulated_frame_time += frame_time_ms;
    frame_count++;
    std::this_thread::sleep_until(next_frame);

    // Check if one second has passed
    if (steady_clock::now() - last_fps_update >= seconds(1))
    {
        float average_frame_time = accumulated_frame_time / (frame_count*pow(10,6));
        float average_fps = 1000 / average_frame_time;

        // Print the average FPS followed by a carriage return
        std::cout << "\rFPS: " << average_fps << " / " << average_frame_time << "ms";

        // Reset counters for the next second
        accumulated_frame_time = 0.0;
        frame_count = 0;
        last_fps_update = steady_clock::now();
    }

    // FPS cap
    current_frame = steady_clock::now();
    next_frame = current_frame + milliseconds(1000 / m_framerate); // 60Hz
    // 
}

void Game::quit() {
    m_running = false;
}

void Game::update() {
    // if play scene exists, update it
    if (m_sceneMap.find("PLAY") != m_sceneMap.end()) {
        m_sceneMap["PLAY"]->update();
    }
    // update current scene
    if (m_currentScene != "PLAY") {
        currentScene()->update();
    }
}

SceneMap& Game::sceneMap(){
    return m_sceneMap;
}

SDL_Renderer* Game::renderer(){
    return m_renderer;
}

SDL_Window* Game::window(){
    return m_window;
}

int Game::getVirtualWidth()
{
    return VIRTUAL_WIDTH;
}

int Game::getVirtualHeight()
{
    return VIRTUAL_HEIGHT;
}

int Game::getWidth()
{
    return m_width;
}

int Game::getHeight()
{
    return m_height;
}

void Game::setWidth(int width)
{
    m_width = width;
}

void Game::setHeight(int height)
{
    m_height = height;
}

void Game::sUserInput()
{
    SDL_Event event;
    while ( SDL_PollEvent( &event ) )
    {
        if (SDL_QUIT == event.type){
            quit();
        }
        if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) { 
            if (currentScene()->getActionMap().find(event.key.keysym.sym) == currentScene()->getActionMap().end()) {
                continue;
            }
            const std::string actionType = (event.type == SDL_KEYDOWN) ? "START" : "END";
            // look up the action and send the action to the scene
            currentScene()->doAction(Action(currentScene()->getActionMap().at(event.key.keysym.sym), actionType));
        }
        if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP){
            // Map the mouse button to an action, if needed (assuming you have mappings)
            if (currentScene()->getActionMap().find(event.button.button) == currentScene()->getActionMap().end()) {
                continue;
            }
            const std::string actionType = (event.type == SDL_MOUSEBUTTONDOWN ) ? "START" : "END";
            // look up the action and send the action to the scene  
            currentScene()->doAction(Action(currentScene()->getActionMap().at(event.button.button), actionType));
        }
        if (event.type == SDL_MOUSEMOTION){
            currentScene()->updateMousePosition(Vec2{float(event.motion.x),float(event.motion.y)}/getScale());
        }

        // Mouse scroll wheel handling
        if (event.type == SDL_MOUSEWHEEL) {
            const std::string actionType = (event.type == SDL_MOUSEBUTTONDOWN ) ? "START" : "END";
            currentScene()->updateMouseScroll(event.wheel.y);
            // Determine scroll direction: up or down
            if (currentScene()->getActionMap().find(event.wheel.direction) != currentScene()->getActionMap().end()) {
                currentScene()->doAction(Action(currentScene()->getActionMap().at(event.wheel.direction), "START"));
            }
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

void Game::setScale(int scale){
    m_scale = scale;
}

int Game::getScale(){
    return m_scale;
}