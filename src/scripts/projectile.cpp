#include "ecs/ScriptableEntity.h"
#include <iostream>

class ProjectileController : public ScriptableEntity 
{
public:
    void OnCreateFunction()
    {
    }

    void OnDestroyFunction()
    {
        auto& animation     = m_ECS->getComponent<CAnimation>(m_entity.getID());
        animation.animation = m_game->assets().getAnimation("fireball_explode");
        animation.repeat    = false;
        m_ECS->queueRemoveComponent<CVelocity>(m_entity.getID());
        m_ECS->queueRemoveComponent<CCollisionBox>(m_entity.getID());
        m_ECS->queueRemoveComponent<CDamage>(m_entity.getID());
    }

    void OnUpdateFunction()
    {
    }

    void OnAttackFunction(EntityID victimID)
    {
        int damage = m_entity.getComponent<CDamage>().damage;
        m_ECS->getComponent<CHealth>(victimID).HP -= damage;
    }

    void OnCollisionFunction(EntityID colliderID, CollisionMask colliderLayer, Vec2 overlap)
    {
        OnDestroyFunction();
    }
};

