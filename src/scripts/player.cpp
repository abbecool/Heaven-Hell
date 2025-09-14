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
        if (!hasComponent<CEquippedWeapon>()) 
        {
            return; // Player has no weapon
        }
        EntityID weaponID = getComponent<CEquippedWeapon>().weaponID;
        m_ECS->getComponent<CScript>(weaponID).Instance->OnAttackFunction();
    }
};

