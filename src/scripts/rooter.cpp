#include "ecs/ScriptableEntity.h"
#include <iostream>

class RooterController : public ScriptableEntity 
{
public:
    void OnCreateFunction()
    {
    }

    void OnDestroyFunction()
    {
    }

    void OnUpdateFunction()
    {
        OnAttackFunction();
    }

    void OnAttackFunction()
    {

        // if can attack and sees enemy
        // spawn damage area
        
        auto& transform = getComponent<CTransform>();
        auto& attack = getComponent<CAttack>();
        attack.attackTimer--;
        
        Vec2 target = getComponent<CPathfind>().target;
        if ( abs((target-transform.pos).length()) < (float)attack.range && (attack.attackTimer <= 0))
        {
            // child parent relation removed since code does not support multiple children per parent
            EntityID damageAreaID = m_ECS->addEntity();
            // addComponent<CChild>(damageAreaID, true);
            // m_ECS->addComponent<CParent>(damageAreaID, m_entity.getID(), (target-transform.pos).norm(64/4));
            m_ECS->addComponent<CTransform>(damageAreaID, transform.pos + (target-transform.pos).norm(20));
            m_ECS->addComponent<CCollisionBox>(damageAreaID, attack.area, 0, 0, 255);
            m_ECS->addComponent<CDamage>(damageAreaID, attack.damage);
            m_ECS->addComponent<CLifespan>(damageAreaID, attack.duration);
            attack.attackTimer = attack.speed;
        }
    }

    void OnCollisionFunction(EntityID colliderID, CollisionMask colliderLayer, Vec2 overlap)
    {
        auto id = m_entity.getID();
        m_ECS->getComponent<CTransform>(id).pos += overlap;
    }
};

