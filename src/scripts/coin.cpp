#include "ecs/ScriptableEntity.h"
#include "story/EventBus.h"
#include <iostream>

class CoinController : public ScriptableEntity 
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

    void OnInteractFunction(EntityID colliderID, CollisionMask colliderLayer)
    {
        m_ECS->queueRemoveEntity(m_entity.getID());
        Mix_PlayChannel(-1, m_game->assets().getAudio("loot_pickup"), 0);
        m_scene->onEvent({EventType::FlagChanged, "house_entered"});
    }

};

