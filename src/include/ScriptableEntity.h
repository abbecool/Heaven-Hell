#pragma once

#include "Game.h"
#include "Entity.h"
#include "Physics.h"
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
        template<typename T>
        std::vector<EntityID> view(){
            return m_entity.view<T>();
        }
        void removeEntity() {
            m_entity.removeEntity();
        }
        Entity m_entity; // private
        ECS* m_ECS;
        Physics* m_physics;
        Game* m_game;
        Scene* m_scene;
        virtual void OnCreateFunction() {} //protected
        virtual void OnDestroyFunction() {} //protected
        virtual void OnUpdateFunction() {} //protected
        virtual void OnAttackFunction() {}
        virtual void OnCollisionFunction(EntityID colliderID, CollisionMask colliderLayer, Vec2 overlap) {} //protected
        virtual void OnInteractionCollisionFunction(EntityID colliderID, CollisionMask colliderLayer) {} //protected
        virtual void OnInteractFunction() {} //protected
    // protected:
    private:
        // friend class Scene;
};