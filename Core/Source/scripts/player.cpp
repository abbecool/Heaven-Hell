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
        auto& playerCollider = getComponent<CBoundingBox>();

        auto viewWater = view<CWater>();
        for (auto entity : viewWater) 
        {
            auto& waterTransform = m_ECS->getComponent<CTransform>(entity);
            auto& waterCollider = m_ECS->getComponent<CBoundingBox>(entity);
            
            if (m_physics->isCollided(transformPlayer, playerCollider, waterTransform, waterCollider) )
            {
                if (!hasComponent<CSwimming>()) 
                {
                    // transformPlayer.pos.y += 4;
                    addComponent<CSwimming>();
                    std::cout << "Player is swimming" << std::endl;
                    // auto& animation = getComponent<CAnimation>().animation;
                    // animation = m_game->assets().getAnimation("demon");
                    getComponent<CAnimation>().animation = m_game->assets().getAnimation("demon");

                }
                return;
            }
        }

        if (hasComponent<CSwimming>()) 
        {
            // transformPlayer.pos.y -= 4;
            std::cout << "Player is not swimming" << std::endl;
            // getComponent<CAnimation>().animation.setTexture(m_game->assets().getTexture("wiz"));
            getComponent<CAnimation>().animation = m_game->assets().getAnimation("wiz");
            removeComponent<CSwimming>();
        }
    }
};

