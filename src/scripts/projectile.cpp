#include "../ecs/ScriptableEntity.h"
#include <iostream>

class ProjectileController : public ScriptableEntity 
{
public:
    void OnCreateFunction()
    {
        // addComponent<CCollisionBox>(Vec2{12, 12});
        // getComponent<CTransform>().isMovable = true;
        // getComponent<CProjectileState>().state = "Free";
        // getComponent<CTransform>().vel = (m_game->currentScene()->getMousePosition()-getComponent<CTransform>().pos+m_game->currentScene()->getCameraPosition());
        // getComponent<CTransform>().angle = getComponent<CTransform>().vel.angle();
        // std::cout << "create script entity: OnCreate" << std::endl;
        // Mix_PlayChannel(-1, m_game->assets().getAudio("loot_pickup"), 0);
    }

    void OnDestroyFunction()
    {
        auto& animation     = m_ECS->getComponent<CAnimation>(m_entity.getID());
        animation.animation = m_game->assets().getAnimation("fireball_explode");
        animation.repeat    = false;
        m_ECS->getComponent<CTransform>(m_entity.getID()).isMovable = false;
        m_ECS->queueRemoveComponent<CCollisionBox>(m_entity.getID());
        m_ECS->queueRemoveComponent<CDamage>(m_entity.getID());
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

    void OnAttackFunction(EntityID victimID)
    {
        int damage = m_entity.getComponent<CDamage>().damage;
        m_ECS->getComponent<CHealth>(victimID).HP -= damage;
    }

    void OnCollisionFunction(EntityID colliderID, CollisionMask colliderLayer, Vec2 overlap)
    {
        OnDestroyFunction();
    }
};

