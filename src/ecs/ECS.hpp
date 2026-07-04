#pragma once

#include "ecs/Components.hpp"
#include "ComponentPool.hpp"

#include <iostream>
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>
#include <memory>
#include <unordered_set>
#include <typeindex>
#include <functional>
#include <cassert>
#include <bitset>
#include <cstdint>
#include <tuple>
#include <algorithm>
#include <limits>
#include <utility>

using EntityID = uint32_t;

static bool comp(int a, int b) {
    return a > b;
}

template<typename... Components>
class ECSView {
    static_assert(sizeof...(Components) > 0, "View requires at least one component type.");

    using PoolTuple = std::tuple<ComponentPool<Components>*...>;

public:
    explicit ECSView(ComponentPool<Components>*... pools)
        : m_pools(pools...)
    {
        if ((pools && ...)) {
            (selectDriverPool(pools), ...);
        }
    }

    class Iterator {
    public:
        Iterator(const ECSView* view, size_t index)
            : m_view(view), m_index(index)
        {
            advanceToMatch();
        }

        auto operator*() const {
            const EntityID entity = m_view->entityAt(m_index);
            return std::tuple<EntityID, Components&...>{
                entity,
                std::get<ComponentPool<Components>*>(m_view->m_pools)->getComponent(entity)...
            };
        }

        Iterator& operator++() {
            ++m_index;
            advanceToMatch();
            return *this;
        }

        bool operator==(const Iterator& other) const {
            return m_view == other.m_view && m_index == other.m_index;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        void advanceToMatch() {
            while (m_view && m_index < m_view->driverSize() && !m_view->hasAllComponents(m_view->entityAt(m_index))) {
                ++m_index;
            }
        }

        const ECSView* m_view = nullptr;
        size_t m_index = 0;
    };

    Iterator begin() const {
        return Iterator(this, 0);
    }

    Iterator end() const {
        return Iterator(this, driverSize());
    }

private:
    void selectDriverPool(BaseComponentPool* pool) {
        const auto& entities = pool->getEntities();
        if (!m_driverEntities || entities.size() < m_driverEntities->size()) {
            m_driverEntities = &entities;
        }
    }

    size_t driverSize() const {
        return m_driverEntities ? m_driverEntities->size() : 0;
    }

    EntityID entityAt(size_t index) const {
        return (*m_driverEntities)[index];
    }

    bool hasAllComponents(EntityID entity) const {
        return (std::get<ComponentPool<Components>*>(m_pools)->hasComponent(entity) && ...);
    }

    PoolTuple m_pools;
    const std::vector<EntityID>* m_driverEntities = nullptr;
};

class ECS
{
private:
    friend class Entity;
    std::vector<EntityID> m_freeIDs;
    std::vector<EntityID> m_usedIDs;
    
    std::vector<EntityID> m_dense;
    std::vector<EntityID> m_sparse;
    
    // Map to store component pools for each component type
    std::unordered_map<std::type_index, std::unique_ptr<BaseComponentPool>> m_componentPools;
    std::vector<EntityID> m_entitiesToRemove;
    std::vector<EntityID> matchingEntities;

    template <typename T>
    ComponentPool<T>* findComponentPool() {
        std::type_index typeIdx(typeid(T));
        auto it = m_componentPools.find(typeIdx);
        if (it == m_componentPools.end()) {
            return nullptr;
        }
        return reinterpret_cast<ComponentPool<T>*>(it->second.get());
    }
public:

    ECS() {
        m_sparse.resize(MAX_ENTITIES); // reserve sparse array
    }
    
    std::string numberOfEntities(){
        std::string message = std::to_string(m_dense.size());
        for (auto& [type, pool]: m_componentPools) {
            if (pool != nullptr) {
                message += ", " + std::to_string(pool->getLength());
            }
        }
        return message;
    }

    EntityID addEntity() {
        EntityID id;
        if (!m_freeIDs.empty()) {
            id = m_freeIDs.back();
            m_freeIDs.pop_back();
        } else {
            id = m_dense.size(); // next sequential ID
        }

        if (id >= m_sparse.size()) { // expand sparse array if needed
            m_sparse.resize(id + 1);
        }

        m_sparse[id] = m_dense.size(); // store index in dense array
        m_dense.push_back(id);
        return id;
    }

    void removeEntity(EntityID entity) {
        assert(entity < m_sparse.size() && "Invalid entity ID!");
        if (!isAlive(entity)){return;}

        size_t index = m_sparse[entity];         // where entity lives in dense
        size_t lastIndex = m_dense.size() - 1;   // last element
        EntityID lastID = m_dense[lastIndex];

        // Swap with last element
        m_dense[index] = lastID;
        m_sparse[lastID] = index;                // update moved element

        m_dense.pop_back();                       // remove last
        m_freeIDs.push_back(entity);             // recycle ID

        for (auto& [type, pool]: m_componentPools) {
            if (pool != nullptr) {
                pool->removeComponent(entity);
            }
        }
    }

    bool isAlive(EntityID entity) const {
        if (entity >= m_sparse.size()) return false;
        size_t index = m_sparse[entity];
        return index < m_dense.size() && m_dense[index] == entity;
    }

    void attachChild(EntityID parent, EntityID child, Vec2 relativePos, bool removeOnDeath = true) {
        if (!isAlive(parent) || !isAlive(child)) {
            return;
        }

        if (hasComponent<CParent>(child)) {
            detachChild(child);
        }
        addComponent<CParent>(child, parent, relativePos);

        if (!hasComponent<CChild>(parent)) {
            addComponent<CChild>(parent, child, removeOnDeath);
            return;
        }

        auto& children = getComponent<CChild>(parent).children;
        for (auto& childLink : children) {
            if (childLink.child == child) {
                childLink.removeOnDeath = removeOnDeath;
                return;
            }
        }
        children.emplace_back(child, removeOnDeath);
    }

