#include "ecs/ScriptableEntity.h"
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
            // std::cout << "Player entity does not have a input component." << std::endl;
            return;
        }
        if ( !m_ECS->getComponent<CInputs>(playerID).interact )
        {
            // std::cout << "Player entity is not interacting." << std::endl;
            return;
        }
        
        int currentQuestID = m_scene->getStoryManager().getCurrentQuestID();

        if (currentQuestID == 0) {
            m_scene->getStoryManager().setFlag("talked_to_wizard", true);
            std::cout << "Go and look for the staff!" << std::endl;
        }
        else if (currentQuestID == 1) {
            std::cout << "Have you found the staff yet?" << std::endl;
        }
        else if (currentQuestID == 2) {
            std::cout << "Have you found you're home?" << std::endl;
        }
        else
        {
            std::cout << "I am a wizard, what are you?" << std::endl;
        }
    }
};

