#include "scenes/Scene.hpp"
#include "scenes/TextBoxHelpers.hpp"

#include <algorithm>
#include <cmath>

Scene::Scene()
{
    bindEcsRemovalObserver();
}

Scene::Scene(Game* game)
: m_game(game)
{
    bindEcsRemovalObserver();
}

Scene::~Scene() {}

void Scene::bindEcsRemovalObserver()
{
    m_ECS.setEntityRemovalObserver([this](EntityID entity) {
        m_rendererManager.queueRemoveEntity(entity);
    });
}

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
    return SpawnTextBox(
        dialog,
        size,
        font,
        parentID,
        Vec2{0, -2 * m_gridSize.y},
        60
    );
}

EntityID Scene::SpawnTextBox(
    const std::string& text,
    int size,
    const std::string& font,
    EntityID parentID,
    const Vec2& relativePosition,
    int lifespan
) {
    auto id = m_ECS.addEntity();
    CTransform& transform = m_ECS.addComponent<CTransform>(id);
    CSprite& sprite = addSprite(id, "button_unpressed", RenderLayer::Dialog);
    CText& textComponent = m_ECS.addComponent<CText>(
        id,
        text,
        static_cast<float>(size),
        font
    );
    const float padding = std::max(2.0f, static_cast<float>(size) * 0.35f);
    TextBoxHelpers::configureSpriteBackedTextBox(textComponent, transform, sprite, padding);

    m_ECS.attachChild(parentID, id, relativePosition);
    m_ECS.addComponent<CLifespan>(id, lifespan);
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

void Scene::drawSprite(const CSprite& sprite, const RectF& dst, float angle, float whiteTint)
{
    if (!sprite.visible) {
        return;
    }
    m_game->render().drawSprite(SpriteDrawCommand{
        sprite.texture,
        sprite.src,
        dst,
        angle,
        whiteTint
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

void Scene::drawSprite(const SpriteDefinition& sprite, const RectF& src, const RectF& dst, float angle)
{
    m_game->render().drawSprite(SpriteDrawCommand{
        sprite.texture(),
        src,
        dst,
        angle
    });
}

void Scene::drawWorldSprite(const CSprite& sprite, const RectF& dst, float angle, float whiteTint)
{
    if (!sprite.visible) {
        return;
    }
    m_game->render().drawWorldSprite(WorldSpriteDrawCommand{
        sprite.texture,
        sprite.src,
        dst,
        angle,
        whiteTint
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
    for (auto [e, animation, sprite] : m_ECS.View<CAnimation, CSprite>()) {
        const size_t totalFrames = animation.frameCount * animation.frameDuration;
        const bool animationFinished = animation.currentFrame + 1 >= totalFrames;

        if (animationFinished) {
            if (!animation.repeat) {
                m_ECS.queueRemoveEntity(e);
                continue;
            }
            animation.currentFrame = 0;
        }
        else {
            animation.currentFrame++;
        }
        sprite.src = animation.sourceRect();
    }
}

void Scene::sRenderBasic() {
    m_game->render().setWorldView(worldRenderView());

    if (m_drawTextures)
    {
        renderTextures();
    }

    if (m_drawCollision)
    {
        renderColliderShapes();
    }

    if (m_drawDrawGrid)
    {
        renderGrid();
    }
}

void Scene::renderTextures()
{
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
            float whiteTint = 0.0f;
            if (m_ECS.hasComponent<CDamageFlash>(e)) {
                whiteTint = m_ECS.getComponent<CDamageFlash>(e).whiteTint();
            }
            drawWorldSprite(sprite, dst, transform.angle, whiteTint);
        }
    }

    for (auto [e, dialog, transform] : m_ECS.constView<CText, CTransform>()){
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
}

void Scene::renderColliderShapes()
{
    for (auto [e, collider, transform] : m_ECS.constView<CCollider, CTransform>())
    {
        for (const auto& shape : collider.shapes) {
            const Vec2 center = transform.pos + shape.offset;
            RectF boxRect{
                center.x - shape.halfSize.x,
                center.y - shape.halfSize.y,
                shape.size.x,
                shape.size.y
            };
            m_game->render().drawWorldRect(boxRect, shape.debugColor);
        }
    }
}

void Scene::renderGrid()
{
    if (m_gridSize.x <= 0.0f || m_gridSize.y <= 0.0f) {
        return;
    }

    const RenderView view = worldRenderView();
    if (view.scale <= 0.0f) {
        return;
    }

    const Vec2 visibleWorldOrigin{
        view.cameraX - view.originX / view.scale,
        view.cameraY - view.originY / view.scale
    };
    const Vec2 visibleWorldSize{
        static_cast<float>(m_game->getWidth()) / view.scale,
        static_cast<float>(m_game->getHeight()) / view.scale
    };
    const int firstGridX = static_cast<int>(std::floor(visibleWorldOrigin.x / m_gridSize.x));
    const int firstGridY = static_cast<int>(std::floor(visibleWorldOrigin.y / m_gridSize.y));
    const int lastGridX = static_cast<int>(std::ceil((visibleWorldOrigin.x + visibleWorldSize.x) / m_gridSize.x)) - 1;
    const int lastGridY = static_cast<int>(std::ceil((visibleWorldOrigin.y + visibleWorldSize.y) / m_gridSize.y)) - 1;

    constexpr Color gridColor{0, 255, 0, 190};
    constexpr Color coordinateColor{255, 255, 180, 255};
    auto& renderer = m_game->render();
    for (int gridY = firstGridY; gridY <= lastGridY; ++gridY) {
        for (int gridX = firstGridX; gridX <= lastGridX; ++gridX) {
            const RectF cellRect{
                static_cast<float>(gridX) * m_gridSize.x,
                static_cast<float>(gridY) * m_gridSize.y,
                m_gridSize.x,
                m_gridSize.y
            };
            renderer.drawWorldRect(cellRect, gridColor);
            renderer.drawWorldText(WorldTextDrawCommand{
                std::to_string(gridX) + "," + std::to_string(gridY),
                "OCRAEXT",
                RectF{
                    cellRect.x + 1.0f,
                    cellRect.y + 1.0f,
                    std::max(1.0f, cellRect.w / 2.0f),
                    std::max(1.0f, cellRect.h / 2.0f)
                },
                coordinateColor
            });
        }
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
    CSprite& sprite = addSprite(id, unpressed, RenderLayer::MenuControl);
    Vec2 spriteSize = sprite.size();
    m_ECS.addComponent<CTransform>(id, pos);
    m_ECS.addComponent<CCollider>(id, spriteSize);
    m_ECS.addComponent<CName>(id, name);
    m_ECS.addComponent<CText>(id, dialog, spriteSize.y*0.9f, "Minecraft");
}
