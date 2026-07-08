#pragma once

#include <array>
#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

#include "ecs/Components.hpp"
#include "ecs/ECS.hpp"
#include "ecs/Entity.hpp"
#include "physics/Quadtree.hpp"
#include "render/RenderBackend.hpp"

class Scene_Play;

using Handler = std::function<void(Entity, Entity, Vec2)>;
using CollisionMatrix = std::array<std::array<Handler, MAX_LAYERS>, MAX_LAYERS>;

struct ColliderProxy
{
    EntityID entity = 0;
    size_t shapeIndex = 0;
    Vec2 center = {0, 0};
    Vec2 prevCenter = {0, 0};
    Vec2 size = {0, 0};
    Vec2 halfSize = {0, 0};
    CollisionMask layer = EMPTY_MASK;
    CollisionMask targetMask = EMPTY_MASK;
    bool isTrigger = false;
    bool isStatic = false;
};

class CollisionManager
{
private:
    ECS* m_ECS = nullptr;
    Scene_Play* m_scene = nullptr;
    CollisionMatrix m_solidHandlers{};
    CollisionMatrix m_triggerHandlers{};
    std::unique_ptr<Quadtree> m_quadRoot;
    std::unique_ptr<Quadtree> m_staticQuadRoot;
    Vec2 m_worldCenter = {0, 0};
    Vec2 m_worldSize = {0, 0};
    std::vector<ColliderProxy> m_proxies;
    std::vector<ColliderProxy> m_staticProxies;
    std::unordered_set<uint64_t> m_processedShapePairs;
    std::unordered_set<uint64_t> m_processedTriggerPairs;

    static uint8_t layerIndex(CollisionMask layer);
    static uint64_t pairKey(size_t first, size_t second);
    static uint64_t triggerPairKey(const ColliderProxy& first, const ColliderProxy& second);
    static bool layersMatch(const ColliderProxy& first, const ColliderProxy& second);
    static bool aabbIntersects(const ColliderProxy& first, const ColliderProxy& second);

    bool hasValidWorldBounds() const;
    void insertColliderProxy(
        EntityID entityID,
        size_t shapeIndex,
        const ColliderShape& shape,
        const CTransform& transform,
        bool isStatic,
        std::vector<ColliderProxy>& proxies,
        Quadtree& tree
    );
    Vec2 calculateDelta(Vec2 aPos, Vec2 aSize, Vec2 bPos, Vec2 bSize) const;
    Vec2 calculateHorizontalMovement(
        const Vec2& aPos,
        const Vec2& aSize,
        const Vec2& bPos,
        const Vec2& bSize,
        const Vec2& overlap,
        const Vec2& prevOverlap
    ) const;
    Vec2 calculateVerticalMovement(
        const Vec2& aPos,
        const Vec2& aSize,
        const Vec2& bPos,
        const Vec2& bSize,
        const Vec2& overlap,
        const Vec2& prevOverlap
    ) const;
    Vec2 collisionOverlap(const ColliderProxy& first, const ColliderProxy& second) const;

    void registerSolidHandler(CollisionMask layerA, CollisionMask layerB, Handler handler);
    void registerTriggerHandler(CollisionMask layerA, CollisionMask layerB, Handler handler);
    void dispatch(
        CollisionMatrix& matrix,
        EntityID entityA,
        CollisionMask layerA,
        EntityID entityB,
        CollisionMask layerB,
        Vec2 overlap
    );
    void buildQuadtree();
    void processQuadtreeLeaf(const std::vector<size_t>& proxyIndices);

    bool talkToNPC(Entity player, Entity friendly);
    bool possesNPC(Entity player, Entity friendly);
    bool addItemToInventory(Entity player, const Item& item);
    void showLootLabel(Entity loot, const std::string& name);
    void handleDamageHitbox(Entity entityA, Entity entityB, Vec2 overlap);
    void handlePlayerFriendly(Entity player, Entity friendly, Vec2 overlap);
    void handlePlayerLoot(Entity player, Entity loot, Vec2 overlap);
    void handlePlayerArea(Entity player, Entity area, Vec2 overlap);
    void handleMobWater(Entity mob, Entity water, Vec2 overlap);

public:
    CollisionManager() = default;
    CollisionManager(ECS* ecs, Scene_Play* scene);

    void setWorldBounds(Vec2 center, Vec2 size);
    void rebuildStaticQuadtree();
    void doCollisions();
    void renderQuadtree(RenderBackend& renderer);
    const std::vector<ColliderProxy>& proxies() const { return m_proxies; }
};
