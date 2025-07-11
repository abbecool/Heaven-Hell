#include "ScriptableEntity.h"
#include <iostream>

class WeaponController : public ScriptableEntity 
{
    EntityID spawnProjectile(EntityID creator, Vec2 vel, int layer)
    {
        auto entity = m_ECS->addEntity();
        m_ECS->addComponent<CAnimation>(entity, m_game->assets().getAnimation("fireball"), true, layer);
        m_scene->m_rendererManager.addEntityToLayer(entity, layer);
        m_ECS->addComponent<CTransform>(entity, m_ECS->getComponent<CTransform>(creator).pos, vel, Vec2{1, 1}, vel.angle(), 200.0f, true);
        m_ECS->addComponent<CDamage>(entity, 1);
        m_ECS->getComponent<CDamage>(entity).damageType = {"Fire", "Explosive"};
        // m_ECS->addComponent<CProjectileState>(entity, "Create");
        CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER;
        m_ECS->addComponent<CCollisionBox>(entity, Vec2{12, 12}, PROJECTILE_LAYER, collisionMask);
        // m_ECS->getComponent<CTransform>(entity).isMovable = true;
        // m_ECS->addComponent<CProjectileState>(entity, "Free");
        // m_ECS->getComponent<CTransform>(entity).vel = (m_game->currentScene()->getMousePosition()-m_ECS->getComponent<CTransform>(entity).pos+m_game->currentScene()->getCameraPosition());
        // m_ECS->getComponent<CTransform>(entity).angle = m_ECS->getComponent<CTransform>(entity).vel.angle();

        // spawnShadow(entity, Vec2{0,0}, 1, layer-1);
        auto& script = m_ECS->addComponent<CScript>(entity);
        m_scene->InitiateProjectileScript(script, entity);
        return entity;
    }
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
        // OnAttackFunction();
    }

    void OnAttackFunction()
    {
        spawnProjectile(m_entity.getID(), m_scene->getMousePosition()-m_ECS->getComponent<CTransform>(m_entity.getID()).pos+m_scene->getCameraPosition(), 8);

        // if ( hasComponent<CProjectile>() )
        // {
        //     EntityID projectileID = getComponent<CProjectile>().projectileID;
        //     removeComponent<CProjectile>();
        //     m_ECS->queueRemoveComponent<CParent>(projectileID);
        //     m_ECS->getComponent<CScript>(projectileID).Instance->OnAttackFunction();
        // }
    }

    void OnInteractFunction(EntityID colliderID, CollisionMask colliderLayer)
    {
        addComponent<CParent>(colliderID, Vec2{8, -4});
        m_ECS->addComponent<CWeaponChild>(colliderID, m_entity.getID());
        m_ECS->queueRemoveComponent<CInteractionBox>(m_entity.getID());
        Mix_PlayChannel(-1, m_game->assets().getAudio("loot_pickup"), 0);
    }

};

