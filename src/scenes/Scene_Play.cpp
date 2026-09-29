#include "scenes/Scene_Play.hpp"
#include "scenes/Scene_Menu.hpp"
#include "scenes/Scene_GameOver.hpp"

#include "core/Game.hpp"
#include "core/Action.hpp"

#include "debug/EntityInspector.hpp"

#include <iostream>
#include <string>

// TODO: fix line count in file

using json = nlohmann::json;

namespace
{
std::string activeTerrainPath(const std::string& fallbackPath)
{
    try
    {
        LayoutRepository layouts;
        layouts.load();
        return layouts.activeLayout().terrainPath;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Could not load active level layout: " << exception.what()
                  << ". Falling back to " << fallbackPath << std::endl;
        return fallbackPath;
    }
}

} // namespace

Scene_Play::Scene_Play(Game* game, std::string levelPath, bool newGame)
    : Scene(game), m_levelPath(activeTerrainPath(levelPath)),
      m_collisionManager(&m_ECS, this),
      m_inventoryManager("config_files/items"),
      m_storyManager("config_files/story1.json"),
      m_levelLoader(this, m_gridSize, game->loadImagePixels(m_levelPath)),
      m_newGame(newGame)
{
    m_ECS.setEntityRemovalObserver([this](EntityID id) {
        m_rendererManager.queueRemoveEntity(id);
        const auto it = m_placedEntities.find(id);
        if (it != m_placedEntities.end())
        {
            m_removedPlacements.insert(it->second);
            m_placedEntities.erase(it);
        }
    });
    if (!m_newGame)
        restoreSavedWorld();
    registerAction(InputCode::W, "UP");
    registerAction(InputCode::Up, "UP");
    registerAction(InputCode::S, "DOWN");
    registerAction(InputCode::Down, "DOWN");
    registerAction(InputCode::A, "LEFT");
    registerAction(InputCode::Left, "LEFT");
    registerAction(InputCode::D, "RIGHT");
    registerAction(InputCode::Right, "RIGHT");

    registerAction(InputCode::I, "INVENTORY");
    registerAction(InputCode::L, "PRINT_HOVERED_COMPONENTS");
    registerAction(InputCode::MouseLeft, "USE");
    registerAction(InputCode::MouseRight, "WRITE POSITION");
    registerAction(InputCode::MouseWheel, "SCROLL");
    registerAction(InputCode::E, "INTERACT");
    registerAction(InputCode::LeftShift, "SHIFT");
    registerAction(InputCode::LeftCtrl, "CTRL");
    registerAction(InputCode::Escape, "ESC");
    registerAction(InputCode::U, "SAVE");
    registerAction(InputCode::R, "RESET");

    registerAction(InputCode::X, "CAMERA FOLLOW");
    registerAction(InputCode::Z, "CAMERA PAN");
    registerAction(InputCode::Plus, "ZOOM IN");
    registerAction(InputCode::Minus, "ZOOM OUT");
    registerAction(InputCode::Q, "TAKE OVER");
    registerAction(InputCode::P, "PAUSE");
    registerAction(InputCode::O, "FPS COUNTER");
    registerAction(InputCode::K, "KILL_PLAYER");
    registerAction(InputCode::F3, "TOGGLE_COLLISION");
    registerAction(InputCode::F4, "TOGGLE_GRID");
    registerAction(InputCode::F5, "TOGGLE_TEXTURE");
    registerAction(InputCode::Num1, "Slot1");
    registerAction(InputCode::Num2, "Slot2");
    registerAction(InputCode::Num3, "Slot3");
    registerAction(InputCode::Num7, "TP1");
    registerAction(InputCode::Num8, "TP2");
    registerAction(InputCode::Num9, "TP3");

    const Vec2 worldSize = m_levelLoader.getWorldSize();
    m_collisionManager.setWorldBounds(worldSize / 2.0f, worldSize);
    m_collisionManager.rebuildStaticQuadtree();

    spawnPlayer();
    // Layout entities spawn after the player so their existing AI targets it.
    loadActiveLayout();

    m_camera.calibrate(Vec2{width(), height()}, m_levelLoader.getLevelSize(),
                       m_gridSize);
}

