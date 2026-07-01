#pragma once

#include "ecs/Components.hpp"
#include "physics/CollisionManager.hpp"
#include "physics/InventoryManager.hpp"
#include "story/StoryManager.hpp"
#include "physics/Camera.hpp"
#include "scenes/Scene.hpp"
#include "scenes/Scene_Pause.hpp"
#include "scenes/Scene_Finish.hpp"
#include "physics/Level_Loader.hpp"
#include "story/EventBus.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

class Scene_Play : public Scene
{
    protected:
    
    friend class LevelLoader;
    EntityID m_player;
    std::string m_playerDefinition = "player";
    std::string m_levelPath;
    
    CollisionManager m_collisionManager;
    InteractionManager m_interactionManager;
    InventoryManager m_inventoryManager;
    StoryManager m_storyManager;
    LevelLoader m_levelLoader;
    EventBus m_eventBus;
    
    float m_zoomStep = 2;
    Vec2 m_levelSize;
    bool m_newGame;
    
    std::unordered_map<std::string, std::unordered_set<std::string>> m_damageToEnemyMap = {
        {"fire", {"grass"}},
        {"water", {"fire"}},
        {"ice", {"water"}},
        {"explosive", {"rock"}},
        {"piercing", {"shielded"}}
    };
    
    void loadMobsNItems(const std::string& path);
    void SubscribeToStoryEvents();
    void saveGame();
    
    EntityID spawnPlayer();
    EntityID spawnShadow(EntityID parentID);
    
    EntityID spawnObstacle  (const Vec2 pos, bool movable, const int frame );
    EntityID spawnWater     (const Vec2 pos, const std::string tag, const int frame );
    std::vector<EntityID> spawnDualTiles(
        const Vec2 pos, 
        std::array<int, 5> tileIndex
    );
    
    void sLoader();
    void sAttack();
    void sAI();
    void sMovement();
    void sInteraction();
    void sCollision();
    void sStatus();
    void sAnimation();
    void sRender();
    void sRenderHealth();
    void sRenderCurrency();
    void sRenderInventory();
    void sRenderUI();
    void sAudio();
    
    void sDoAction(const Action&);
    void onEnd();
    void togglePause();
    void changePlayerState(EntityID entity, PlayerState s);
    void beginProjectileFlight(EntityID projectileID);
    bool hasLineOfSight(Vec2 origin, Vec2 target);
    bool rayIntersectsAABB(Vec2 origin, Vec2 dir, float maxDist, 
        Vec2 boxMin, Vec2 boxMax);
    void tickPatrol(CAIAgent& agent, Vec2 pos, CInput& intent);
    
    public:    
    Scene_Play(Game* game, std::string path, bool newGame);
    Vec2 getCameraPosition() override;
    
    EntityID spawnProjectile(Vec2 startPos, Vec2 vel);
    EntityID spawnHitbox(EntityID attackerID, Vec2 direction, const CWeapon& weapon);
    void destroyProjectile(EntityID projectileID);
    bool addCurrencyToPlayer(int amount);
    bool addCurrencyToPlayer(const Item& item);
    void updateActiveItem(int newActiveItem);
    void update();
    void setPaused(bool);
    
    StoryManager& getStoryManager() {
        return m_storyManager;
    }  
    
    InventoryManager& getInventoryManager() {
        return m_inventoryManager;
    }  

    EntityID SpawnFromJSON(std::string name, Vec2 pos);
    EntityID Spawn(std::string name, Vec2 pos);
    EntityID DropItem(const Item& item, Vec2 position);

// event - subscriber: These emit a signal when called
    void onItemPickedUp(const std::string& itemName) {
        Event e{ EventType::ItemPickedUp, itemName };
        m_eventBus.emit(e);
    }

    void onEnemyKilled(const std::string& itemName) {
        Event e{ EventType::EntityKilled, itemName };
        m_eventBus.emit(e);
    }

    void Emit(Event e) {
        m_eventBus.emit(e);
    }

    void onFinish();
    void OnPlayerDeath();
    EntityID changePlayerID(EntityID otherID);
};
