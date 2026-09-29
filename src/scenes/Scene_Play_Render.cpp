#include "scenes/Scene_Play.hpp"

#include "assets/Assets.hpp"

#include <algorithm>
#include <cmath>
#include <string>

void Scene_Play::sRenderHealth()
{
    int windowScale = m_game->getScale();

    const SpriteDefinition& heartsSprite = getSprite("hearts");

    auto& playerHealth = m_ECS.getComponent<CHealth>(m_player);
    const float playerHearts = static_cast<float>(playerHealth.HP) / 2.0f;
    const float playerMaxHearts =
        static_cast<float>(playerHealth.HP_max) / 2.0f;
    const Vec2 playerHeartFrameSize = heartsSprite.frameSize();
    const Vec2 playerHeartSize = playerHeartFrameSize * windowScale;
    const bool playerHasHalfHeart = playerHearts != std::floor(playerHearts);
    const float playerVisibleHeartSlots = std::ceil(playerHearts);
    const RectF playerHeartSource = heartsSprite.sourceRegion();
    const RectF playerSrc = {
        playerHeartSource.x +
            (10.0f - playerVisibleHeartSlots) * playerHeartFrameSize.x,
        playerHeartSource.y +
            playerHeartFrameSize.y * static_cast<float>(playerHasHalfHeart),
        playerHeartFrameSize.x * std::ceil(playerMaxHearts),
        playerHeartFrameSize.y};
    const RectF playerDst = {0.0f, 0.0f,
                             playerHeartSize.x * std::ceil(playerMaxHearts),
                             playerHeartSize.y};
    drawSprite(heartsSprite, playerSrc, playerDst);

    const RenderView view = worldRenderView();
    const float viewScale = view.scale > 0.0f ? view.scale : 1.0f;
    for (auto [entityID, health, transform] :
         m_ECS.constView<CHealth, CTransform>())
    {
        if (entityID == m_player)
        {
            continue;
        }

        const float hearts = static_cast<float>(health.HP) / 2.0f;
        const float maxHearts = static_cast<float>(health.HP_max) / 2.0f;

        const Vec2 heartFrameSize = heartsSprite.frameSize();
        const Vec2 heartSize = heartFrameSize * transform.scale *
                               (static_cast<float>(windowScale) / viewScale);
        const bool hasHalfHeart = hearts != std::floor(hearts);
        const float visibleHeartSlots = std::ceil(hearts);
        const RectF heartSource = heartsSprite.sourceRegion();
        const RectF src = {
            heartSource.x + (10.0f - visibleHeartSlots) * heartFrameSize.x,
            heartSource.y + heartFrameSize.y * static_cast<float>(hasHalfHeart),
            heartFrameSize.x * std::ceil(maxHearts), heartFrameSize.y};
        const CSprite& entitySprite = m_ECS.getComponent<CSprite>(entityID);
        const float entityVisualHeight =
            entitySprite.size().y * transform.scale.y;
        const RectF dst = {
            transform.pos.x - std::ceil(maxHearts) * heartSize.x / 2.0f,
            transform.pos.y - entityVisualHeight / 2.0f - heartSize.y / 2.0f,
            heartSize.x * std::ceil(maxHearts), heartSize.y};
        drawWorldSprite(heartsSprite, src, dst);
    }
}

void Scene_Play::sRenderCurrency()
{
    int windowScale = m_game->getScale();

    const SpriteDefinition& coinSprite = getSprite("coin");
    const int currency = m_ECS.hasComponent<CCurrency>(m_player)
                             ? m_ECS.getComponent<CCurrency>(m_player).value
                             : 0;
    const std::string currencyText = std::to_string(currency);
    const Vec2 coinSize = coinSprite.frameSize() * windowScale;
    const float margin = 4.0f * windowScale;
    const float gap = 3.0f * windowScale;
    const float textHeight = std::max(8.0f * windowScale, coinSize.y * 0.55f);
    const float textWidth =
        std::max(8.0f * windowScale,
                 static_cast<float>(currencyText.length()) * textHeight * 0.5f);
    const float currencyWidth = coinSize.x + gap + textWidth;
    const float currencyX =
        static_cast<float>(width() * windowScale) - currencyWidth - margin;
    drawSprite(coinSprite, RectF{currencyX, margin, coinSize.x, coinSize.y});
    m_game->render().drawText(TextDrawCommand{
        currencyText,
        "Minecraft",
        RectF{currencyX + coinSize.x + gap,
              margin + (coinSize.y - textHeight) / 2.0f, textWidth, textHeight},
        {255, 255, 255, 255}});
}

void Scene_Play::sRenderInventory()
{
    int windowScale = m_game->getScale();

    const SpriteDefinition& inventorySprite = getSprite("inventory");
    Vec2 inventorySize = inventorySprite.frameSize() * windowScale;
    drawSprite(inventorySprite,
               RectF{width() / 2.0f * windowScale - inventorySize.x / 2, 0.0f,
                     inventorySize.x, inventorySize.y});
    auto& inventory = m_ECS.getComponent<CInventory>(m_player);
    auto& items = inventory.items;
    auto activeItemIndex = inventory.activeItem.index;
    int slotIndex = -1;
    for (Item& item : items)
    {
        slotIndex++;
        if (slotIndex >= inventory.size())
        {
            break;
        }
        if (item.index == activeItemIndex)
        {
            const SpriteDefinition& activeItemSprite =
                getSprite("activeItemInventory");
            Vec2 activeSize = activeItemSprite.frameSize() * windowScale;
            drawSprite(
                activeItemSprite,
                RectF{(width() / 2.0f + slotIndex * 32.0f) * windowScale -
                          inventorySize.x / 2,
                      0.0f, activeSize.x, activeSize.y});
        }
        if (item.id == -1)
        {
            continue;
        }
        const SpriteDefinition& itemSprite = getSprite(item.iconPath);
        Vec2 itemSize = itemSprite.frameSize() * windowScale;
        drawSprite(itemSprite, itemSprite.firstFrame(),
                   RectF{(width() / 2.0f + slotIndex * 32.0f) * windowScale -
                             inventorySize.x / 2,
                         0.0f, itemSize.x, itemSize.y});
    }
}

void Scene_Play::sRenderUI()
{
    sRenderHealth();
    sRenderCurrency();
    sRenderInventory();
}

void Scene_Play::sRender()
{
    sRenderBasic();
    sRenderUI();

    if (m_playerHealthCritical)
    {
        const float pulse =
            m_lowHealthOverlayConfig.pulseMin +
            (m_lowHealthOverlayConfig.pulseMax -
             m_lowHealthOverlayConfig.pulseMin) *
                (0.5f + 0.5f * std::sin(static_cast<float>(m_currentFrame) *
                                        m_lowHealthOverlayConfig.pulseSpeed));

        m_game->render().drawScreenRadialGradient(
            m_lowHealthOverlayConfig.color,
            m_lowHealthOverlayConfig.centerTransparency,
            m_lowHealthOverlayConfig.edgeTransparency, pulse, 0.5f, 0.5f);
    }

    if (m_drawCollision)
    {
        m_levelLoader.renderChunkGrid(m_game->render());
        m_collisionManager.renderQuadtree(m_game->render());
    }
}
