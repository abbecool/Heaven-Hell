#pragma once

#include "Entity.h"
#include <memory>
#include <iostream>

class ScriptableEntity{
    public:
        virtual ~ScriptableEntity() {}
        template<typename T>
        T& getComponent(){
            return m_entity.getComponent<T>();
        }
        Entity m_entity; // private
        ECS* m_ECS;
        virtual void OnCreateFunction() {} //protected
        virtual void OnDestroyFunction() {} //protected
        virtual void OnUpdateFunction() {} //protected
        virtual void OnAttackFunction() {}
        virtual void OnCollisionFunction() {} //protected
    // protected:
    private:
        friend class Scene;
};