#pragma once

#include <memory>
#include "Components.h"
#include <tuple>
#include <string>

typedef std::tuple<
    CTransform,
    CInputs,
    CBoundingBox,
    CAnimation,
    CTexture,
    CKey,
    CState,
    CHealth,    
    CName,  
    CShadow,    
    CDamage,
    CDialog,
    CPathfind  
> ComponentTuple;
class Entity
{
    friend class EntityManager;

    const std::string   m_tag   = "Default";
    const size_t        m_id    = 0;
    const size_t        m_layer    = 0;
    bool                m_alive = true;
    ComponentTuple      m_components;
public:

    Entity(const std::string& tag, const size_t id, const size_t layer);
    bool isAlive() const;
    const std::string& tag() const;
    const size_t layer() const;
    const size_t id() const;
    const bool movable() const;
    void kill();

    template<typename T>
    bool hasComponent() const {
        return getComponent<T>().has;
    }

    template<typename T, typename... TArgs>
    T& addComponent(TArgs&&... mArgs) {
        auto& component = getComponent<T>();
        component = T(std::forward<TArgs>(mArgs)...);
        component.has = true;
        return component;
    }

    template<typename T>
    T& getComponent() {
        return std::get<T>(m_components);
    }

    template<typename T>
    const T& getComponent() const {
        return std::get<T>(m_components);
    }

    template<typename T>
    void removeComponent() {
        getComponent<T>() = T();
    }

    // std::shared_ptr<CTransform> cTransform;
    // std::shared_ptr<CName> cName;
    // std::shared_ptr<CShape> cShape;
    // std::shared_ptr<CInputs> cInputs;
    // std::shared_ptr<CKey> cKey;
    // std::shared_ptr<CTexture> cTexture;
    // std::shared_ptr<CAnimation> cAnimation;
    // size_t id();
    // const std::string tag();
    // const size_t layer();
    // bool isAlive();
    // void kill();
    void movePosition(Vec2);
    void setScale(Vec2 scale);
    void takeDamage(int damage, size_t frame);

    // void setColor(const int r, const int g, const int b, const int a);
};
