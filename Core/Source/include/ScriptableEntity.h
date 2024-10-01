#pragma once

// #include "Entity.h"
#include <memory>
#include <iostream>

class ScriptableEntity{
    public:
        virtual ~ScriptableEntity() {}
        // template<typename T>
        // T& getComponent(){
        //     return m_entity.getComponent<T>();
        // }
    protected:
        virtual void OnCreateFunction() {}
        virtual void OnDestroyFunction() {}
        virtual void OnUpdateFunction() {}
    private:
        // Entity m_entity();
        friend class Scene;
};