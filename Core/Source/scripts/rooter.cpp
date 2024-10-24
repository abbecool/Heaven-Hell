#include "ScriptableEntity.h"
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
        
        Vec2 target = getComponent<CPathfind>().target;
        if ( target.length() < 3*64 )
        {
            EntityID damageAreaID = m_ECS->addEntity();
            m_ECS->addComponent<CTransform>(damageAreaID, getComponent<CTransform>().pos + Vec2{32,0});
            m_ECS->addComponent<CBoundingBox>(damageAreaID, Vec2{16, 16});
            m_ECS->addComponent<CDamage>(damageAreaID, 1, 60*3);
            m_ECS->addComponent<CLifespan>(damageAreaID, 60);
        }
        
    }

    void OnCollisionFunction()
    {
        // auto& pos = getComponent<CTransform>().pos;
        // pos.x += 1;
    }
};

