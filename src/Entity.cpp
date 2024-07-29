#include "Entity.h"
#include <string>
#include <tuple>

Entity::Entity(const std::string& tag, const size_t id, const size_t layer)
        : m_tag(tag), m_id(id), m_layer(layer){
            m_components = std::make_tuple( 
                    CTransform(),
                    CInputs(),
                    CBoundingBox(),
                    CAnimation(),
                    CTexture(),
                    CKey(),
                    CState()
                );
        }

const size_t Entity::Entity::id() const{
    return m_id;
}

const std::string & Entity::Entity::tag() const{
    return m_tag;
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

void Entity::Entity::movePosition(Vec2 move){
    getComponent<CTransform>().pos += move;
}