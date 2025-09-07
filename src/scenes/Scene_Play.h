#pragma once

#include "ecs/Components.h"
#include "physics/CollisionManager.h"
#include "story/StoryManager.h"
#include "physics/Camera.h"
#include "scenes/Scene.h"
#include "scenes/Scene_Inventory.h"
#include "scenes/Scene_Pause.h"
#include "scenes/Scene_Finish.h"
#include "physics/Level_Loader.h"
#include "story/EventBus.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

class Scene_Play : public Scene
{
    struct PlayerConfig
    {
        int x, y;
        float SPEED, MAXSPEED;
        int HP, DAMAGE;
        int ATTACK_SPEED;
    };
    
    protected:
    
    friend class LevelLoader;
    EntityID m_player;
    std::string m_levelPath;
    PlayerConfig m_playerConfig;
    PlayerConfig m_rooterConfig;
    PlayerConfig m_goblinConfig;
    
    CollisionManager m_collisionManager;
    InteractionManager m_interactionManager;
    StoryManager m_storyManager;
    LevelLoader m_levelLoader;
    EventBus m_eventBus;

    float m_zoomStep = 2;
    std::shared_ptr<Scene_Inventory> m_inventory_scene;
    Vec2 m_levelSize;
    bool m_inventoryOpen = false;
    bool m_newGame;
    
    std::unordered_map<std::string, std::unordered_set<std::string>> m_damageToEnemyMap = {
        {"fire", {"grass"}},
        {"water", {"fire"}},
        {"ice", {"water"}},
        {"explosive", {"rock"}},
        {"piercing", {"shielded"}}
    };
    
    void loadConfig(const std::string& path);
    void loadMobsNItems(const std::string& path);
    void SubscribeToStoryEvents();
    void saveGame(const std::string& filename);
    
    EntityID spawnPlayer();
    EntityID spawnNPC(Vec2 pos);
    EntityID spawnDwarf(Vec2 pos);
    EntityID spawnWeapon(Vec2 pos, std::string weaponName = "staff");
    EntityID spawnSword(Vec2 pos, std::string weaponName = "sword");
    EntityID spawnProjectile(EntityID player, Vec2 vel, int layer);
    EntityID spawnCoin(Vec2 pos, const size_t layer);
    EntityID spawnShadow(EntityID parentID, Vec2 relPos, int size, int layer);
    EntityID spawnDecoration(
        Vec2 pos, 
        Vec2 collisionBox, 
        const size_t layer, 
        std::string animation
    );
    
    EntityID spawnObstacle  (const Vec2 pos, bool movable, const int frame );
    EntityID spawnGrass     (const Vec2 pos, const int frame);
    EntityID spawnDirt      (const Vec2 pos, const int frame);
    EntityID spawnCampfire  (const Vec2 pos, int layer);
    EntityID spawnWater     (const Vec2 pos, const std::string tag, const int frame );
    EntityID spawnLava      (const Vec2 pos, const std::string tag, const int frame );
    std::vector<EntityID> spawnDualTiles(
        const Vec2 pos, 
        std::unordered_map<std::string, int> tileIndex
    );
    
    void sLoader();
    void sScripting();
    void sMovement();
    void sInteraction();
    void sCollision();
    void sStatus();
    void sAnimation();
    void sRender();
    void sAudio();
    
    void sDoAction(const Action&);
    void onEnd();
    void togglePause();
    void changePlayerStateTo(EntityID entity, PlayerState s);
    
    
    public:
    template<typename T>
    void InitiateScript(CScript& sc, EntityID entityID);
    void InitiateProjectileScript(CScript& sc, EntityID entityID);

    Scene_Play(Game* game, std::string path, bool newGame);
    Vec2 gridSize();
    Vec2 getCameraPosition() override;
    
    void update();
    void setPaused(bool);

    StoryManager& getStoryManager() {
        return m_storyManager;
    }  

    EntityID SpawnFromJSON(std::string name, Vec2 pos);
    EntityID Spawn(std::string name, Vec2 pos);
    EntityID SpawnDialog(std::string dialog, int size, std::string font, EntityID parentID);

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
    EntityID changePlayerID(EntityID otherID);
};
