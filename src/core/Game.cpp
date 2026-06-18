#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <exception>
#include <stdexcept>
#include <thread>

#include "Game.hpp"
#include "assets/Assets.hpp"
#include "core/SDLPlatform.hpp"
#include "render/opengl/OpenGLRenderBackend.hpp"
#include "render/sdl/SDLRenderBackend.hpp"
#include "scenes/Scene_Menu.hpp"

namespace {
std::unique_ptr<RenderBackend> createRenderBackend(RenderDriver driver, SDLPlatform& platform)
{
    switch (driver) {
    case RenderDriver::SDLRenderer:
        return std::make_unique<SDLRenderBackend>(platform.window());
    case RenderDriver::OpenGL:
        return std::make_unique<OpenGLRenderBackend>(platform.window());
    }
    throw std::runtime_error("Unknown render driver.");
}
}

Game::Game(const std::string & pathImages, const std::string & pathText)
{
    try {
        m_platform = std::make_unique<SDLPlatform>(
            "Heaven & Hell",
            m_width,
            m_height,
            m_renderDriver);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        m_running = false;
        return;
    }
    
    current_frame = steady_clock::now();
    last_fps_update = current_frame;

    try {
        m_renderBackend = createRenderBackend(m_renderDriver, *m_platform);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        m_running = false;
        return;
    }

    m_assets.loadFromFile(pathImages, pathText, *m_renderBackend, *m_platform);
    
    updateResolution(displayScale(false));
    changeScene("MENU", std::make_shared<Scene_Menu>(this));
}

Game::~Game() = default;

void Game::updateResolution(int scale)
{
    const int width = scale * VIRTUAL_WIDTH;
    const int height = scale * VIRTUAL_HEIGHT;
    setScale(scale);
    setWidth(width);
    setHeight(height);
    m_platform->setWindowSize(width, height);
    m_renderBackend->onWindowResized(width, height);
    m_fpsRect = {
        static_cast<float>(width - 100),
        static_cast<float>(height - 20),
        100.0f,
        20.0f
    };
}

void Game::ToggleFullscreen(){
    if (m_platform->isFullscreen()) {
        m_platform->setFullscreen(false);
        updateResolution(displayScale(false));
    } else {
        m_platform->setFullscreen(true);
        updateResolution(displayScale(true));
    }
}

int Game::displayScale(bool fullscreen) const
{
    const int displayHeight = m_platform->currentDisplaySize(VIRTUAL_WIDTH, VIRTUAL_HEIGHT).h;
    const int scale = displayHeight / VIRTUAL_HEIGHT;
    return std::max(1, fullscreen ? scale : scale - 1);
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
        m_renderBackend->beginFrame({0, 0, 0, 255});
        update();
        sUserInput();
        FrametimeHandler(); // caps the framerate and prints the theoretical unlimited FPS.
        m_renderBackend->endFrame();
    }
    m_assets.shutdown();
    m_renderBackend.reset();
    m_platform.reset();
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

        accumulated_frame_time = 0.0;
        frame_count = 0;
        last_fps_update = steady_clock::now();
    }
    m_renderBackend->drawText(TextDrawCommand{
        "FPS: " + std::to_string(average_fps),
        "Minecraft",
        m_fpsRect,
        {255, 255, 255, 255}
    });
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

RenderBackend& Game::render(){
    return *m_renderBackend;
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
    m_platform->pollEvents(*this);
}

Assets& Game::assets(){
    return m_assets;
}

void Game::playAudio(const std::string& name)
{
    m_platform->playAudio(name);
}

PixelImage Game::loadImagePixels(const std::string& path) const
{
    try {
        return m_platform->loadImagePixels(path);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return {};
    }
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
