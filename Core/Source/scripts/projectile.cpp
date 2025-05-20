#include "ScriptableEntity.h"
#include <iostream>

class ProjectileController : public ScriptableEntity 
{
public:
    void OnCreateFunction()
    {
        // addComponent<CBoundingBox>(Vec2{12, 12});
        // getComponent<CTransform>().isMovable = true;
        // getComponent<CProjectileState>().state = "Free";
        // getComponent<CTransform>().vel = (m_game->currentScene()->getMousePosition()-getComponent<CTransform>().pos+m_game->currentScene()->getCameraPosition());
        // getComponent<CTransform>().angle = getComponent<CTransform>().vel.angle();
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
        // OnAttackFunction();
    }

    void OnAttackFunction()
    {
        // if ( getComponent<CProjectileState>().state == "Ready" )
        // {
        //     addComponent<CBoundingBox>(Vec2{12, 12});
        //     getComponent<CTransform>().isMovable = true;
        //     getComponent<CProjectileState>().state = "Free";
        //     getComponent<CTransform>().vel = (m_game->currentScene()->getMousePosition()-getComponent<CTransform>().pos+m_game->currentScene()->getCameraPosition());
        //     getComponent<CTransform>().angle = getComponent<CTransform>().vel.angle();
        // } else {
        //     removeEntity();
        // }
    }
};

