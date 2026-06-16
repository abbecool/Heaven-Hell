#pragma once

#include "core/Action.hpp"
#include "ecs/ECS.hpp"
#include "physics/Renderer.hpp"
#include "core/Game.hpp"
#include "physics/Camera.hpp"
#include "physics/Physics.hpp"
#include "ecs/ComponentFactory.hpp"
#include "core/InputCode.hpp"

#include <cstddef>
#include <map>

class Game;

typedef std::map<InputCode, std::string> ActionMap;

struct MouseState {
    Vec2 pos = {0,0};
    int scroll = 0;
    bool hold_left_click = false;
    bool hold_right_click = false;
};

class Scene
{
    protected:

    Game* m_game = nullptr;  
    ActionMap m_actionMap;
    bool m_pause = false;
    bool m_hasEnded = false;
    size_t m_currentFrame = 0;
    bool m_restart = false;
    Physics m_physics;

    Camera m_camera;
    Vec2 m_mousePosition;
    MouseState m_mouseState;
    bool m_drawTextures = true;
    bool m_drawCollision = false;
    bool m_drawInteraction = false;
    bool m_drawDrawGrid = true;
    Vec2 m_gridSize = {16, 16};    
    virtual void onEnd() = 0;
    Vec2 gridToMidPixel(Vec2 grid, EntityID);
    
    public:
    ECS m_ECS;
    RendererManager m_rendererManager;
    
    Scene();
    Scene(Game* game);
    virtual ~Scene();
    
    virtual void update() = 0;
    virtual void sDoAction(const Action& action) = 0;
    EntityID SpawnDialog(std::string dialog, int size, std::string font, EntityID parentID);
    
    CSprite& addSprite(EntityID entity, const std::string& spriteName, int layer);
    void addVisual(EntityID entity, const std::string& spriteName, int layer, bool repeat = true);
    void setSprite(EntityID entity, const std::string& spriteName);
    void setAnimation(EntityID entity, const std::string& spriteName, bool repeat = true);
    void drawSprite(const CSprite& sprite, const RectF& dst, float angle = 0.0f);
    void drawSprite(const SpriteDefinition& sprite, const RectF& dst, float angle = 0.0f);
    void updateAnimations();
    void sRenderBasic();
    template<typename T>
    void renderBox(std::vector<EntityID> viewCollisions, ComponentPool<CTransform> transform, ComponentPool<T> box, const Vec2& screenCenterZoomed, int totalZoom);

    virtual void doAction(const Action& action);
    void registerAction(InputCode inputKey, const std::string& actionName);
    
    int width() const;
    int height() const;
    size_t currentFrame() const;

    bool hasEnded() const;
    ActionMap& getActionMap();

    void updateMousePosition(Vec2 pos);
    void updateMouseScroll(int scroll);

    MouseState getMouseState();
    Vec2 getMousePosition();
    virtual Vec2 getCameraPosition();
    const SpriteDefinition& getSprite(const std::string& name) const;

    void spawnButton(
        const Vec2 pos, 
        const std::string& button_name, 
        const std::string& name, 
        const std::string& dialog
    );
    
};
