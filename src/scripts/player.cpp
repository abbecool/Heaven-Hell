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

