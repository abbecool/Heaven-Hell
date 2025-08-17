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
        auto& transform = getComponent<CTransform>();
        auto& attack = getComponent<CAttack>();
        Vec2 target = getComponent<CPathfind>().target;
        
        bool targetInRange = abs((target-transform.pos).length()) < (float)attack.range;
        bool attackReady = (attack.attackTimer <= 0);
        if (!targetInRange || !attackReady)
        {
            return;
        }
        EntityID damageAreaID = m_ECS->addEntity();
        Vec2 damageAreaPosition = transform.pos + (target-transform.pos).norm(20);
        SDL_Color damageAreaColor = {0, 0, 255, 255};
        m_ECS->addComponent<CTransform>(damageAreaID, damageAreaPosition);
        m_ECS->addComponent<CCollisionBox>(damageAreaID, attack.area, damageAreaColor);
        m_ECS->addComponent<CDamage>(damageAreaID, attack.damage);
        m_ECS->addComponent<CLifespan>(damageAreaID, attack.duration);

        attack.attackTimer = attack.speed;
        attack.attackTimer--;
    }

    void OnCollisionFunction(EntityID colliderID, Vec2 overlap)
    {
        auto id = m_entity.getID();
        m_ECS->getComponent<CTransform>(id).pos += overlap;
    }
};

