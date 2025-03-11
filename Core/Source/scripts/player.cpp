#include "ScriptableEntity.h"
#include <iostream>
#include <unordered_map>

// Forward declarations for clarity
struct CTransform;
struct CBoundingBox;
struct CAnimation;
struct CSwimming;
struct CInput;

class PlayerController : public ScriptableEntity 
{
public:
    // Configuration constants
    static constexpr float MOVE_SPEED = 200.0f;      // Units per second
    static constexpr float SWIM_SPEED_MODIFIER = 0.6f; // Slower movement in water
    static constexpr float WATER_BUOYANCY = 4.0f;    // Vertical offset when entering water
    static constexpr float GRAVITY = 9.8f;           // Basic downward force when not swimming

    void OnCreateFunction() override
    {
        std::cout << "PlayerController: Entity created" << std::endl;
        // Initialize player components if not already present
        if (!hasComponent<CInput>()) {
            addComponent<CInput>();
        }
        m_state = State::IDLE;
        m_lastPosition = getComponent<CTransform>().pos; // Cache initial position
    }

    void OnDestroyFunction() override
    {
        std::cout << "PlayerController: Entity destroyed" << std::endl;
        // Cleanup could go here (e.g., unregister from event systems)
    }

    void OnUpdateFunction(float deltaTime) override
    {
        auto& transform = getComponent<CTransform>();
        auto& collider = getComponent<CBoundingBox>();
        auto& animation = getComponent<CAnimation>();
        auto& input = getComponent<CInput>();

        // Handle movement
        handleInput(input, transform, deltaTime);

        // Check water collision
        bool inWater = false;
        auto viewWater = view<CWater>();
        for (auto entity : viewWater) 
        {
            auto& waterTransform = m_ECS->getComponent<CTransform>(entity);
            auto& waterCollider = m_ECS->getComponent<CBoundingBox>(entity);
            
            if (m_physics->isCollided(transform, collider, waterTransform, waterCollider)) 
            {
                inWater = true;
                transitionToSwimming(transform, animation);
                break;
            }
        }

        // Exit swimming state if not in water
        if (!inWater && hasComponent<CSwimming>()) 
        {
            transitionToGround(transform, animation);
        }

        // Apply basic gravity when not swimming
        if (!hasComponent<CSwimming>()) {
            transform.pos.y -= GRAVITY * deltaTime;
        }

        // Update state based on movement
        updateState(transform);

        // Debug output (toggleable)
        if (m_debugMode) {
            logPlayerState(transform);
        }
    }

    // Toggle debug mode via external call (e.g., from a console or UI)
    void toggleDebugMode(bool enabled) { m_debugMode = enabled; }

private:
    enum class State { IDLE, MOVING, SWIMMING };
    State m_state = State::IDLE;
    glm::vec3 m_lastPosition = glm::vec3(0.0f); // Assuming 3D transform with GLM
    bool m_debugMode = false;

    // Component definitions (for clarity; typically in a separate header)
    struct CInput {
        bool moveLeft = false;
        bool moveRight = false;
        bool moveUp = false;
        bool moveDown = false;
    };

    void handleInput(CInput& input, CTransform& transform, float deltaTime)
    {
        float speed = hasComponent<CSwimming>() ? MOVE_SPEED * SWIM_SPEED_MODIFIER : MOVE_SPEED;
        glm::vec3 velocity(0.0f);

        if (input.moveLeft)  velocity.x -= speed;
        if (input.moveRight) velocity.x += speed;
        if (input.moveUp)    velocity.y += speed;
        if (input.moveDown)  velocity.y -= speed;

        transform.pos += velocity * deltaTime;

        // Clamp position to game bounds (example bounds)
        transform.pos.x = glm::clamp(transform.pos.x, -800.0f, 800.0f);
        transform.pos.y = glm::clamp(transform.pos.y, -600.0f, 600.0f);
    }

    void transitionToSwimming(CTransform& transform, CAnimation& animation)
    {
        if (!hasComponent<CSwimming>()) 
        {
            transform.pos.y += WATER_BUOYANCY; // Buoyancy effect
            addComponent<CSwimming>();
            animation.animation = m_game->assets().getAnimation("demon");
            m_game->playSound("splash.wav"); // Assume sound system exists
            std::cout << "Player entered water: Swimming mode activated" << std::endl;
            m_state = State::SWIMMING;
        }
    }

    void transitionToGround(CTransform& transform, CAnimation& animation)
    {
        transform.pos.y -= WATER_BUOYANCY; // Exit water effect
        animation.animation = m_game->assets().getAnimation("wiz");
        m_game->playSound("step.wav");   // Exit water sound
        std::cout << "Player left water: Ground mode restored" << std::endl;
        removeComponent<CSwimming>();
        m_state = State::MOVING;
    }

    void updateState(const CTransform& transform)
    {
        if (m_state != State::SWIMMING) {
            if (transform.pos != m_lastPosition) {
                m_state = State::MOVING;
            } else {
                m_state = State::IDLE;
            }
        }
        m_lastPosition = transform.pos;
    }

    void logPlayerState(const CTransform& transform)
    {
        static std::unordered_map<State, std::string> stateNames = {
            {State::IDLE, "Idle"}, {State::MOVING, "Moving"}, {State::SWIMMING, "Swimming"}
        };
        std::cout << "Player State: " << stateNames[m_state]
                  << " | Pos: (" << transform.pos.x << ", " << transform.pos.y << ")"
                  << " | Swimming: " << (hasComponent<CSwimming>() ? "Yes" : "No") << std::endl;
    }
};
