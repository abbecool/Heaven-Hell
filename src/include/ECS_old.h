// #pragma once

// #include <iostream>

// #include <map>
// #include <vector>
// #include <algorithm>

// #include <memory>
// #include <unordered_set>
// #include <typeindex>
// // #include <functional>

// typedef uint32_t EntityID;

// class ECS {
// public:
//     // Create a new entity
//     EntityID createEntity() {
//         return ECS.createEntity();
//     }

//     // Add a component to an entity
//     template <typename Component>
//     void addComponent(EntityID entity, Component component) {
//         getOrCreateComponentPool<Component>().addComponent(entity, std::move(component));
//     }

//     // Remove a component from an entity
//     template <typename Component>
//     void removeComponent(EntityID entity) {
//         getOrCreateComponentPool<Component>().removeComponent(entity);
//     }

//     // Check if an entity has a component
//     template <typename Component>
//     bool hasComponent(EntityID entity) const {
//         return getComponentPool<Component>().hasComponent(entity);
//     }

//     // Get a component from an entity
//     template <typename Component>
//     Component& getComponent(EntityID entity) {
//         return getComponentPool<Component>().getComponent(entity);
//     }

// private:

//     // Map to store component pools for each component type
//     std::unordered_map<std::type_index, std::unique_ptr<void>> componentPools;

//     // Helper to get or create the component pool for a specific type
//     template <typename Component>
//     ComponentPool<Component>& getOrCreateComponentPool() {
//         std::type_index typeIdx(typeid(Component));
//         if (componentPools.find(typeIdx) == componentPools.end()) {
//             componentPools[typeIdx] = std::make_unique<ComponentPool<Component>>();
//         }
//         return *reinterpret_cast<ComponentPool<Component>*>(componentPools[typeIdx].get());
//     }

//     // Helper to get the component pool for a specific type (const version)
//     template <typename Component>
//     const ComponentPool<Component>& getComponentPool() const {
//         std::type_index typeIdx(typeid(Component));
//         return *reinterpret_cast<const ComponentPool<Component>*>(componentPools.at(typeIdx).get());
//     }
// };
