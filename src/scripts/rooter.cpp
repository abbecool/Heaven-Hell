#include "../ecs/ScriptableEntity.h"
#include <iostream>

class RooterController : public ScriptableEntity 
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
        // auto& pos = getComponent<CTransform>().pos;
        // pos.x += 1;
        // auto& transform = transformPool.getComponent(e);
        // auto& pathfind = pathfindPool.getComponent(e);
        // if ((pathfind.target - transform.pos).length() < 64*2) {
        //     transform.vel = pathfind.target - transform.pos;
        // } else {
        //     transform.vel = Vec2 {0,0};
        // }
        // pathfind.target = transformPool.getComponent(m_player).pos;
        OnAttackFunction();
    }

    void OnAttackFunction()
    {

        // if can attack and sees enemy
        // spawn damage area
        
        auto& transform = getComponent<CTransform>();
        auto& attack = getComponent<CAttack>();
        attack.attackTimer--;
        
        Vec2 target = getComponent<CPathfind>().target;
        if ( abs((target-transform.pos).length()) < (float)attack.range && (attack.attackTimer <= 0))
        {
            // child parent relation removed since code does not support multiple children per parent
            EntityID damageAreaID = m_ECS->addEntity();
            // addComponent<CChild>(damageAreaID, true);
            // m_ECS->addComponent<CParent>(damageAreaID, m_entity.getID(), (target-transform.pos).norm(64/4));
            m_ECS->addComponent<CTransform>(damageAreaID, transform.pos + (target-transform.pos).norm(20) );
            m_ECS->addComponent<CCollisionBox>(damageAreaID, attack.area, 0, 0, 255);
            m_ECS->addComponent<CDamage>(damageAreaID, attack.damage);
            m_ECS->addComponent<CLifespan>(damageAreaID, attack.duration);
            attack.attackTimer = attack.speed;
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

