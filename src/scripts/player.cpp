#include "ecs/ScriptableEntity.h"
#include <iostream>

class PlayerController : public ScriptableEntity 
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
        // auto& transformPlayer = getComponent<CTransform>();
        // auto& playerCollider = getComponent<CCollisionBox>();

        // auto viewWater = view<CWater, CTransform, CCollisionBox>();
        // for (auto entity : viewWater) 
        // {
        //     auto& waterTransform = m_ECS->getComponent<CTransform>(entity);
        //     auto& waterCollider = m_ECS->getComponent<CCollisionBox>(entity);
            
        //     if (m_physics->isCollided(transformPlayer, playerCollider, waterTransform, waterCollider) )
        //     {
        //         if (!hasComponent<CSwimming>()) 
        //         {
        //             addComponent<CSwimming>();
        //             getComponent<CAnimation>().animation = m_game->assets().getAnimation("dwarf-sheet");

        //         }
        //         return;
        //     }
        // }

        // if (hasComponent<CSwimming>()) 
        // {
        //     removeComponent<CSwimming>();
        //     getComponent<CAnimation>().animation = m_game->assets().getAnimation("demon-sheet");
        // }
    }

    void OnAttackFunction()
    {
        if (!hasComponent<CWeaponChild>()) 
        {
            return; // Player has no weapon
        }
        EntityID weaponID = getComponent<CWeaponChild>().weaponID;
        m_ECS->getComponent<CScript>(weaponID).Instance->OnAttackFunction();
    }
};

