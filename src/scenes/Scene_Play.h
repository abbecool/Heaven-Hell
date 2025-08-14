#pragma once

#include "ecs/Components.h"
#include "physics/Physics.h"
#include "physics/CollisionManager.h"
#include "story/StoryManager.h"
#include "physics/Camera.h"
#include "scenes/Scene.h"
#include "scenes/Scene_Inventory.h"
#include "scenes/Scene_Pause.h"
#include "physics/Level_Loader.h"

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
    Physics m_physics;
    CollisionManager m_collisionManager;
    InteractionManager m_interactionManager;
    StoryManager m_storyManager;
    float m_zoomStep = 2;
    Vec2 m_currentChunk = Vec2{1, 0};
    Vec2 m_chunkSize = Vec2{12, 12};
    std::vector<Vec2> m_loadedChunks;
    std::vector<EntityID> m_loadedChunkIDs;
    LevelLoader m_levelLoader;
    std::shared_ptr<Scene_Inventory> m_inventory_scene;
    Vec2 m_levelSize;
    bool m_inventoryOpen = false;
    bool m_newGame;
    std::vector<std::vector<std::string>> m_pixelMatrix;
    
    std::unordered_map<std::string, std::unordered_set<std::string>> m_damageToEnemyMap = {
        {"fire", {"grass"}},
        {"water", {"fire"}},
        {"ice", {"water"}},
        {"explosive", {"rock"}},
        {"piercing", {"shielded"}}
    };
    
    void loadLevel(const std::string& path);
    void loadConfig(const std::string& path);
    void loadMobsNItems(const std::string& path);
    
    void saveGame(const std::string& filename);
    
    EntityID spawnPlayer();
    EntityID spawnNPC(Vec2 pos);
    EntityID spawnWeapon(Vec2 pos, int layer);
    EntityID spawnSword(Vec2 pos, int layer);
    EntityID spawnProjectile(EntityID player, Vec2 vel, int layer);
    EntityID spawnCoin(Vec2 pos, const size_t layer);
    EntityID spawnSmallEnemy(Vec2 pos, const size_t layer, std::string type);
    EntityID spawnShadow(EntityID parentID, Vec2 relPos, int size, int layer);
    EntityID spawnDecoration(Vec2 pos, Vec2 collisionBox, const size_t layer, std::string animation);
    
    EntityID spawnObstacle  (const Vec2 pos, bool movable, const int frame );
    EntityID spawnGrass     (const Vec2 pos, const int frame);
    EntityID spawnDirt      (const Vec2 pos, const int frame);
    EntityID spawnCampfire  (const Vec2 pos, int layer);
    EntityID spawnWater     (const Vec2 pos, const std::string tag, const int frame );
    EntityID spawnLava      (const Vec2 pos, const std::string tag, const int frame );
    std::vector<EntityID> spawnDualTiles (const Vec2 pos, std::unordered_map<std::string, int> tileIndex);
    
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
    Vec2 levelSize();
    Vec2 getCameraPosition() override;
    
    void update();
    void setPaused(bool);

    StoryManager& getStoryManager() {
        return m_storyManager;
    }

    EntityID Spawn(std::string name, Vec2 pos);
};
