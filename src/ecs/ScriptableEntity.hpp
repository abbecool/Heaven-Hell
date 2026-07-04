#pragma once

#include "core/Game.hpp"
#include "ecs/Entity.hpp"
#include "physics/Physics.hpp"
#include "scenes/Scene_Play.hpp"

#include <memory>
#include <iostream>

class ScriptableEntity{
    public:
        virtual ~ScriptableEntity() {}

        template<typename T, typename... Args>
        T& addComponent(Args&&... args) {
            return m_entity.addComponent<T>(std::forward<Args>(args)...);
        }
        template<typename T>
        T& getComponent(){
            return m_entity.getComponent<T>();
        }
        template<typename T>
        bool hasComponent(){
            return m_entity.hasComponent<T>();
        }
        template<typename T>
        void removeComponent(){
            m_entity.removeComponent<T>();
        }
        template<typename... T>
        auto view(){
            return m_entity.view<T...>();
        }
        template<typename... T>
        const std::vector<EntityID>& viewEntities(){
            return m_entity.viewEntities<T...>();
        }
        void removeEntity() {
            m_entity.removeEntity();
        }
        Entity m_entity;
        ECS* m_ECS;
        Physics* m_physics;
        Game* m_game;
        Scene_Play* m_scene;
        virtual void OnCreateFunction() {}
        virtual void OnDestroyFunction() {}
        virtual void OnUpdateFunction() {}
        virtual void OnAttackFunction() {}
        virtual void OnAttackFunction(EntityID victimID) {}
        virtual void OnCollisionFunction(EntityID colliderID, Vec2 overlap) {}
        virtual void OnInteractFunction(EntityID colliderID, CollisionMask colliderLayer) {}
        virtual void onPosses(EntityID playerID, EntityID otherID) {}

        virtual void showMessage() {}
};
