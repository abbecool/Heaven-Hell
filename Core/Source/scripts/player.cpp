#include "ScriptableEntity.h"
#include <iostream>

class PlayerController : public ScriptableEntity 
{
public:
    void OnCreateFunction()
    {
        // std::cout << "create script entity: OnCreate" << std::endl;
    }

    void OnDestroyFunction()
    {
        // std::cout << "destoy script entity: OnDestroy" << std::endl;
    }

    void OnUpdateFunction()
    {
        auto& transformPlayer = getComponent<CTransform>();
        auto& playerCollider = getComponent<CCollisionBox>();

        auto viewWater = view<CWater>();
        for (auto entity : viewWater) 
        {
            auto& waterTransform = m_ECS->getComponent<CTransform>(entity);
            auto& waterCollider = m_ECS->getComponent<CCollisionBox>(entity);
            
            if (m_physics->isCollided(transformPlayer, playerCollider, waterTransform, waterCollider) )
            {
                if (!hasComponent<CSwimming>()) 
                {
                    addComponent<CSwimming>();
                    getComponent<CAnimation>().animation = m_game->assets().getAnimation("dwarf-sheet");

                }
                return;
            }
        }

        if (hasComponent<CSwimming>()) 
        {
            removeComponent<CSwimming>();
            getComponent<CAnimation>().animation = m_game->assets().getAnimation("demon-sheet");
        }
    }

    void OnAttackFunction()
    {
        EntityID weaponID = m_ECS->getComponent<CWeaponChild>(m_entity.getID()).weaponID;
        m_ECS->getComponent<CScript>(weaponID).Instance->OnAttackFunction();
    }

    void OnCollisionFunction(EntityID colliderID, CollisionMask colliderLayer, Vec2 overlap)
    {
        auto id = m_entity.getID();
        m_ECS->getComponent<CTransform>(id).pos += overlap;
    }
};

