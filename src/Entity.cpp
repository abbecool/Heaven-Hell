#include "Entity.h"
#include <string>
#include <tuple>

Entity::Entity(const std::string& tag, const size_t id, const size_t layer)
        : m_tag(tag), m_id(id), m_layer(layer){
            m_components = std::make_tuple( 
                    CInputs(),
                    CTransform(),
                    CSize(),
                    CShape(),
                    CTexture(),
                    CName(),
                    CKey(),
                    CAnimation()
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
    // cTransform->pos.x += move.x;
    // cTransform->pos.y += move.y;
    getComponent<CTransform>().pos += move;
}

// void Entity::Entity::setColor(const int r, const int g, const int b, const int a){
//     cShape->r_val = r;
//     cShape->g_val = g;
//     cShape->b_val = b;
//     cShape->a_val = a;
// }
