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
        virtual void OnCreateFunction() {} //protected
        virtual void OnDestroyFunction() {} //protected
        virtual void OnUpdateFunction() {} //protected
    // protected:
    private:
        friend class Scene;
};