// #pragma once
// #include <unordered_map>

// #include "Types.hpp"
// #include <array>
// #include <cassert>
// #include <unordered_map>

// class BasicComponentArray {
// public:
//     virtual ~BasicComponentArray() = default;  // Virtual destructor to allow proper deletion
// 	virtual void EntityRemoved(EntityID entity) = 0;
// };

// template<typename T>
// class ComponentArray : public BasicComponentArray{
// public:
//     // Add a component to the pool for a specific entity
//     // template<typename... Args>
//     // T& addComponent(EntityID entityId, Args... args) {
//     //     auto& component = pool.emplace(entityId, T(std::forward<Args>(args)...)).first->second;
//     //     return component;
//     // }

//     // // Remove a component from the pool
//     // void removeComponent(EntityID entityId) {
//     //     pool.erase(entityId);  // Remove the component if it exists
//     // }

//     // // Check if an entityId has the component
//     // bool hasComponent(EntityID entityId) const {
//     //     return pool.find(entityId) != pool.end();  // Check if the entityId is in the pool
//     // }

//     // // Retrieve the component for an entityId
//     // T& getComponent(EntityID entityId) {
//     //     return pool.at(entityId);  // Returns a reference to the component
//     // }
//     void InsertData(Entity entity, T component)
// 	{
// 		assert(mEntityToIndexMap.find(entity) == mEntityToIndexMap.end() && "Component added to same entity more than once.");

// 		// Put new entry at end
// 		size_t newIndex = mSize;
// 		mEntityToIndexMap[entity] = newIndex;
// 		mIndexToEntityMap[newIndex] = entity;
// 		mComponentArray[newIndex] = component;
// 		++mSize;
// 	}

// 	void RemoveData(Entity entity)
// 	{
// 		assert(mEntityToIndexMap.find(entity) != mEntityToIndexMap.end() && "Removing non-existent component.");

// 		// Copy element at end into deleted element's place to maintain density
// 		size_t indexOfRemovedEntity = mEntityToIndexMap[entity];
// 		size_t indexOfLastElement = mSize - 1;
// 		mComponentArray[indexOfRemovedEntity] = mComponentArray[indexOfLastElement];

// 		// Update map to point to moved spot
// 		Entity entityOfLastElement = mIndexToEntityMap[indexOfLastElement];
// 		mEntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
// 		mIndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

// 		mEntityToIndexMap.erase(entity);
// 		mIndexToEntityMap.erase(indexOfLastElement);

// 		--mSize;
// 	}

// 	T& GetData(Entity entity)
// 	{
// 		assert(mEntityToIndexMap.find(entity) != mEntityToIndexMap.end() && "Retrieving non-existent component.");

// 		return mComponentArray[mEntityToIndexMap[entity]];
// 	}

// 	void EntityDestroyed(Entity entity) override
// 	{
// 		if (mEntityToIndexMap.find(entity) != mEntityToIndexMap.end())
// 		{
// 			RemoveData(entity);
// 		}
// 	}

// private:
// 	std::array<T, MAX_ENTITIES> mComponentArray;
// 	std::unordered_map<EntityID, size_t> mEntityToIndexMap;
// 	std::unordered_map<size_t, EntityID> mIndexToEntityMap;
// 	size_t mSize;
// };