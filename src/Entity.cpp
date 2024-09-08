#include "Entity.h"
#include <string>
#include <iostream>
#include <tuple>
#include <algorithm>

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
                    CPathfind(),
                    CKnockback()
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

bool Entity::Entity::inCamera() const{
    return m_inCamera;
}

void Entity::Entity::setInCamera(bool set){
    m_inCamera = set;
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
        float damageMultiplier = 1.0f;
        if ( !getComponent<CHealth>().HPType.empty() && !attack.damageType.empty() ) 
        {
            auto hpType = getComponent<CHealth>().HPType;

            for (const auto& damageType : attack.damageType) {
                // Check if the damage type is effective
                if (m_effectiveDamageToEnemyMap.find(damageType) != m_effectiveDamageToEnemyMap.end()) {
                    if ( std::any_of(m_effectiveDamageToEnemyMap[damageType].begin(), m_effectiveDamageToEnemyMap[damageType].end(), 
                        [&](const std::string& element) { return hpType.find(element) != hpType.end(); }) ){
                        damageMultiplier *= 2.0f;
                        // std::cout << "Enemy is weak to " << damageType << " damage!" << std::endl;
                    }
                }
                if (m_ineffectiveDamageToEnemyMap.find(damageType) != m_ineffectiveDamageToEnemyMap.end()) {
                    if ( std::any_of(m_ineffectiveDamageToEnemyMap[damageType].begin(), m_ineffectiveDamageToEnemyMap[damageType].end(), 
                        [&](const std::string& element) { return hpType.find(element) != hpType.end(); }) ){
                        damageMultiplier *= 0.5f;
                        // std::cout << "Enemy is resistent to " << damageType << " damage!" << std::endl;
                    }
                }
            }
        }
        getComponent<CHealth>().HP = getComponent<CHealth>().HP-(int)(attack.damage*damageMultiplier);
        getComponent<CHealth>().damage_frame = (int)frame;
        attack.lastAttackFrame = (int)frame;
    }
}