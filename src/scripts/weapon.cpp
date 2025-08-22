#include "ecs/ScriptableEntity.h"
#include "story/EventBus.h"
#include <iostream>

class WeaponController : public ScriptableEntity 
{
    EntityID spawnProjectile(EntityID creator, Vec2 vel, int layer)
    {
        auto entity = m_ECS->addEntity();
        m_ECS->addComponent<CAnimation>(entity, m_game->assets().getAnimation("fireball"), true, layer);
        m_scene->m_rendererManager.addEntityToLayer(entity, layer);
        m_ECS->addComponent<CTransform>(entity, m_ECS->getComponent<CTransform>(creator).pos, vel.angle());
        m_ECS->addComponent<CVelocity>(entity, vel, 200.0f);
        m_ECS->addComponent<CDamage>(entity, 1);
        m_ECS->getComponent<CDamage>(entity).damageType = {"Fire", "Explosive"};
        CollisionMask collisionMask = ENEMY_LAYER | OBSTACLE_LAYER;
        m_ECS->addComponent<CCollisionBox>(entity, Vec2{6, 6}, PROJECTILE_LAYER, collisionMask);
        m_ECS->addComponent<CLifespan>(entity, 60);

        // spawnShadow(entity, Vec2{0,0}, 1, layer-1);
        auto& script = m_ECS->addComponent<CScript>(entity);
        m_scene->InitiateProjectileScript(script, entity);
        return entity;
    }
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

    void OnAttackFunction()
    {
        Vec2 pos = m_ECS->getComponent<CTransform>(m_entity.getID()).pos;
        spawnProjectile(
            m_entity.getID(), 
            m_scene->getMousePosition()-pos+m_scene->getCameraPosition(), 
            8
        );
    }

    void OnInteractFunction(EntityID colliderID, CollisionMask colliderLayer)
    {
        addComponent<CParent>(colliderID, Vec2{8, -4});
        m_ECS->addComponent<CWeaponChild>(colliderID, m_entity.getID());
        m_ECS->queueRemoveComponent<CInteractionBox>(m_entity.getID());
        Mix_PlayChannel(-1, m_game->assets().getAudio("loot_pickup"), 0);

        std::string name = m_ECS->getComponent<CName>(m_entity.getID()).name;
        m_scene->onEvent({EventType::ItemPickedUp, name});
    }
};
