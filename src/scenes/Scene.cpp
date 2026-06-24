#include "scenes/Scene.hpp"

Scene::Scene() {}

Scene::Scene(Game* game)
: m_game(game) {}

Scene::~Scene() {}

void Scene::doAction(const Action& action) {
    sDoAction(action);
}

void Scene::registerAction(InputCode inputKey, const std::string& actionName) {
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
    CSprite& sprite = addSprite(id, "button_unpressed", layer);
    Vec2 spriteSize = sprite.size();
    m_ECS.addComponent<CText>(id, dialog, spriteSize.y*0.9f, font);
    m_ECS.addComponent<CLifespan>(id, 60);
    return id;
}

CSprite& Scene::addSprite(EntityID entity, const std::string& spriteName, int layer)
{
    CSprite& sprite = m_ECS.addComponent<CSprite>(entity, getSprite(spriteName), layer);
    m_rendererManager.addEntityToLayer(entity, layer);
    return sprite;
}

void Scene::addVisual(EntityID entity, const std::string& spriteName, int layer, bool repeat)
{
    addSprite(entity, spriteName, layer);
    if (getSprite(spriteName).isAnimated()) {
        m_ECS.addComponent<CAnimation>(entity, getSprite(spriteName), repeat);
    }
}

void Scene::setSprite(EntityID entity, const std::string& spriteName)
{
    const SpriteDefinition& spriteDefinition = getSprite(spriteName);
    CSprite& sprite = m_ECS.getComponent<CSprite>(entity);
    sprite.texture = spriteDefinition.texture();
    sprite.src = spriteDefinition.firstFrame();
}

void Scene::setAnimation(EntityID entity, const std::string& spriteName, bool repeat)
{
    setSprite(entity, spriteName);
    if (m_ECS.hasComponent<CAnimation>(entity)) {
        m_ECS.getComponent<CAnimation>(entity) = CAnimation(getSprite(spriteName), repeat);
        return;
    }
    m_ECS.addComponent<CAnimation>(entity, getSprite(spriteName), repeat);
}

void Scene::drawSprite(const CSprite& sprite, const RectF& dst, float angle)
{
    if (!sprite.visible) {
        return;
    }
    m_game->render().drawSprite(SpriteDrawCommand{
        sprite.texture,
        sprite.src,
        dst,
        angle
    });
}

void Scene::drawSprite(const SpriteDefinition& sprite, const RectF& dst, float angle)
{
    m_game->render().drawSprite(SpriteDrawCommand{
        sprite.texture(),
        sprite.firstFrame(),
        dst,
        angle
    });
}

void Scene::drawWorldSprite(const CSprite& sprite, const RectF& dst, float angle)
{
    if (!sprite.visible) {
        return;
    }
    m_game->render().drawWorldSprite(WorldSpriteDrawCommand{
        sprite.texture,
        sprite.src,
        dst,
        angle
    });
}

void Scene::drawWorldSprite(const SpriteDefinition& sprite, const RectF& dst, float angle)
{
    m_game->render().drawWorldSprite(WorldSpriteDrawCommand{
        sprite.texture(),
        sprite.firstFrame(),
        dst,
        angle
    });
}

void Scene::drawWorldSprite(const SpriteDefinition& sprite, const RectF& src, const RectF& dst, float angle)
{
    m_game->render().drawWorldSprite(WorldSpriteDrawCommand{
        sprite.texture(),
        src,
        dst,
        angle
    });
}

RenderView Scene::worldRenderView()
{
    const Vec2 screenCenter = Vec2{
        static_cast<float>(width()),
        static_cast<float>(height())
    } / 2;
    const int totalZoom = m_game->getScale() - m_camera.getCameraZoom();
    const Vec2 screenCenterZoomed = screenCenter * m_camera.getCameraZoom();

    return RenderView{
        m_camera.position.x,
        m_camera.position.y,
        static_cast<float>(totalZoom),
        screenCenterZoomed.x,
        screenCenterZoomed.y
    };
}

