#pragma once
#include <unordered_map>

#include "Types.hpp"

class BasicComponentPool {
public:
    virtual ~BasicComponentPool() = default;  // Virtual destructor to allow proper deletion
	virtual void EntityDestroyed(EntityID entity) = 0;
};

template<typename T>
class ComponentPool_new : public BasicComponentPool{
public:
    // Add a component to the pool for a specific entity
    // template<typename... Args>
    // T& addComponent(EntityID entityId, Args... args) {
    //     auto& component = pool.emplace(entityId, T(std::forward<Args>(args)...)).first->second;
    //     return component;
    // }

    // // Remove a component from the pool
    // void removeComponent(EntityID entityId) {
    //     pool.erase(entityId);  // Remove the component if it exists
    // }

    // // Check if an entityId has the component
    // bool hasComponent(EntityID entityId) const {
    //     return pool.find(entityId) != pool.end();  // Check if the entityId is in the pool
    // }

    // // Retrieve the component for an entityId
    // T& getComponent(EntityID entityId) {
    //     return pool.at(entityId);  // Returns a reference to the component
    // }
private:
	// The packed array of components (of generic type T),
	// set to a specified maximum amount, matching the maximum number
	// of entities allowed to exist simultaneously, so that each entity
	// has a unique spot.
	std::array<T, MAX_ENTITIES> mComponentArray;

	// Map from an entity ID to an array index.
	std::unordered_map<EntityID, size_t> mEntityToIndexMap;

	// Map from an array index to an entity ID.
	std::unordered_map<size_t, EntityID> mIndexToEntityMap;

	// Total size of valid entries in the array.
	size_t mSize;
};