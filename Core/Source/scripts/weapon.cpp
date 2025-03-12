#include "ScriptableEntity.h"
#include <iostream>

class WeaponController : public ScriptableEntity 
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
        if ( hasComponent<CProjectile>() )
        {
            EntityID projectileID = getComponent<CProjectile>().projectileID;
            removeComponent<CProjectile>();
            m_ECS->queueRemoveComponent<CParent>(projectileID);
            m_ECS->getComponent<CScript>(projectileID).Instance->OnAttackFunction();
        }
    }

};

