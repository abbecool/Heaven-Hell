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
        m_ECS->addComponent<CAudio>(m_entity.getID(), "loot_pickup");
    }
};
