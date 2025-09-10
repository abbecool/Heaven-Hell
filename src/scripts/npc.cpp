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
        if ( !m_ECS->hasComponent<CInputs>(playerID))
        {
            return;
        }
        if ( !m_ECS->getComponent<CInputs>(playerID).interact)
        {
            return;
        }
        
        int currentQuestID = m_scene->getStoryManager().getCurrentQuestID();

        std::string dialog;
        if (currentQuestID == 0) {
            m_scene->getStoryManager().setFlag("talked_to_wizard", true);
            std::string name = getComponent<CName>().name;
            m_scene->Emit(Event{EventType::DialogueFinished, name});
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

    void onPosses(EntityID playerID, EntityID otherID)
    {
        if (!m_ECS->hasComponent<CInputs>(playerID))
        {
            return;
        }
        if (!m_ECS->getComponent<CInputs>(playerID).posses)
        {
            return;
        }
        if (!m_ECS->hasComponent<CPossesLevel>(otherID))
        {
            return;
        }

        int possesLevel = m_ECS->getComponent<CPossesLevel>(otherID).level;
        m_ECS->removeComponent<CPossesLevel>(otherID);

        m_scene->changePlayerID(otherID);
        m_ECS->addComponent<CInputs>(otherID);
        // CCollisionBox& collisionBox = m_ECS->getComponent<CCollisionBox>(playerID);
        // m_ECS->addComponent<CCollisionBox>(otherID, collisionBox);
        // m_ECS->copyComponent<CCollisionBox>(otherID, playerID);

        // CInteractionBox& interactionBox = m_ECS->getComponent<CInteractionBox>(playerID);
        // m_ECS->addComponent<CInteractionBox>(otherID, interactionBox);
        

        m_ECS->removeEntity(playerID);
    }
};

