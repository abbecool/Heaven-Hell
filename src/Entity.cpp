#include "Entity.h"
#include <string>
#include <iostream>
#include <tuple>

Entity::Entity(const std::string& tag, const size_t id, const size_t layer)
        : m_tag(tag), m_id(id), m_layer(layer){
            m_components = std::make_tuple( 
                    CTransform(),
                    CInputs(),
                    CBoundingBox(),
                    CAnimation(),
                    CKey(),
                    CState(),
                    CHealth(),
                    CName(),
                    CShadow(),
                    CDamage(),
                    CDialog(),
                    CPathfind()
                );
        }

const size_t Entity::Entity::id() const{
    return m_id;
}

const std::string & Entity::Entity::tag() const{
    return m_tag;
}

bool Entity::Entity::isTag(const std::string tag) const{
    return m_tag == tag;
}

const size_t Entity::Entity::layer() const{
    return m_layer;
}

bool Entity::Entity::isAlive() const{
    return m_alive;
}

void Entity::Entity::kill(){
    m_alive = false;
}

const bool Entity::Entity::movable() const{
    return getComponent<CTransform>().isMovable;
}

void Entity::Entity::movePosition(Vec2 move){
    getComponent<CTransform>().pos += move;
}

void Entity::Entity::setScale(Vec2 scale){
    getComponent<CTransform>().scale = scale;
}

void Entity::Entity::takeDamage(std::shared_ptr<Entity> attacker, size_t frame){
    auto attack = attacker->getComponent<CDamage>();
    if ((int)frame - getComponent<CHealth>().heart_frames > getComponent<CHealth>().damage_frame && (int)frame - getComponent<CHealth>().heart_frames > attack.lastAttackFrame){
        getComponent<CHealth>().HP = getComponent<CHealth>().HP-attack.damage;
        getComponent<CHealth>().damage_frame = (int)frame;
        attack.lastAttackFrame = (int)frame;
    }
}