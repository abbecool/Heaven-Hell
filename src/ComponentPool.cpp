#include <unordered_map>
#include <typeindex>
#include <memory>
#include <iostream>

// Define a pool to store components for a specific type
template<typename Component>
class ComponentPool {
public:
    // Add a component to the pool for a specific entity
    void addComponent(EntityID entity, Component component) {
        pool[entity] = std::move(component);  // Add or replace the component for the entity
    }

    // Remove a component from the pool
    void removeComponent(EntityID entity) {
        pool.erase(entity);  // Remove the component if it exists
    }

    // Check if an entity has the component
    bool hasComponent(EntityID entity) const {
        return pool.find(entity) != pool.end();  // Check if the entity is in the pool
    }

    // Retrieve the component for an entity
    Component& getComponent(EntityID entity) {
        return pool.at(entity);  // Returns a reference to the component
    }

private:
    std::unordered_map<EntityID, Component> pool;  // Map of components indexed by EntityID
};