void Scene_Play::sDoAction(const Action& action)
{
    if (action.type() == "START")
    {
        if (action.name() == "RESET")
        {
            m_game->changeScene(
                "PLAY", std::make_shared<Scene_Play>(m_game, m_levelPath, true),
                true);
        }
        else if (action.name() == "KILL_PLAYER")
        {
            m_ECS.getComponent<CHealth>(m_player).HP = 0;
        }
        else if (action.name() == "Slot1")
        {
            updateActiveItem(0);
        }
        else if (action.name() == "Slot2")
        {
            updateActiveItem(1);
        }
        else if (action.name() == "Slot3")
        {
            updateActiveItem(2);
        }
        if (action.name() == "UP")
        {
            m_ECS.getComponent<CInput>(m_player).up = true;
        }
        if (action.name() == "DOWN")
        {
            m_ECS.getComponent<CInput>(m_player).down = true;
        }
        if (action.name() == "LEFT")
        {
            m_ECS.getComponent<CInput>(m_player).left = true;
        }
        if (action.name() == "RIGHT")
        {
            m_ECS.getComponent<CInput>(m_player).right = true;
        }
        if (action.name() == "SHIFT")
        {
            m_ECS.getComponent<CInput>(m_player).shift = true;
        }
        if (action.name() == "CTRL")
        {
            m_ECS.getComponent<CInput>(m_player).ctrl = true;
        }
        if (action.name() == "INTERACT")
        {
            m_ECS.getComponent<CInput>(m_player).interact = true;
        }
        if (action.name() == "TAKE OVER")
        {
            auto& input = m_ECS.getComponent<CInput>(m_player);
            if (!input.possesHeld)
            {
                input.posses = true;
            }
            input.possesHeld = true;
        }
        if (action.name() == "USE")
        {
            auto& input = m_ECS.getComponent<CInput>(m_player);
            input.use = true;
            input.useHeld = true;
        }
        if (action.name() == "TOGGLE_TEXTURE")
        {
            m_drawTextures = !m_drawTextures;
        }
        if (action.name() == "TOGGLE_COLLISION")
        {
            m_drawCollision = !m_drawCollision;
        }
        if (action.name() == "TOGGLE_GRID")
        {
            m_drawDrawGrid = !m_drawDrawGrid;
        }
    }
    else if (action.type() == "END")
    {
        if (action.name() == "PAUSE")
        {
            togglePause();
        }
        if (action.name() == "FPS COUNTER")
        {
            m_game->toggleRenderFPS();
        }
        if (action.name() == "INVENTORY")
        {
            std::cout << "toggle inventory" << std::endl;
        }
        if (action.name() == "ZOOM IN")
        {
            m_camera.stepCameraZoom(-1, m_game->getScale());
        }
        if (action.name() == "ZOOM OUT")
        {
            m_camera.stepCameraZoom(1, m_game->getScale());
        }
        if (action.name() == "CAMERA FOLLOW")
        {
            m_camera.toggleCameraFollow();
        }
        if (action.name() == "CAMERA PAN")
        {
            m_pause = m_camera.startPan(2048, 1000, Vec2{0, 0}, m_pause);
        }
        if (action.name() == "SAVE")
        {
            saveGame();
        }
        if (action.name() == "TP1")
        {
            m_ECS.getComponent<CTransform>(m_player).pos =
                Vec2{460 * 16, 460 * 16};
        }
        if (action.name() == "TP2")
        {
            m_ECS.getComponent<CTransform>(m_player).pos =
                Vec2{292 * 16, 236 * 16};
        }
        if (action.name() == "TP3")
        {
            m_ECS.getComponent<CTransform>(m_player).pos =
                Vec2{801 * 16, 181 * 16};
        }
        if (action.name() == "DOWN")
        {
            m_ECS.getComponent<CInput>(m_player).down = false;
        }
        if (action.name() == "UP")
        {
            m_ECS.getComponent<CInput>(m_player).up = false;
        }
        if (action.name() == "LEFT")
        {
            m_ECS.getComponent<CInput>(m_player).left = false;
        }
        if (action.name() == "RIGHT")
        {
            m_ECS.getComponent<CInput>(m_player).right = false;
        }
        if (action.name() == "SHIFT")
        {
            m_ECS.getComponent<CInput>(m_player).shift = false;
        }
        if (action.name() == "CTRL")
        {
            m_ECS.getComponent<CInput>(m_player).ctrl = false;
        }
        if (action.name() == "INTERACT")
        {
            m_ECS.getComponent<CInput>(m_player).interact = false;
        }
        if (action.name() == "TAKE OVER")
        {
            auto& input = m_ECS.getComponent<CInput>(m_player);
            input.posses = false;
            input.possesHeld = false;
        }
        if (action.name() == "USE")
        {
            auto& input = m_ECS.getComponent<CInput>(m_player);
            input.use = false;
            input.useHeld = false;
        }
        if (action.name() == "WRITE POSITION")
        {
            Vec2 cursorPosition =
                (m_mousePosition + m_camera.position) / m_gridSize;
            cursorPosition.print("Cursor position");
        }
        if (action.name() == "PRINT_HOVERED_COMPONENTS")
        {
            printHoveredEntityComponents();
        }
        if (action.name() == "ESC")
        {
            m_game->changeScene("SETTINGS",
                                std::make_shared<Scene_Pause>(m_game), false);
            saveGame();
            m_pause = true;
        }
    }
    else if (action.name() == "SCROLL")
    {
        const CInventory& inventory = m_ECS.getComponent<CInventory>(m_player);
        int size = inventory.size();
        const int index = inventory.activeItem.index;
        int newIndex = (index - getMouseState().scroll + size * 10) % size;
        updateActiveItem(newIndex);
    }
    auto& inputs = m_ECS.getComponent<CInput>(m_player);
    inputs.direction = {0, 0};
    if (inputs.up)
    {
        inputs.direction.y--;
    }
    if (inputs.down)
    {
        inputs.direction.y++;
    }
    if (inputs.left)
    {
        inputs.direction.x--;
    }
    if (inputs.right)
    {
        inputs.direction.x++;
    }
}

