#include "ScriptableEntity.h"
#include <iostream>

class PlayerController : public ScriptableEntity 
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
        auto& transformPlayer = getComponent<CTransform>();
        auto& playerCollider = getComponent<CCollisionBox>();

        auto viewWater = view<CWater>();
        for (auto entity : viewWater) 
        {
            auto& waterTransform = m_ECS->getComponent<CTransform>(entity);
            auto& waterCollider = m_ECS->getComponent<CCollisionBox>(entity);
            
            if (m_physics->isCollided(transformPlayer, playerCollider, waterTransform, waterCollider) )
            {
                if (!hasComponent<CSwimming>()) 
                {
                    addComponent<CSwimming>();
                    getComponent<CAnimation>().animation = m_game->assets().getAnimation("dwarf-sheet");

                }
                return;
            }
        }

        if (hasComponent<CSwimming>()) 
        {
            removeComponent<CSwimming>();
            getComponent<CAnimation>().animation = m_game->assets().getAnimation("demon-sheet");
        }
    }

    void OnCollisionFunction(EntityID colliderID, CollisionMask colliderLayer, Vec2 overlap)
    {
        // std::cout << "PlayerController: OnCollisionFunction called with colliderID: " << colliderID << std::endl;
        // auto& transformPlayer = getComponent<CTransform>();
        // auto& collisionPlayer = getComponent<CCollisionBox>();
        auto id = m_entity.getID();

        // if (colliderLayer == OBSTACLE_LAYER) 
        // {
            // auto& obstacleTransform = m_ECS->getComponent<CTransform>(colliderID);
            // auto& obstacleCollider = m_ECS->getComponent<CCollisionBox>(colliderID);
            
            // Vec2 overlap = m_physics.overlap(transformPlayer, collisionPlayer, obstacleTransform, obstacleCollider);
            // if ( m_ECS->hasComponent<CChild>(id) )
            // {
            //     EntityID childID = m_ECS->getComponent<CChild>(id).childID;
            //     m_ECS->getComponent<CTransform>(childID).pos += overlap;
            // }      
        // }
        m_ECS->getComponent<CTransform>(id).pos += overlap;
    }
};

