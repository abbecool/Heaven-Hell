#pragma once

#include "Entity.h"

class ScriptableEntity{
    public:
        template<typename T>
        T& getComponent(){
            return m_entity.getComponent<T>();
        }
        template<typename T>
        T& hasComponent(){
            return m_entity.hasComponent<T>();
        }
        
    private:
        Entity m_entity;
};