#include "scenes/Scene_Play.hpp"

#include "ecs/FactionRelations.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
constexpr float PHYSICS_DT = 1.0f / 60.0f;
constexpr float SPRINT_MULTIPLIER = 2.0f;
constexpr float SNEAK_MULTIPLIER = 0.5f;
constexpr float WEAK_MULTIPLIER = 0.33f;
constexpr float STOP_SPEED = 1.0f;
constexpr float FIELD_OF_VIEW_MIN_DOT_PRODUCT = 0.0f;

Vec2 facingDirection(const CState& state)
{
    switch (state.facing)
    {
    case PlayerState::RUN_UP:
        return {0.0f, -1.0f};
    case PlayerState::RUN_DOWN:
        return {0.0f, 1.0f};
    case PlayerState::RUN_LEFT:
        return {-1.0f, 0.0f};
    case PlayerState::RUN_RIGHT:
        return {1.0f, 0.0f};
    case PlayerState::STAND:
        return {0.0f, 1.0f};
    }

    return {0.0f, 1.0f};
}

bool isWithinFieldOfView(const CState& state, const Vec2& observerPos,
                         const Vec2& targetPos)
{
    const Vec2 toTarget = (targetPos - observerPos).norm();
    const Vec2 facing = facingDirection(state);
    const float directionDotProduct =
        facing.x * toTarget.x + facing.y * toTarget.y;
    return toTarget.isNull() ||
           directionDotProduct >= FIELD_OF_VIEW_MIN_DOT_PRODUCT;
}
} // namespace

void Scene_Play::sLoader()
{
    Vec2 playerPosition = m_ECS.getComponent<CTransform>(m_player).pos;
    m_levelLoader.update(playerPosition);
}

void Scene_Play::onTerrainChanged()
{
    m_collisionManager.rebuildStaticQuadtree();
}

void Scene_Play::sAI()
{
    Vec2 playerPos = m_ECS.getComponent<CTransform>(m_player).pos;

    for (auto [e, agent, input, transform, state, allegiance] :
         m_ECS.View<CAIAgent, CInput, CTransform, CState, CAllegiance>())
    {
        if (e == m_player)
        {
            continue;
        }

        Vec2 pos = transform.pos;
        float distToPlayer = (playerPos - pos).length();
        bool inRange =
            agent.sightRange > 0.0f && distToPlayer <= agent.sightRange;
        agent.canSeePlayer = inRange &&
                             isWithinFieldOfView(state, pos, playerPos) &&
                             hasLineOfSight(pos, playerPos);

        if (agent.canSeePlayer)
        {
            agent.lastKnownPlayerPos = playerPos;
            agent.memoryTimer = agent.memoryDuration;
        }

        const Faction playerFaction =
            m_ECS.getComponent<CAllegiance>(m_player).perceivedFaction;
        switch (agent.state)
        {
        case AIStateType::Patrol:
            if (isHostile(allegiance.perceivedFaction, playerFaction) &&
                agent.canSeePlayer)
            {
                agent.state = AIStateType::Chase;
            }
            break;

        case AIStateType::Chase:
            if (!agent.canSeePlayer)
            {
                agent.state = AIStateType::Investigate;
                agent.hasPatrolTarget = false;
            }
            break;

        case AIStateType::Investigate:
            if (agent.canSeePlayer)
            {
                agent.state = AIStateType::Chase;
            }
            else
            {
                agent.memoryTimer--;
                bool arrived =
                    (pos - agent.lastKnownPlayerPos).length() < 12.0f;
                if (arrived || agent.memoryTimer <= 0)
                {
                    agent.state = AIStateType::Patrol;
                    agent.hasPatrolTarget = false;
                    agent.memoryTimer = 0;
                }
            }
            break;
        }

        input.direction = {0, 0};

        switch (agent.state)
        {
        case AIStateType::Chase:
            input.direction = (playerPos - pos).norm();
            input.use = distToPlayer < activeItemUseRange(e);
            input.useHeld = input.use;
            break;

        case AIStateType::Investigate:
            input.direction = (agent.lastKnownPlayerPos - pos).norm();
            input.use = false;
            input.useHeld = false;
            break;

        case AIStateType::Patrol:
            tickPatrol(agent, pos, input);
            input.use = false;
            input.useHeld = false;
            break;
        }
    }
}

