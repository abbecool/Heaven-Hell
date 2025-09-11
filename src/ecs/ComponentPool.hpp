#pragma once

#include "Components.h"

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
#include <limits>
#include <array>

using EntityID = uint32_t;
static constexpr EntityID tombstone = std::numeric_limits<EntityID>::max();

class BaseComponentPool {
    public:
    std::vector<EntityID> entitiesToRemove;
    std::vector<EntityID> entities;  // Dense vector of IDs
    virtual ~BaseComponentPool() = default;  // Virtual destructor to allow proper deletion
    virtual void removeComponent(EntityID entityId){};
    virtual bool hasComponent(EntityID id) const = 0;
    virtual std::string getTypeName() const = 0;
    const std::vector<EntityID>& getEntities() const {return entities;};
};

static constexpr EntityID MAX_ENTITIES = 8192;  // Define a maximum number of entities 

template<typename T>
class ComponentPool : public BaseComponentPool{
    private:
    
    std::array<EntityID, MAX_ENTITIES> sparse; // Sparse vector of IDs
    std::vector<T> dense;  // Dense vector of components
public:

    ComponentPool() {
        sparse.fill(tombstone);  // Initialize sparse array with tombstone value
    }

    template<typename... Args>
    T& addComponent(EntityID id, Args... args) {
        if (hasComponent(id)){
            removeComponent(id);
        }
        // Sparse set implementation
        const size_t index = dense.size();
        sparse[id] = index;
        dense.emplace_back(std::forward<Args>(args)...);
        entities.push_back(id);
        return dense.back();  // Return a reference to the added component
    }
    
    inline bool hasComponent(EntityID e) const noexcept {
        return sparse[e] != tombstone && sparse[e] < dense.size();
    }
    
    inline T& getComponent(EntityID e) {
        
        EntityID index = sparse[e];
        if (index == tombstone) {
            std::cout << "component type: " << typeid(T).name() << " entity id: " << e << std::endl;
            throw std::out_of_range("Component not found.");
        }
        T& component = dense[index];
        return component;
    }

    T& moveComponent(EntityID dst, EntityID src) {
        assert((sparse[dst] != tombstone) && "src already has a component!");
        sparse[dst] = sparse[src];
        sparse[src] = tombstone;

        return dense[sparse[dst]];  // Return a reference to the moved component
    }
    
    void queueRemoveEntity(EntityID id) {
        if (id == 0)
        {
            std::cerr << "Warning: Attempting to remove player entity." << std::endl;
            return;
        }
        entitiesToRemove.push_back(id);
    }

    void removeComponent(EntityID id) {
        if (id == 0) {
            return;
        }
        EntityID index = sparse[id];
        if (index == tombstone) {
            return;
        }
        EntityID lastId = entities.back();

        std::swap(dense[index], dense.back());  // Swap with the last element
        std::swap(entities[index], entities.back());  // Swap with the last element

        sparse[lastId] = index;  // Update the sparse index for the moved element
        sparse[id] = tombstone;

        dense.pop_back();  // Remove the last element
        entities.pop_back();  // Remove the last ID
    }

    std::vector<T>& getDense() {
        return dense;
    }

    std::string getTypeName() const override {
        return typeid(T).name(); // or a custom string if you prefer
    }

};
