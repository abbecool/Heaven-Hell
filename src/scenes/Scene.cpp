#include "scenes/Scene.h"

namespace {
SDL_FRect toFRect(const SDL_Rect& rect)
{
    return SDL_FRect{
        static_cast<float>(rect.x),
        static_cast<float>(rect.y),
        static_cast<float>(rect.w),
        static_cast<float>(rect.h)
    };
}
}

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

EntityID Scene::SpawnDialog(
    std::string dialog, 
    int size, 
    std::string font, 
    EntityID parentID
)
{
    int layer = 8;
    auto id = m_ECS.addEntity();
    m_ECS.addComponent<CTransform>(id);
    m_ECS.addComponent<CChild>(parentID, id);
    Vec2 relativePosition = {0, -2*m_gridSize.y};
    m_ECS.addComponent<CParent>(id, parentID, relativePosition);
    CAnimation& animation =  m_ECS.addComponent<CAnimation>(id, getAnimation("button_unpressed"), layer);
    Vec2 animationSize = animation.animation.getSize();
    CText& text = m_ECS.addComponent<CText>(id, dialog, animationSize.y*0.9f, font);
    animation.animation.setScale(Vec2{2*animationSize.x, animationSize.y});
    m_rendererManager.addEntityToLayer(id, layer);
    m_ECS.addComponent<CLifespan>(id, 60);
    return id;
}

void Scene::spriteRender(Animation &animation){
    SDL_FRect srcRect = toFRect(*animation.getSrcRect());
    SDL_FRect destRect = toFRect(*animation.getDestRect());
    SDL_RenderTextureRotated(
        m_game->renderer(), 
        animation.getTexture(), 
        &srcRect, 
        &destRect,
        animation.getAngle(),
        NULL,
        SDL_FLIP_NONE
    );
}

void Scene::sRenderBasic() {
    if (!m_drawTextures){
        return;
    }
    Vec2 screenCenter = Vec2{(float)width(), (float)height()}/2;
    int windowScale = m_game->getScale();
    int totalZoom = windowScale - m_camera.getCameraZoom(); // Combined zoom level with screen resolution and camera zoom
    Vec2 screenCenterZoomed = screenCenter * m_camera.getCameraZoom(); // Tranpose the screen center to the zoomed screen center
    // Above code does not have to be calculated every frame

    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& animationPool = m_ECS.getComponentPool<CAnimation>();
    auto layers = m_rendererManager.getLayers();
    for (const auto& layer : layers){
        for (const auto& e : layer){                
            if (!transformPool.hasComponent(e) || !animationPool.hasComponent(e)){
                continue;
            }
            auto& transform = transformPool.getComponent(e);
            auto& animation = animationPool.getComponent(e).animation;

            // Adjust the entity's position based on the camera position
            Vec2 adjustedPosition = (transform.pos - m_camera.position)*totalZoom 
                                        + screenCenterZoomed;

            animation.setScale(transform.scale * totalZoom);
            animation.setAngle(transform.angle);
            animation.setDestRect(adjustedPosition - animation.getDestSize() / 2);
            spriteRender(animation);
        }
    }

    auto& dialogPool = m_ECS.getComponentPool<CText>();
    auto dialogView = m_ECS.View<CText, CTransform>();
    for (const auto& e : dialogView){
        CText& dialog = dialogPool.getComponent(e);
        SDL_Color color = {255, 255, 255, 255};
        SDL_Surface* surface = TTF_RenderText_Blended(getFont(dialog.font_name), dialog.text.c_str(), 0, color);
        if (!surface) {
            SDL_Log("TTF_RenderText_Blended error: %s", SDL_GetError());
            continue;
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(m_game->renderer(), surface);
        SDL_DestroySurface(surface);
        if (!texture) {
            SDL_Log("SDL_CreateTextureFromSurface error: %s", SDL_GetError());
            continue;
        }
        CTransform& transform = transformPool.getComponent(e);
        Vec2 pos = (transform.pos - m_camera.position- dialog.size/2)*totalZoom 
                    + screenCenterZoomed;
                    
        SDL_Rect texRect;
        texRect.x = int(pos.x);
        texRect.y = int(pos.y);
        texRect.w = int(dialog.size.x * totalZoom);
        texRect.h = int(dialog.size.y * totalZoom);

        SDL_FRect renderRect = toFRect(texRect);
        SDL_RenderTextureRotated(
            m_game->renderer(),
            texture,
            nullptr,
            &renderRect,
            0,
            nullptr,
            SDL_FLIP_NONE
        );

        SDL_DestroyTexture(texture); // Free after rendering to avoid leaks
    }

    if (m_drawCollision)
    {
        auto viewCollisions = m_ECS.View<CCollisionBox, CTransform>();
        auto& collisionPool = m_ECS.getComponentPool<CCollisionBox>();
        renderBox<CCollisionBox>(viewCollisions, transformPool, collisionPool, screenCenterZoomed, totalZoom);
    }
    if (m_drawInteraction)
    {
        auto viewInteractions = m_ECS.View<CInteractionBox, CTransform>();
        auto& interactionPool = m_ECS.getComponentPool<CInteractionBox>();
        renderBox<CInteractionBox>(viewInteractions, transformPool, interactionPool, screenCenterZoomed, totalZoom);
    }
}

template<typename BoxType>
void Scene::renderBox(
    std::vector<EntityID> view, 
    ComponentPool<CTransform> transformPool, 
    ComponentPool<BoxType> boxPool, 
    const Vec2& screenCenterZoomed, 
    int totalZoom
) {
    for (auto e : view)
    {   
        auto& transform = transformPool.getComponent(e);
        auto& box = boxPool.getComponent(e);
        // Adjust the box box position based on the camera position
        SDL_Rect boxRect;
        boxRect.x = (transform.pos.x - box.halfSize.x - m_camera.position.x) * totalZoom + screenCenterZoomed.x;
        boxRect.y = (transform.pos.y - box.halfSize.y - m_camera.position.y) * totalZoom + screenCenterZoomed.y;
        boxRect.w = box.size.x * totalZoom;
        boxRect.h = box.size.y * totalZoom;
        SDL_Color color = box.color;
        SDL_SetRenderDrawColor(m_game->renderer(), color.r, color.g, color.b, color.a);
        SDL_FRect renderRect = toFRect(boxRect);
        SDL_RenderRect(m_game->renderer(), &renderRect);
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

const Animation& Scene::getAnimation(const std::string& name) const {
    return m_game->assets().getAnimation(name);
}

TTF_Font* Scene::getFont(const std::string& name) const {
    return m_game->assets().getFont(name);
}

void Scene::spawnButton(
    const Vec2 pos, 
    const std::string& unpressed, 
    const std::string& name, 
    const std::string& dialog)
{   
    EntityID id = m_ECS.addEntity();
    m_rendererManager.addEntityToLayer(id, 3);
    CAnimation& animation = m_ECS.addComponent<CAnimation>(id, getAnimation(unpressed));
    Vec2 animationSize = animation.animation.getSize();
    m_ECS.addComponent<CTransform>(id, pos);
    m_ECS.addComponent<CCollisionBox>(id, animationSize);
    m_ECS.addComponent<CName>(id, name);
    m_ECS.addComponent<CText>(id, dialog, animationSize.y*0.9f, "Minecraft");
}
