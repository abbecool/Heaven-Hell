#pragma once

#include "Components.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

using EntityID = uint32_t;
static constexpr std::size_t INVALID_DENSE_INDEX = std::numeric_limits<std::size_t>::max();
static constexpr std::size_t INITIAL_ENTITY_CAPACITY = 8192;

class BaseComponentPool {
public:
    std::vector<EntityID> entitiesToRemove;
    std::vector<EntityID> denseEntities;  // Dense vector of IDs
    virtual ~BaseComponentPool() = default;  // Virtual destructor to allow proper deletion
    virtual void removeComponent(EntityID entityId) = 0;
    virtual bool hasComponent(EntityID id) const = 0;
    virtual std::string getTypeName() const = 0;
    const std::vector<EntityID>& getEntities() const { return denseEntities; }
    virtual std::size_t getLength() const = 0;
};

template<typename T>
class ComponentPool : public BaseComponentPool {
private:
    std::vector<std::size_t> sparse;
    std::vector<T> dense;  // Dense vector of components

    void ensureEntityCapacity(EntityID id) {
        if (id < sparse.size()) {
            return;
        }

        const std::size_t requiredSize = static_cast<std::size_t>(id) + 1;
        const std::size_t grownSize = std::max(requiredSize, sparse.size() * 2);
        sparse.resize(grownSize, INVALID_DENSE_INDEX);
    }

public:

    ComponentPool()
        : sparse(INITIAL_ENTITY_CAPACITY, INVALID_DENSE_INDEX) {}

    std::size_t getLength() const override {
        return dense.size();
    }

    template<typename... Args>
    T& addComponent(EntityID id, Args&&... args) {
        ensureEntityCapacity(id);
        if (hasComponent(id)){
            removeComponent(id);
        }
        // Sparse set implementation
        const std::size_t index = dense.size();
        sparse[id] = index;
        dense.emplace_back(std::forward<Args>(args)...);
        denseEntities.push_back(id);
        return dense.back();  // Return a reference to the added component
    }
    
    inline bool hasComponent(EntityID e) const noexcept {
        if (e >= sparse.size()) {
            return false;
        }
        return sparse[e] != INVALID_DENSE_INDEX && sparse[e] < dense.size();
    }
    
    inline T& getComponent(EntityID e) {
        
        if (!hasComponent(e)) {
            std::cout << "component type: " << typeid(T).name() << " entity id: " << e << std::endl;
            throw std::out_of_range("Component not found.");
        }
        const std::size_t index = sparse[e];
        T& component = dense[index];
        return component;
    }

    inline const T& getComponent(EntityID e) const {

        if (!hasComponent(e)) {
            std::cout << "component type: " << typeid(T).name() << " entity id: " << e << std::endl;
            throw std::out_of_range("Component not found.");
        }
        const std::size_t index = sparse[e];
        const T& component = dense[index];
        return component;
    }

    T& moveComponent(EntityID dst, EntityID src) {
        if (dst == src) {
            return getComponent(src);
        }
        if (!hasComponent(src)) {
            throw std::out_of_range("Source component not found.");
        }
        ensureEntityCapacity(dst);
        if (hasComponent(dst)) {
            removeComponent(dst);
        }

        const std::size_t index = sparse[src];
        sparse[dst] = index;
        sparse[src] = INVALID_DENSE_INDEX;
        denseEntities[index] = dst;

        return dense[index];
    }
    
    void queueRemoveEntity(EntityID id) {
        entitiesToRemove.push_back(id);
    }

    void removeComponent(EntityID id) override {
        if (!hasComponent(id)) {
            return;
        }
        const std::size_t index = sparse[id];
        const EntityID lastId = denseEntities.back();

        std::swap(dense[index], dense.back());  // Swap with the last element
        std::swap(denseEntities[index], denseEntities.back());  // Swap with the last element

        sparse[lastId] = index;  // Update the sparse index for the moved element
        sparse[id] = INVALID_DENSE_INDEX;

        dense.pop_back();  // Remove the last element
        denseEntities.pop_back();  // Remove the last ID
    }

    std::vector<T>& getDense() {
        return dense;
    }

    const std::vector<T>& getDense() const {
        return dense;
    }

    std::string getTypeName() const override {
        return typeid(T).name(); // or a custom string if you prefer
    }

};
