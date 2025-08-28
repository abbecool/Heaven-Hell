#include "ecs/ScriptableEntity.h"
#include "story/EventBus.h"
#include <iostream>

class HouseController : public ScriptableEntity 
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
        m_scene->Emit({EventType::FlagChanged, "house_entered"});
    }
};