void Scene_Play::sMovement()
{
    for (auto [e, inputs, velocity, body] :
         m_ECS.View<CInput, CVelocity, CPhysicsBody>())
    {
        const bool isPlayer = (e == m_player);
        if (!isPlayer && m_ECS.hasComponent<CAttackState>(e))
        {
            velocity.vel = {0, 0};
            inputs = CInput();
            continue;
        }

        if (!inputs.direction.isNull())
        {
            const float sneakMultiplier =
                inputs.shift ? SNEAK_MULTIPLIER : 1.0f;
            const float sprintMultiplier =
                inputs.ctrl ? SPRINT_MULTIPLIER : 1.0f;
            const float weakMultiplier =
                (isPlayer && m_playerHealthCritical) ? WEAK_MULTIPLIER : 1.0f;
            body.accumulatedForce +=
                inputs.direction.norm(body.moveForce * sprintMultiplier *
                                      sneakMultiplier * weakMultiplier);
        }
        if (!isPlayer)
        {
            inputs = CInput();
        }
    }

    for (auto [e, velocity, body] : m_ECS.View<CVelocity, CPhysicsBody>())
    {
        const Vec2 acceleration = body.accumulatedForce / body.mass;

        if (body.linearDamping > 0.0f)
        {
            const float decay = std::exp(-body.linearDamping * PHYSICS_DT);
            velocity.vel = velocity.vel * decay +
                           acceleration * ((1.0f - decay) / body.linearDamping);
        }
        else
        {
            velocity.vel += acceleration * PHYSICS_DT;
        }

        float maxSpeed = body.maxSpeed;
        if (m_ECS.hasComponent<CInput>(e) && m_ECS.getComponent<CInput>(e).ctrl)
        {
            maxSpeed *= SPRINT_MULTIPLIER;
        }
        if (velocity.vel.length() > maxSpeed)
        {
            velocity.vel = velocity.vel.norm(maxSpeed);
        }
        if (velocity.vel.length() < STOP_SPEED)
        {
            velocity.vel = {0, 0};
        }

        body.accumulatedForce = {0, 0};
    }

    for (auto [e, transform, velocity] : m_ECS.View<CTransform, CVelocity>())
    {
        transform.prevPos = transform.pos;
        transform.pos += velocity.vel * PHYSICS_DT;
    }

    for (auto [e, parent, transform] :
         m_ECS.View<CParent, CTransform>(ecs::Exclude<CStatic>{}))
    {
        transform.pos = m_ECS.getComponent<CTransform>(parent.parent).pos +
                        parent.relativePos;
    }
}

void Scene_Play::sCollision()
{
    m_collisionManager.doCollisions();
}

void Scene_Play::sStatus()
{
    const auto& playerHealth = m_ECS.getComponent<CHealth>(m_player);
    m_playerHealthCritical = (playerHealth.HP == 1);

    for (auto [entityID, lifespan] : m_ECS.View<CLifespan>())
    {
        lifespan.lifespan--;
        if (lifespan.lifespan <= 0)
        {
            m_ECS.queueRemoveEntity(entityID);
            if (m_ECS.hasComponent<CCollider>(entityID))
            {
                m_ECS.removeComponent<CCollider>(entityID);
            }
            if (m_ECS.hasComponent<CPossessable>(entityID))
            {
                m_ECS.removeComponent<CPossessable>(entityID);
            }
        }
    }

    for (auto [entityID, lifetime] : m_ECS.View<CActiveHitboxLifetime>())
    {
        lifetime.framesRemaining--;
        if (lifetime.framesRemaining <= 0)
        {
            m_ECS.queueRemoveComponent<CCollider>(entityID);
            m_ECS.queueRemoveComponent<CDamage>(entityID);
            m_ECS.queueRemoveComponent<CAttackHitbox>(entityID);
            m_ECS.queueRemoveComponent<CActiveHitboxLifetime>(entityID);
        }
    }

    for (auto [entityID, flash] : m_ECS.View<CDamageFlash>())
    {
        if (flash.framesRemaining > 0)
        {
            flash.framesRemaining--;
        }
    }

    for (auto [entityID, health] : m_ECS.View<CHealth>())
    {
        if (health.HP > 0)
        {
            health.HP = std::min(health.HP, health.HP_max);
            continue;
        }
        auto& transform = m_ECS.getComponent<CTransform>(entityID);
        if (m_player == entityID)
        {
            std::cout << "Player has died!" << std::endl;
            m_restart = true;
            continue;
        }
        if (m_ECS.hasComponent<CLifespan>(entityID))
        {
            const int lifespan =
                m_ECS.getComponent<CLifespan>(entityID).lifespan;
            if (lifespan > 0)
            {
                continue;
            }
        }
        const Vec2 gridPos{std::floor(transform.pos.x / m_gridSize.x),
                           std::floor(transform.pos.y / m_gridSize.y)};
        Spawn("coin", gridPos);
        m_ECS.queueRemoveEntity(entityID);
        m_game->playAudio("enemy_death_ida");
        Emit(Event{EventType::EntityKilled,
                   m_ECS.getComponent<CName>(entityID).name});
    }
}