    void detachChild(EntityID child) {
        if (!isAlive(child) || !hasComponent<CParent>(child)) {
            return;
        }

        const EntityID parent = getComponent<CParent>(child).parent;
        if (isAlive(parent) && hasComponent<CChild>(parent)) {
            auto& children = getComponent<CChild>(parent).children;
            children.erase(
                std::remove_if(
                    children.begin(),
                    children.end(),
                    [child](const auto& childLink) {
                        return childLink.child == child;
                    }
                ),
                children.end()
            );
        }

        removeComponent<CParent>(child);
    }

    void queueRemoveEntity(EntityID entity) {
        if (entity == 0) {
            std::cerr << "Error: Attempting to remove player entity!" << std::endl;
            return;
        }
        if (hasComponent<CParent>(entity))
        {
            detachChild(entity);
        }
        if (hasComponent<CChild>(entity))
        {
            const auto children = getComponent<CChild>(entity).children;
            for (const auto& childLink : children) {
                if (!isAlive(childLink.child)) {
                    continue;
                }
                if (childLink.removeOnDeath) {
                    queueRemoveEntity(childLink.child);
                }
                else if (hasComponent<CParent>(childLink.child)) {
                    removeComponent<CParent>(childLink.child);
                }
            }
            getComponent<CChild>(entity).children.clear();
        }
        m_entitiesToRemove.push_back(entity);
    }

    template<typename T>
    void queueRemoveComponent(EntityID entity) {
        auto& pool = getComponentPool<T>();
        pool.queueRemoveEntity(entity);
    }

    void update(){
        for (auto e : m_entitiesToRemove) {
            removeEntity(e);
        }
        m_entitiesToRemove.clear();

        for (auto& [type, pool] : m_componentPools) {
            if (pool == nullptr) continue;
            
            auto& entitiesToRemove = pool->entitiesToRemove;
            for (auto e : entitiesToRemove) {
                pool->removeComponent(e);
            }
            entitiesToRemove.clear();
        }
        // std::cout << numberOfEntities() << "\r";
    }
        
    // Add a component to an entity
    template<typename T, typename... Args>
    T& addComponent(EntityID entity, Args &&... args) {
        auto& pool = getOrCreateComponentPool<T>();
        return pool.addComponent(entity, std::forward<Args>(args)...);
    };
    
    // Remove a component from an entityId
    template <typename T>
    void removeComponent(EntityID entityId) {
        getComponentPool<T>().removeComponent(entityId);
    }
    
    // Check if an entity has a component
    template <typename T>
    bool hasComponent(EntityID entityId) {
        return getOrCreateComponentPool<T>().hasComponent(entityId);
    }
    
    // Get a component from an entity
    template <typename T>
    T& getComponent(EntityID entityId) {
        return getComponentPool<T>().getComponent(entityId);
    }
    
    // make a copy of a entities component and add it to another entity
    template<typename T>
    T& copyComponent(EntityID dst, EntityID src) {
        const T& component = getComponent<T>(src);
        return addComponent<T>(dst, component);
    }

    // move a component from one entity to another
    template<typename T>
    T& moveComponent(EntityID dst, EntityID src) {
        auto& pool = getOrCreateComponentPool<T>();
        return pool.moveComponent(dst, src);
    }
    
    template <typename T>
    ComponentPool<T>& getComponentPool() {
        std::type_index typeIdx(typeid(T));
        auto it = m_componentPools.find(typeIdx);
        if (it != m_componentPools.end()) {
            return *reinterpret_cast<ComponentPool<T>*>(it->second.get());
        } else {
            static ComponentPool<T> emptyPool;
            return emptyPool;
            std::cout << typeid(T).name() << " pool doesnt exist." << std::endl;
        }
    }
    
    template <typename T>
    ComponentPool<T>& getOrCreateComponentPool() {
        std::type_index typeIdx(typeid(T));
        if (m_componentPools.find(typeIdx) == m_componentPools.end()) {
            m_componentPools[typeIdx] = std::make_unique<ComponentPool<T>>();
        }
        return *reinterpret_cast<ComponentPool<T>*>(m_componentPools[typeIdx].get());
    }
    
    template<typename... Components>
    ECSView<Components...> View()
    {
        return ECSView<Components...>(findComponentPool<Components>()...);
    }

    template<typename... Components>
    const std::vector<EntityID>& ViewEntities()
    {
        BaseComponentPool* smallestPoolBase = nullptr;
        size_t minSize = std::numeric_limits<size_t>::max();

        ((
            [&] {
                auto& pool = getOrCreateComponentPool<Components>();
                // dense is the vector<T> in your sparse set
                size_t sz = pool.getDense().size();
                if (sz < minSize) {
                    minSize = sz;
                    smallestPoolBase = &pool;
                }
            }()
        ), ...);
        
        matchingEntities.clear();
        if (!smallestPoolBase) return matchingEntities;

        matchingEntities.reserve(minSize);

        for (EntityID e : smallestPoolBase->getEntities()) {
            if ((getOrCreateComponentPool<Components>().hasComponent(e) && ...)) {
                matchingEntities.push_back(e);
            }
        }
        return matchingEntities;
    }

    void printEntityComponents(EntityID entity) {
        if (hasComponent<CName>(entity)){
            std::cout << getComponent<CName>(entity).name << "'s components:" << std::endl;
        }
        for (auto& [typeId, poolBase] : m_componentPools) {
            if (poolBase->hasComponent(entity)) {
                std::cout << "- " << poolBase->getTypeName() << "\n";
            }
        }
    }
};