void Scene::updateAnimations()
{
    auto view = m_ECS.View<CAnimation, CSprite>();
    auto& animationPool = m_ECS.getComponentPool<CAnimation>();
    auto& spritePool = m_ECS.getComponentPool<CSprite>();
    for (auto e : view) {
        auto& animation = animationPool.getComponent(e);
        if (animation.hasEnded() && !animation.repeat) {
            m_ECS.queueRemoveEntity(e);
            continue;
        }
        animation.currentFrame++;
        spritePool.getComponent(e).src = animation.sourceRect();
    }
}

void Scene::sRenderBasic() {
    m_game->render().setWorldView(worldRenderView());
    if (!m_drawTextures){
        return;
    }

    auto& transformPool = m_ECS.getComponentPool<CTransform>();
    auto& spritePool = m_ECS.getComponentPool<CSprite>();
    const auto& layers = m_rendererManager.getLayers();
    for (const auto& layer : layers){
        for (const auto& e : layer){                
            if (!transformPool.hasComponent(e) || !spritePool.hasComponent(e)){
                continue;
            }
            auto& transform = transformPool.getComponent(e);
            auto& sprite = spritePool.getComponent(e);

            const Vec2 destSize = sprite.size() * transform.scale;
            RectF dst{
                transform.pos.x - destSize.x / 2,
                transform.pos.y - destSize.y / 2,
                destSize.x,
                destSize.y
            };
            drawWorldSprite(sprite, dst, transform.angle);
        }
    }

    auto& dialogPool = m_ECS.getComponentPool<CText>();
    auto dialogView = m_ECS.View<CText, CTransform>();
    for (const auto& e : dialogView){
        CText& dialog = dialogPool.getComponent(e);
        CTransform& transform = transformPool.getComponent(e);
                    
        m_game->render().drawWorldText(WorldTextDrawCommand{
            dialog.text,
            dialog.font_name,
            RectF{
                transform.pos.x - dialog.size.x / 2,
                transform.pos.y - dialog.size.y / 2,
                dialog.size.x,
                dialog.size.y
            },
            {255, 255, 255, 255}
        });
    }

    if (m_drawCollision)
    {
        auto viewCollisions = m_ECS.View<CCollisionBox, CTransform>();
        auto& collisionPool = m_ECS.getComponentPool<CCollisionBox>();
        renderBox<CCollisionBox>(viewCollisions, transformPool, collisionPool);
    }
    if (m_drawInteraction)
    {
        auto viewInteractions = m_ECS.View<CInteractionBox, CTransform>();
        auto& interactionPool = m_ECS.getComponentPool<CInteractionBox>();
        renderBox<CInteractionBox>(viewInteractions, transformPool, interactionPool);
    }
}

template<typename BoxType>
void Scene::renderBox(
    const std::vector<EntityID>& view,
    ComponentPool<CTransform>& transformPool,
    ComponentPool<BoxType>& boxPool
) {
    for (auto e : view)
    {   
        auto& transform = transformPool.getComponent(e);
        auto& box = boxPool.getComponent(e);
        RectF boxRect{
            transform.pos.x - box.halfSize.x,
            transform.pos.y - box.halfSize.y,
            box.size.x,
            box.size.y
        };
        m_game->render().drawWorldRect(boxRect, box.color);
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
    if ( m_ECS.hasComponent<CSprite>(entity) ){
        eSize = m_ECS.getComponent<CSprite>(entity).size();
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

const SpriteDefinition& Scene::getSprite(const std::string& name) const {
    return m_game->assets().getSprite(name);
}

void Scene::spawnButton(
    const Vec2 pos, 
    const std::string& unpressed, 
    const std::string& name, 
    const std::string& dialog)
{   
    EntityID id = m_ECS.addEntity();
    CSprite& sprite = addSprite(id, unpressed, 3);
    Vec2 spriteSize = sprite.size();
    m_ECS.addComponent<CTransform>(id, pos);
    m_ECS.addComponent<CCollisionBox>(id, spriteSize);
    m_ECS.addComponent<CName>(id, name);
    m_ECS.addComponent<CText>(id, dialog, spriteSize.y*0.9f, "Minecraft");
}
