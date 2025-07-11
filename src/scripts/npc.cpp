#include "ScriptableEntity.h"
#include <iostream>

class NPCController : public ScriptableEntity 
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
        
    }
    
    void OnInteractFunction(EntityID playerID, CollisionMask colliderLayer)
    {
        if ( !m_ECS->hasComponent<CInputs>(playerID) )
        {
            std::cout << "Player entity does not have a input component." << std::endl;
            return;
        }
        // if ( !m_ECS->getComponent<CInputs>(playerID).interact )
        // {
        //     std::cout << "Player entity is not interacting." << std::endl;
        //     return;
        // }
        std::cout << "NPC interaction triggered!" << std::endl;
        showMessage();
    }

    void showMessage()
    {
        m_scene->showDialog(m_entity.getID())
    }
};

