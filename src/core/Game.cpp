#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

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
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        m_running = false;
        return;
    }

    m_window = SDL_CreateWindow(
        "Heaven & Hell", 
        m_width, m_height, 
        SDL_WINDOW_RESIZABLE
    );

    if ( NULL == m_window )
    {
        std::cout << "Window nullprt: " << SDL_GetError( ) << std::endl;
        m_running = false;
        return;
    }

    SDL_Surface* icon = IMG_Load("assets/images/wizard_profile_pic.png");
    if (icon) {
        SDL_SetWindowIcon(m_window, icon);
        SDL_DestroySurface(icon);
    }
    SDL_SetWindowPosition(m_window, 0, 30);
    
    current_frame = steady_clock::now();
    last_fps_update = current_frame;
    
    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        m_running = false;
        return;
    }
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetDefaultTextureScaleMode(m_renderer, SDL_SCALEMODE_NEAREST);
    if (!TTF_Init()) {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
        m_running = false;
        return;
    }
    m_assets.loadFromFile(pathImages, pathText, m_renderer);
    
    const SDL_DisplayMode* displayMode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    if (displayMode) {
        DM = *displayMode;
    } else {
        DM.w = VIRTUAL_WIDTH;
        DM.h = VIRTUAL_HEIGHT;
    }
    updateResolution(std::max(1, int(DM.h / VIRTUAL_HEIGHT)-1));
    // updateResolution(1);
    changeScene("MENU", std::make_shared<Scene_Menu>(this));
}

void Game::updateResolution(int scale)
{
    setScale(scale);
    setWidth(scale*VIRTUAL_WIDTH);
    setHeight(scale*VIRTUAL_HEIGHT);
    SDL_SetWindowSize(m_window, scale*VIRTUAL_WIDTH, scale*VIRTUAL_HEIGHT);
    m_fpsCacheRect = {scale*VIRTUAL_WIDTH-100, scale*VIRTUAL_HEIGHT-20, 100, 20};
}

void Game::ToggleFullscreen(){
    SDL_WindowFlags flags = SDL_GetWindowFlags(m_window);
    if (flags & SDL_WINDOW_FULLSCREEN) {
        updateResolution(std::max(1, int(DM.h / VIRTUAL_HEIGHT)-1));
        SDL_SetWindowFullscreen(m_window, false); // Exit fullscreen
    } else {
        updateResolution(std::max(1, int(DM.h / VIRTUAL_HEIGHT)));
        SDL_SetWindowFullscreen(m_window, true); // Enter fullscreen
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
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        SDL_RenderClear(m_renderer);
        update();
        sUserInput();
        FrametimeHandler(); // caps the framerate and prints the theoretical unlimited FPS.
        SDL_RenderPresent(m_renderer);

    }
    m_assets.shutdown();
    if (m_fpsCacheTexture) {
        SDL_DestroyTexture(m_fpsCacheTexture);
        m_fpsCacheTexture = nullptr;
    }
    TTF_Quit();
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}   

void RenderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color)
{
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, color);
    if (!surface) return;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FRect dstRect = { static_cast<float>(x), static_cast<float>(y), static_cast<float>(surface->w), static_cast<float>(surface->h) };
    SDL_DestroySurface(surface);
    if (!texture) return;

    SDL_RenderTexture(renderer, texture, nullptr, &dstRect);

    SDL_DestroyTexture(texture);
}


void Game::FrametimeHandler()
{
    m_currentFrame++;
    if (!m_renderFPS){
        return;
    }
    auto now = std::chrono::steady_clock::now();
    auto frameDuration = now - current_frame;

    int64_t frame_time_ns = std::chrono::duration_cast<nanoseconds>(frameDuration).count();
    accumulated_frame_time += frame_time_ns;
    frame_count++;

    if (steady_clock::now() - last_fps_update >= seconds(1))
    {
        float average_frame_time = accumulated_frame_time / frame_count;
        average_fps = 1e9 / average_frame_time;

        SDL_Color white = {255, 255, 255, 255};
        auto font = m_assets.getFont("Minecraft");
        std::string temp_str = "FPS: " + std::to_string(average_fps);
        const char* text = temp_str.c_str();
        SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, text, 0, white); 

        if (m_fpsCacheTexture) {
            SDL_DestroyTexture(m_fpsCacheTexture);
        }
        m_fpsCacheTexture = SDL_CreateTextureFromSurface(m_renderer, surfaceMessage);
        SDL_DestroySurface(surfaceMessage);

        accumulated_frame_time = 0.0;
        frame_count = 0;
        last_fps_update = steady_clock::now();
    }
    if (m_fpsCacheTexture) {
        SDL_FRect fpsRect = {
            static_cast<float>(m_fpsCacheRect.x),
            static_cast<float>(m_fpsCacheRect.y),
            static_cast<float>(m_fpsCacheRect.w),
            static_cast<float>(m_fpsCacheRect.h)
        };
        SDL_RenderTexture(m_renderer, m_fpsCacheTexture, NULL, &fpsRect);
    }
    auto targetFrameDuration = std::chrono::milliseconds(1000 / m_framerate);

    if (frameDuration < targetFrameDuration) {
        std::this_thread::sleep_for(targetFrameDuration - frameDuration);
    }
    current_frame = std::chrono::steady_clock::now();
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
    currentScene()->updateMouseScroll(0);
    while ( SDL_PollEvent( &event ) )
    {
        if (SDL_EVENT_QUIT == event.type){
            quit();
        }
        ActionMap actionMap = currentScene()->getActionMap();
        if(event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) { 
            SDL_Keycode key = event.key.key;
            if (actionMap.find(key) == actionMap.end()) {
                continue;
            }
            const std::string actionType = (event.type == SDL_EVENT_KEY_DOWN) ? "START" : "END";
            // look up the action and send the action to the scene
            currentScene()->doAction(Action(actionMap.at(key), actionType));
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP){
            // Map the mouse button to an action, if needed (assuming you have mappings)
            if (actionMap.find(event.button.button) == actionMap.end()) {
                continue;
            }
            const std::string actionType = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ) ? "START" : "END";
            // look up the action and send the action to the scene  
            currentScene()->doAction(Action(actionMap.at(event.button.button), actionType));
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION){
            currentScene()->updateMousePosition(Vec2{float(event.motion.x),float(event.motion.y)}/getScale());
        }
        // Mouse scroll wheel handling
        else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            currentScene()->updateMouseScroll(event.wheel.integer_y);
            // Determine scroll direction: up or down
            if (actionMap.find(event.wheel.direction) == actionMap.end()) {
                continue;
            }
            currentScene()->doAction(Action(actionMap.at(event.wheel.direction), ""));
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

void Game::toggleRenderFPS(){
    m_renderFPS = !m_renderFPS;
    std::cout << m_renderFPS << std::endl;
}