void Scene_Play::printHoveredEntityComponents()
{
    const RenderView view = worldRenderView();
    const float windowScale = static_cast<float>(m_game->getScale());
    const Vec2 worldPoint{
        (m_mousePosition.x * windowScale - view.originX) / view.scale +
            view.cameraX,
        (m_mousePosition.y * windowScale - view.originY) / view.scale +
            view.cameraY};

    const std::vector<EntityID> entities =
        DebugEntityInspector::findInspectableEntitiesAt(m_ECS, worldPoint);
    if (entities.empty())
    {
        std::cout << "No inspectable entities at world position {\"x\":"
                  << worldPoint.x << ",\"y\":" << worldPoint.y << "}."
                  << std::endl;
        return;
    }

    for (const EntityID entity : entities)
    {
        std::cout << "--- Hovered Entity Inspector ---" << std::endl;
        std::cout << DebugEntityInspector::inspectEntity(m_ECS, entity).dump(2)
                  << std::endl;
    }
}

void Scene_Play::update()
{
    m_pause =
        m_camera.update(m_ECS.getComponent<CTransform>(m_player).pos, m_pause);
    if (!m_pause)
    {
        sLoader();
        sAI();
        sAttack();
        sMovement();
        sStatus();
        sCollision();
        sAnimation();
        sAudio();
        m_currentFrame++;
    }
    sRender();
    m_ECS.update();
    m_rendererManager.update();

    if (m_restart)
    {
        m_game->changeScene("GAMEOVER",
                            std::make_shared<Scene_GameOver>(m_game), true);
        return;
    }
    if (m_storyManager.isStoryFinished())
    {
        onFinish();
        return;
    }
}

void Scene_Play::onEnd()
{
    m_game->changeScene("MAIN_MENU", std::make_shared<Scene_Menu>(m_game));
}

void Scene_Play::onFinish()
{
    std::cout << "Warning, removing scene_play instance" << std::endl;
    m_game->changeScene("FINISH", std::make_shared<Scene_Finish>(m_game), true);
}

void Scene_Play::OnPlayerDeath()
{
    std::cout << "Warning, removing scene_play instance" << std::endl;
    m_game->changeScene("GAMEOVER", std::make_shared<Scene_GameOver>(m_game),
                        true);
}

void Scene_Play::setPaused(bool pause)
{
    m_pause = pause;
}

void Scene_Play::togglePause()
{
    m_pause = !m_pause;
}

Vec2 Scene_Play::getCameraPosition()
{
    return m_camera.position;
}

EntityID Scene_Play::changePlayerID(EntityID otherID)
{
    m_player = otherID;
    return m_player;
}