void Scene_Play::sAnimation()
{
    for (auto [e, state, animation, velocity] :
         m_ECS.View<CState, CAnimation, CVelocity>())
    {
        bool useStateAnimation = true;
        if (m_ECS.hasComponent<CAttackState>(e))
        {
            CAttackState& attackState = m_ECS.getComponent<CAttackState>(e);
            if (attackState.hasAnimationOverride)
            {
                useStateAnimation = false;
            }
            else if (m_ECS.hasComponent<CWeapon>(e) &&
                     m_ECS.getComponent<CWeapon>(e).attackAnimationRow >= 0)
            {
                animation.currentRow =
                    m_ECS.getComponent<CWeapon>(e).attackAnimationRow;
                useStateAnimation = false;
            }
            else
            {
                Vec2 attackDirection =
                    m_ECS.getComponent<CAttackState>(e).direction;
                if (attackDirection.mainDir().x > 0)
                    changePlayerState(e, PlayerState::RUN_RIGHT);
                else if (attackDirection.mainDir().x < 0)
                    changePlayerState(e, PlayerState::RUN_LEFT);
                else if (attackDirection.mainDir().y > 0)
                    changePlayerState(e, PlayerState::RUN_DOWN);
                else if (attackDirection.mainDir().y < 0)
                    changePlayerState(e, PlayerState::RUN_UP);
                else
                    changePlayerState(e, PlayerState::STAND);
            }
        }
        else if (velocity.vel.isNull())
            changePlayerState(e, PlayerState::STAND);
        else if (velocity.vel.mainDir().x > 0)
            changePlayerState(e, PlayerState::RUN_RIGHT);
        else if (velocity.vel.mainDir().x < 0)
            changePlayerState(e, PlayerState::RUN_LEFT);
        else if (velocity.vel.mainDir().y > 0)
            changePlayerState(e, PlayerState::RUN_DOWN);
        else if (velocity.vel.mainDir().y < 0)
            changePlayerState(e, PlayerState::RUN_UP);

        if (useStateAnimation && state.changeAnimate)
        {
            animation.currentRow = static_cast<int>(state.state);
        }
    }

    auto& projectilePool = m_ECS.getComponentPool<CProjectile>();
    for (auto [e, projectileState, animation] :
         m_ECS.View<CProjectileState, CAnimation>())
    {
        if (!m_ECS.hasComponent<CProjectile>(e))
        {
            continue;
        }

        if (projectileState.phase != ProjectilePhase::Flying)
        {
            continue;
        }

        auto& projectile = projectilePool.getComponent(e);
        projectile.flightLifetime--;
        if (projectile.flightLifetime <= 0)
        {
            destroyProjectile(e);
        }
    }

    updateAnimations();
}

void Scene_Play::sAudio()
{
    for (auto [id, audio] : m_ECS.constView<CAudio>())
    {
        m_game->playAudio(audio.audioName);
        m_ECS.queueRemoveComponent<CAudio>(id);
    }
}
