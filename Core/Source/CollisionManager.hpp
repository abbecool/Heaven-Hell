#pragma once
#include <cstdint>
#include <unordered_map>
#include <functional>
#include <utility>
#include "Components.h"

using EntityID = uint32_t;

class CollisionManager {
public:
    using CollisionHandler = std::function<void(EntityID, EntityID)>;
    using LayerPair = std::pair<CollisionLayer, CollisionLayer>;

    CollisionManager() = default;

    void addEntity(EntityID entity, CollisionLayer layer, uint16_t mask) {
        m_collisions[entity] = {layer, mask};
    }

    void removeEntity(EntityID entity) {
        m_collisions.erase(entity);
    }

    void updateEntity(EntityID entity, CollisionLayer layer, uint16_t mask) {
        m_collisions[entity] = {layer, mask};
    }

    bool checkCollision(EntityID entityA, EntityID entityB) const {
        auto itA = m_collisions.find(entityA);
        auto itB = m_collisions.find(entityB);
        if (itA == m_collisions.end() || itB == m_collisions.end()) {
            return false;
        }
        const auto& compA = itA->second;
        const auto& compB = itB->second;
        return (compA.mask & static_cast<uint16_t>(compB.layer)) != 0 &&
               (compB.mask & static_cast<uint16_t>(compA.layer)) != 0;
    }

    // Register a handler for a pair of layers (order-independent)
    void registerHandler(CollisionLayer layerA, CollisionLayer layerB, CollisionHandler handler) {
        auto key = makeOrderedPair(layerA, layerB);
        m_handlers[key] = handler;
    }

    // Call the handler for a collision between two entities, if one exists
    void handleCollision(EntityID entityA, EntityID entityB) {
        auto itA = m_collisions.find(entityA);
        auto itB = m_collisions.find(entityB);
        if (itA == m_collisions.end() || itB == m_collisions.end()) return;
        auto key = makeOrderedPair(itA->second.layer, itB->second.layer);
        auto it = m_handlers.find(key);
        if (it != m_handlers.end()) {
            it->second(entityA, entityB);
        }
    }

    void clear() {
        m_collisions.clear();
        m_handlers.clear();
    }

private:
    std::unordered_map<EntityID, CollisionComponent> m_collisions;
    struct pair_hash {
        std::size_t operator()(const LayerPair& p) const {
            return std::hash<int>()(static_cast<int>(p.first)) ^ std::hash<int>()(static_cast<int>(p.second) << 1);
        }
    };
    std::unordered_map<LayerPair, CollisionHandler, pair_hash> m_handlers;

    // Helper to make an order-independent pair
    static LayerPair makeOrderedPair(CollisionLayer a, CollisionLayer b) {
        return (static_cast<int>(a) < static_cast<int>(b)) ? LayerPair{a, b} : LayerPair{b, a};
    }
};