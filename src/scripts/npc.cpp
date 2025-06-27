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

    void OnInteractFunction()
    {
        // This function can be called when the player interacts with the NPC
        std::cout << "NPC interaction triggered!" << std::endl;
    }
};

