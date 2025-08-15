#include "ecs/ScriptableEntity.h"
#include <iostream>

class NPCController : public ScriptableEntity 
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
    
    void OnInteractFunction(EntityID playerID, CollisionMask colliderLayer)
    {
        if ( !m_ECS->hasComponent<CInputs>(playerID) )
        {
            return;
        }
        if ( !m_ECS->getComponent<CInputs>(playerID).interact )
        {
            return;
        }
        
        int currentQuestID = m_scene->getStoryManager().getCurrentQuestID();

        std::string dialog;
        if (currentQuestID == 0) {
            m_scene->getStoryManager().setFlag("talked_to_wizard", true);
            dialog = "Go and look for the staff!";
        }
        else if (currentQuestID == 1) {
            dialog = "Have you found the staff yet?";
        }
        else if (currentQuestID == 2) {
            dialog = "Have you found you're home?";
        }
        else
        {
            dialog = "I am a wizard, what are you?";
        }
        if (!hasComponent<CChild>()) {
            m_scene->SpawnDialog(dialog, 16, "Minecraft", m_entity.getID());
        }
    }
};

