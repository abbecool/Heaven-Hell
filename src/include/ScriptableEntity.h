#pragma once

#include "Entity.h"
#include <memory>
#include <iostream>

class ScriptableEntity{
    public:
        virtual ~ScriptableEntity() {}
        template<typename T>
        T& getComponent(){
            return m_entity->template getComponent<T>();
        }
    protected:
        virtual void OnCreateFunction() {std::cout << "in scriptaable entity" << std::endl;}
        virtual void OnDestroyFunction() {}
        virtual void OnUpdateFunction() {}
    private:
        std::shared_ptr<Entity> m_entity;
        friend class Scene_Play;
};