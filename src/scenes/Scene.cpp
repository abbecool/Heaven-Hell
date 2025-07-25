#include "Scene.h"

Scene::Scene() {}

Scene::Scene(Game* game)
: m_game(game) {}

Scene::~Scene() {}

void Scene::doAction(const Action& action) {
    sDoAction(action);
}

void Scene::registerAction(int inputKey, const std::string& actionName) {
    m_actionMap[inputKey] = actionName;
}

void Scene::spriteRender(Animation &animation){
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

void Scene::sRenderBasic() {
    Vec2 screenCenter = Vec2{(float)width(), (float)height()}/2;
    int windowScale = m_game->getScale();
    int totalZoom = windowScale - m_camera.getCameraZoom(); // Combined zoom level with screen resolution and camera zoom
    Vec2 screenCenterZoomed = screenCenter * m_camera.getCameraZoom(); // Tranpose the screen center to the zoomed screen center
    // Above code does not have to be calculated every frame

    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    if (m_drawTextures)
    {
        auto& animationPool = m_ECS.getComponentPool<CAnimation>();
        auto layers = m_rendererManager.getLayers();
        for (const auto& layer : layers)
        {
            for (const auto& e : layer)
            {                
                if (!transformPool.hasComponent(e))
                {
                    continue;
                }

                if (animationPool.hasComponent(e))
                {
                    auto& transform = transformPool.getComponent(e);
                    auto& animation = animationPool.getComponent(e).animation;
                    
                    // Adjust the entity's position based on the camera position
                    Vec2 adjustedPosition = (transform.pos - m_camera.position) * totalZoom + screenCenterZoomed;

                    animation.setScale(transform.scale * totalZoom);
                    animation.setAngle(transform.angle);
                    animation.setDestRect(adjustedPosition - animation.getDestSize() / 2);
                    spriteRender(animation);
                }
            }
        }
        auto& dialogPool = m_ECS.getComponentPool<CDialog>();
        auto dialogView = m_ECS.view<CDialog, CTransform>();
        for (const auto& e : dialogView)
        {
            auto& dialog = dialogPool.getComponent(e);
            auto& transform = transformPool.getComponent(e);
    
            SDL_Rect texRect;
            texRect.x = (int)(transform.pos.x + dialog.offset.x - dialog.size.x/2 * 0.9f) * totalZoom + screenCenterZoomed.x;
            texRect.y = (int)(transform.pos.y + dialog.offset.y - dialog.size.y/2 * 0.8f) * totalZoom + screenCenterZoomed.y;
            texRect.w = (int)(dialog.size.x * 0.9f) * totalZoom;
            texRect.h = (int)(dialog.size.y * 0.8f) * totalZoom;
    
            SDL_RenderCopyEx(
                m_game->renderer(), 
                dialog.dialog, 
                nullptr,
                &texRect, 
                0,
                NULL,
                SDL_FLIP_NONE
                );
        } 
    }
    if (m_drawCollision)
    {
        auto viewCollisions = m_ECS.signatureView<CCollisionBox, CTransform>();
        auto& collisionPool = m_ECS.getComponentPool<CCollisionBox>();
        renderBox<CCollisionBox>(viewCollisions, transformPool, collisionPool, screenCenterZoomed, totalZoom);
    }
    if (m_drawInteraction)
    {
        auto viewInteractions = m_ECS.signatureView<CInteractionBox, CTransform>();
        auto& interactionPool = m_ECS.getComponentPool<CInteractionBox>();
        renderBox<CInteractionBox>(viewInteractions, transformPool, interactionPool, screenCenterZoomed, totalZoom);
    }
}

template<typename BoxType>
void Scene::renderBox(std::vector<EntityID> view, ComponentPool<CTransform> transformPool, ComponentPool<BoxType> boxPool, const Vec2& screenCenterZoomed, int totalZoom)
{
    for (auto e : view)
    {   
        auto& transform = transformPool.getComponent(e);
        auto& box = boxPool.getComponent(e);
        // Adjust the box box position based on the camera position
        SDL_Rect boxRect;
        boxRect.x = (int)(transform.pos.x - box.halfSize.x - m_camera.position.x) * totalZoom + screenCenterZoomed.x;
        boxRect.y = (int)(transform.pos.y - box.halfSize.y - m_camera.position.y) * totalZoom + screenCenterZoomed.y;
        boxRect.w = (int)(box.size.x) * totalZoom;
        boxRect.h = (int)(box.size.y) * totalZoom;
        SDL_SetRenderDrawColor(m_game->renderer(), box.red, box.green, box.blue, 255);
        SDL_RenderDrawRect(m_game->renderer(), &boxRect);
    }
}

int Scene::width() const {
    return m_game->getVirtualWidth();
}

int Scene::height() const {
    return m_game->getVirtualHeight();
}

size_t Scene::currentFrame() const {
    return m_currentFrame;
}

bool Scene::hasEnded() const {
    return m_hasEnded;
}

ActionMap& Scene::getActionMap() {
    return m_actionMap;
}

void Scene::updateMousePosition(Vec2 pos){
    m_mousePosition = pos;
    m_mouseState.pos = pos;
}

void Scene::updateMouseScroll(int scroll){
    m_mouseScroll = scroll;
    m_mouseState.scroll = scroll;
}

Vec2 Scene::getMousePosition(){
    return m_mousePosition;
}

MouseState Scene::getMouseState(){
    return m_mouseState;
}

Vec2 Scene::gridToMidPixel(Vec2 grid, EntityID entity) {
    Vec2 offset;
    Vec2 eSize;
    if ( m_ECS.hasComponent<CAnimation>(entity) ){
        eSize = m_ECS.getComponent<CAnimation>(entity).animation.getSize();
    } else {
        eSize = m_gridSize;
    }
    
    Vec2 eScale = {1.0f, 1.0f};
    offset = (m_gridSize - eSize * eScale) / 2.0;

    return grid + m_gridSize / 2 - offset;
}

Vec2 Scene::getCameraPosition() {
    return Vec2{0,0};
}