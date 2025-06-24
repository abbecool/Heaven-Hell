#include "ScriptableEntity.h"
#include <iostream>

class CoinController : public ScriptableEntity 
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
        // auto& transformPlayer = getComponent<CTransform>();
        // auto& playerCollider = getComponent<CCollisionBox>();

        // auto viewWater = view<CWater>();
        // for (auto entity : viewWater) 
        // {
        //     auto& waterTransform = m_ECS->getComponent<CTransform>(entity);
        //     auto& waterCollider = m_ECS->getComponent<CCollisionBox>(entity);
            
        //     if (m_physics->isCollided(transformPlayer, playerCollider, waterTransform, waterCollider) )
        //     {
        //         if (!hasComponent<CSwimming>()) 
        //         {
        //             addComponent<CSwimming>();
        //             getComponent<CAnimation>().animation = m_game->assets().getAnimation("dwarf-sheet");

        //         }
        //         return;
        //     }
        // }

        // if (hasComponent<CSwimming>()) 
        // {
        //     removeComponent<CSwimming>();
        //     getComponent<CAnimation>().animation = m_game->assets().getAnimation("demon-sheet");
        // }
    }

    void OnInteractionCollisionFunction(EntityID colliderID, CollisionMask colliderLayer)
    {
        m_ECS->queueRemoveEntity(m_entity.getID());
        Mix_PlayChannel(-1, m_game->assets().getAudio("loot_pickup"), 0);
    }

};

