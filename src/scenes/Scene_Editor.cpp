#include "scenes/Scene_Editor.hpp"

#include "core/Action.hpp"
#include "core/Game.hpp"
#include "external/json.hpp"
#include "scenes/Scene_Menu.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>

namespace {

constexpr float ToolbarHeight = 112.0f;
constexpr float EditorCameraSpeed = 8.0f;
constexpr Color PanelColor{16, 24, 38, 230};
constexpr Color ActiveColor{73, 128, 184, 255};
constexpr Color ButtonColor{48, 68, 92, 255};

const char* categoryLabel(EditorEntityCategory category)
{
    switch (category) {
    case EditorEntityCategory::Character: return "Characters";
    case EditorEntityCategory::Item: return "Items";
    case EditorEntityCategory::Structure: return "Structures";
    }
    return "Entities";
}

} // namespace

Scene_Editor::Scene_Editor(Game* game)
    : Scene(game)
{
    registerAction(InputCode::MouseLeft, "PRIMARY");
    registerAction(InputCode::MouseRight, "SECONDARY");
    registerAction(InputCode::MouseWheel, "PALETTE_SCROLL");
    registerAction(InputCode::W, "UP");
    registerAction(InputCode::Up, "UP");
    registerAction(InputCode::S, "DOWN");
    registerAction(InputCode::Down, "DOWN");
    registerAction(InputCode::A, "LEFT");
    registerAction(InputCode::Left, "LEFT");
    registerAction(InputCode::D, "RIGHT");
    registerAction(InputCode::Right, "RIGHT");
    registerAction(InputCode::Plus, "ZOOM_IN");
    registerAction(InputCode::Minus, "ZOOM_OUT");
    registerAction(InputCode::U, "SAVE");
    registerAction(InputCode::Escape, "ESC");
    registerAction(InputCode::Delete, "DELETE");
    registerAction(InputCode::F4, "TOGGLE_GRID");

    try {
        m_layouts.load();
        m_catalog.load();
        m_ready = true;
        m_status = "Choose a layout to edit.";
    }
    catch (const std::exception& exception) {
        m_status = std::string("Editor setup failed: ") + exception.what();
        std::cerr << m_status << std::endl;
    }
}

Vec2 Scene_Editor::scenePlayStartPosition() const
{
    std::ifstream file("config_files/entities/player.json");
    if (!file) {
        throw std::runtime_error("Could not read Scene_Play player definition");
    }

    nlohmann::json data;
    file >> data;
    const nlohmann::json& player = data.at("player");
    const Vec2 spawnGrid = player.at("spawn");
    const std::string spriteName = player.at("components").at("CAnimation").at("animation").get<std::string>();
    return spawnGrid * m_gridSize + getSprite(spriteName).frameSize() / 2.0f;
}

bool Scene_Editor::contains(const RectF& rect, Vec2 point)
{
    return point.x >= rect.x && point.x <= rect.x + rect.w &&
        point.y >= rect.y && point.y <= rect.y + rect.h;
}

void Scene_Editor::drawText(const std::string& text, RectF rect, Color color) const
{
    m_game->render().drawText(TextDrawCommand{text, "Minecraft", screenRect(rect), color});
}

RectF Scene_Editor::screenRect(const RectF& virtualRect) const
{
    const float scale = static_cast<float>(m_game->getScale());
    return {
        virtualRect.x * scale,
        virtualRect.y * scale,
        virtualRect.w * scale,
        virtualRect.h * scale
    };
}

void Scene_Editor::drawButton(const RectF& rect, const std::string& label, bool active) const
{
    m_game->render().fillRect(screenRect(rect), active ? ActiveColor : ButtonColor);
    m_game->render().drawRect(screenRect(rect), {190, 210, 235, 255});
    drawText(label, {rect.x + 3.0f, rect.y + 2.0f, rect.w - 6.0f, rect.h - 4.0f});
}

void Scene_Editor::sRenderPicker()
{
    m_game->render().fillRect(screenRect({0.0f, 0.0f, static_cast<float>(width()), static_cast<float>(height())}), PanelColor);
    drawText("LAYOUT EDITOR", {24.0f, 16.0f, 250.0f, 24.0f}, {255, 235, 160, 255});
    drawButton({24.0f, 46.0f, 110.0f, 22.0f}, "NEW LAYOUT");
    drawText("Open a layout, or mark one as the layout Scene_Play uses.", {150.0f, 49.0f, 440.0f, 18.0f});

    if (!m_ready) {
        drawText(m_status, {24.0f, 90.0f, 590.0f, 30.0f}, {255, 120, 120, 255});
        return;
    }

    float y = 84.0f;
    for (const LayoutInfo& info : m_layouts.layouts()) {
        const bool active = info.id == m_layouts.activeLayout().id;
        m_game->render().fillRect(screenRect({18.0f, y, 604.0f, 26.0f}), active ? Color{37, 70, 90, 255} : Color{31, 42, 58, 255});
        drawText((active ? "* " : "") + info.displayName, {24.0f, y + 4.0f, 190.0f, 18.0f});
        drawText(active ? "ACTIVE IN PLAY" : info.id, {214.0f, y + 4.0f, 120.0f, 18.0f}, active ? Color{155, 230, 165, 255} : Color{190, 200, 215, 255});
        drawButton({342.0f, y + 3.0f, 58.0f, 20.0f}, "OPEN");
        drawButton({404.0f, y + 3.0f, 58.0f, 20.0f}, "ACTIVE", active);
        drawButton({466.0f, y + 3.0f, 66.0f, 20.0f}, "DUP");
        drawButton({536.0f, y + 3.0f, 72.0f, 20.0f}, "DELETE");
        y += 30.0f;
    }
    drawText(m_status, {24.0f, static_cast<float>(height()) - 25.0f, 590.0f, 18.0f}, {255, 235, 160, 255});
}

void Scene_Editor::sRenderWorkspace()
{
    sRenderBasic();
    if (m_selectedEntity && m_ECS.isAlive(*m_selectedEntity) &&
        m_ECS.hasComponent<CTransform>(*m_selectedEntity) && m_ECS.hasComponent<CSprite>(*m_selectedEntity)) {
        const CTransform& transform = m_ECS.getComponent<CTransform>(*m_selectedEntity);
        const CSprite& sprite = m_ECS.getComponent<CSprite>(*m_selectedEntity);
        const Vec2 size = sprite.size() * transform.scale;
        m_game->render().drawWorldRect({
            transform.pos.x - size.x / 2.0f - 2.0f,
            transform.pos.y - size.y / 2.0f - 2.0f,
            size.x + 4.0f,
            size.y + 4.0f
        }, {255, 220, 70, 255});
    }

    m_game->render().fillRect(screenRect({0.0f, 0.0f, static_cast<float>(width()), ToolbarHeight}), PanelColor);
    drawButton({8.0f, 7.0f, 68.0f, 20.0f}, "SELECT", m_mode == EditMode::Select);
    drawButton({80.0f, 7.0f, 62.0f, 20.0f}, "PLACE", m_mode == EditMode::Place);
    drawButton({146.0f, 7.0f, 66.0f, 20.0f}, "DELETE", m_mode == EditMode::Delete);
    drawButton({220.0f, 7.0f, 52.0f, 20.0f}, "SAVE");
    drawButton({276.0f, 7.0f, 66.0f, 20.0f}, "EXIT");
    drawText(m_currentLayout.displayName + (m_dirty ? " *" : ""), {355.0f, 9.0f, 270.0f, 18.0f}, {255, 235, 160, 255});

    const RectF characterTab{8.0f, 35.0f, 100.0f, 18.0f};
    const RectF itemTab{112.0f, 35.0f, 70.0f, 18.0f};
    const RectF structureTab{186.0f, 35.0f, 96.0f, 18.0f};
    drawButton(characterTab, "CHARACTERS", m_selectedCategory == EditorEntityCategory::Character);
    drawButton(itemTab, "ITEMS", m_selectedCategory == EditorEntityCategory::Item);
    drawButton(structureTab, "STRUCTURES", m_selectedCategory == EditorEntityCategory::Structure);
    drawText("Mouse wheel over this panel changes palette page.", {300.0f, 37.0f, 310.0f, 16.0f}, {190, 200, 215, 255});

    const std::vector<EditorEntityDefinition>& entries = m_catalog.category(m_selectedCategory);
    const int visibleCount = 7;
    const int maxOffset = std::max(0, static_cast<int>(entries.size()) - visibleCount);
    const int offset = std::clamp(m_paletteOffset, 0, maxOffset);
    for (int i = 0; i < visibleCount && offset + i < static_cast<int>(entries.size()); ++i) {
        const EditorEntityDefinition& definition = entries[offset + i];
        const RectF rect{8.0f + static_cast<float>(i) * 90.0f, 60.0f, 86.0f, 20.0f};
        drawButton(rect, definition.displayName, definition.id == m_selectedDefinition);
    }
    drawText("Mode: " + std::string(m_mode == EditMode::Select ? "Select" : m_mode == EditMode::Place ? "Place" : "Delete") +
        " | Palette: " + categoryLabel(m_selectedCategory) +
        " | U saves, Esc exits", {8.0f, 87.0f, 615.0f, 18.0f});
    drawText(m_status, {8.0f, static_cast<float>(height()) - 22.0f, 620.0f, 16.0f}, {255, 235, 160, 255});
}

void Scene_Editor::sRenderConfirmation()
{
    m_game->render().fillRect(screenRect({0.0f, 0.0f, static_cast<float>(width()), static_cast<float>(height())}), {0, 0, 0, 170});
    m_game->render().fillRect(screenRect({130.0f, 130.0f, 380.0f, 94.0f}), PanelColor);
    m_game->render().drawRect(screenRect({130.0f, 130.0f, 380.0f, 94.0f}), {255, 235, 160, 255});
    drawText("Save changes before leaving the editor?", {150.0f, 148.0f, 340.0f, 20.0f});
    drawButton({175.0f, 184.0f, 100.0f, 22.0f}, "SAVE & EXIT");
    drawButton({290.0f, 184.0f, 90.0f, 22.0f}, "DISCARD");
    drawButton({395.0f, 184.0f, 75.0f, 22.0f}, "CANCEL");
}

void Scene_Editor::sRender()
{
    if (m_view == EditorView::LayoutPicker) {
        sRenderPicker();
    }
    else {
        sRenderWorkspace();
    }
    if (m_confirmExit) {
        sRenderConfirmation();
    }
}

void Scene_Editor::clearWorld()
{
    std::vector<EntityID> entities;
    for (auto [entity, transform] : m_ECS.View<CTransform>()) {
        entities.push_back(entity);
    }
    for (const EntityID entity : entities) {
        m_ECS.queueRemoveEntity(entity);
    }
    m_ECS.update();
    m_rendererManager.update();
    m_selectedEntity.reset();
    m_dragging = false;
}

EntityID Scene_Editor::spawnPreview(const LayoutPlacement& placement)
{
    const EditorEntityDefinition* definition = m_catalog.find(placement.definition);
    if (!definition) {
        return static_cast<EntityID>(-1);
    }
    const EntityID entity = m_ECS.addEntity();
    m_ECS.addComponent<CName>(entity, definition->displayName);
    m_ECS.addComponent<CEditorPlacement>(entity, placement.definition, placement.x, placement.y);
    addVisual(entity, definition->spriteName, definition->renderLayer, true);
    const Vec2 pixelPosition{
        static_cast<float>(placement.x) * m_gridSize.x,
        static_cast<float>(placement.y) * m_gridSize.y
    };
    m_ECS.addComponent<CTransform>(entity, gridToMidPixel(pixelPosition, entity));
    spawnPreviewShadow(entity, *definition);
    return entity;
}

EntityID Scene_Editor::spawnPreviewShadow(EntityID parentID, const EditorEntityDefinition& definition)
{
    const CTransform& parentTransform = m_ECS.getComponent<CTransform>(parentID);
    const CSprite& parentSprite = m_ECS.getComponent<CSprite>(parentID);
    const SpriteDefinition& shadowSprite = getSprite("shadow");
    const Vec2 parentSize = parentSprite.size() * parentTransform.scale;
    const float shadowScale = parentSize.x / shadowSprite.frameSize().x * definition.shadowScale;
    if (shadowScale == 0.0f) {
        return static_cast<EntityID>(-1);
    }

    const Vec2 shadowSize = shadowSprite.frameSize() * shadowScale;
    const Vec2 relativePosition{
        definition.shadowOffset.x,
        parentSize.y / 2.0f - shadowSize.y / 2.0f + definition.shadowOffset.y
    };
    const EntityID shadowID = m_ECS.addEntity();
    m_ECS.addComponent<CTransform>(shadowID);
    m_ECS.getComponent<CTransform>(shadowID).scale = {shadowScale, shadowScale};
    addSprite(shadowID, "shadow", parentSprite.layer - 1);
    m_ECS.attachChild(parentID, shadowID, relativePosition);
    return shadowID;
}

void Scene_Editor::openLayout(const std::string& id)
{
    if (!m_ready) {
        return;
    }
    try {
        const LayoutInfo info = m_layouts.layout(id);
        const PixelImage image = m_game->loadImagePixels(info.terrainPath);
        if (image.width <= 0 || image.height <= 0) {
            throw std::runtime_error("Could not load terrain image: " + info.terrainPath);
        }

        clearWorld();
        m_levelLoader = LevelLoader(this, m_gridSize, image, false);
        m_camera.calibrate(Vec2{static_cast<float>(width()), static_cast<float>(height())}, m_levelLoader.getLevelSize(), m_gridSize);
        m_camera.position = scenePlayStartPosition() - m_camera.getScreenSize() / 2.0f;
        const Vec2 worldSize = m_levelLoader.getWorldSize();
        const RenderView view = worldRenderView();
        const float viewScale = std::max(1.0f, view.scale);
        const float visibleWidth = static_cast<float>(m_game->getWidth()) / viewScale;
        const float visibleHeight = static_cast<float>(m_game->getHeight()) / viewScale;
        m_camera.position.x = std::clamp(m_camera.position.x, 0.0f, std::max(0.0f, worldSize.x - visibleWidth));
        m_camera.position.y = std::clamp(m_camera.position.y, 0.0f, std::max(0.0f, worldSize.y - visibleHeight));
        m_currentLayout = info;
        m_unresolvedPlacements.clear();
        for (const LayoutPlacement& placement : m_layouts.loadLayout(info).placements) {
            if (!isValidGrid(Vec2{placement.x, placement.y}) || spawnPreview(placement) == static_cast<EntityID>(-1)) {
                m_unresolvedPlacements.push_back(placement);
            }
        }
        m_view = EditorView::Workspace;
        m_mode = EditMode::Select;
        m_drawDrawGrid = true;
        m_dirty = false;
        m_status = m_unresolvedPlacements.empty()
            ? "Layout loaded at the Scene_Play start position."
            : "Some unknown or out-of-bounds placements are preserved but not shown.";
    }
    catch (const std::exception& exception) {
        m_status = std::string("Could not open layout: ") + exception.what();
        std::cerr << m_status << std::endl;
    }
}

void Scene_Editor::createLayout()
{
    try {
        const LayoutInfo info = m_layouts.createEmptyLayout();
        m_status = "Created " + info.displayName;
        openLayout(info.id);
    }
    catch (const std::exception& exception) {
        m_status = std::string("Could not create layout: ") + exception.what();
    }
}

void Scene_Editor::duplicateLayout(const std::string& id)
{
    try {
        const LayoutInfo info = m_layouts.duplicateLayout(id);
        m_status = "Duplicated " + info.displayName;
        openLayout(info.id);
    }
    catch (const std::exception& exception) {
        m_status = std::string("Could not duplicate layout: ") + exception.what();
    }
}

void Scene_Editor::deleteLayout(const std::string& id)
{
    try {
        m_layouts.deleteLayout(id);
        m_status = "Deleted layout " + id;
    }
    catch (const std::exception& exception) {
        m_status = std::string("Could not delete layout: ") + exception.what();
    }
}

void Scene_Editor::setActiveLayout(const std::string& id)
{
    try {
        m_layouts.setActiveLayout(id);
        m_status = m_layouts.layout(id).displayName + " is now active in Scene_Play.";
    }
    catch (const std::exception& exception) {
        m_status = std::string("Could not activate layout: ") + exception.what();
    }
}

WorldLayout Scene_Editor::buildLayout() const
{
    WorldLayout layout;
    layout.placements = m_unresolvedPlacements;
    for (auto [entity, placement, transform] : m_ECS.constView<CEditorPlacement, CTransform>()) {
        layout.placements.push_back(LayoutPlacement{
            placement.definition,
            placement.x,
            placement.y
        });
    }
    std::sort(layout.placements.begin(), layout.placements.end(), [](const LayoutPlacement& lhs, const LayoutPlacement& rhs) {
        if (lhs.definition != rhs.definition) return lhs.definition < rhs.definition;
        if (lhs.y != rhs.y) return lhs.y < rhs.y;
        return lhs.x < rhs.x;
    });
    return layout;
}

void Scene_Editor::saveCurrentLayout()
{
    if (m_view != EditorView::Workspace) {
        return;
    }
    try {
        m_layouts.saveLayout(m_currentLayout, buildLayout());
        m_dirty = false;
        m_status = "Saved " + m_currentLayout.displayName;
    }
    catch (const std::exception& exception) {
        m_status = std::string("Could not save layout: ") + exception.what();
    }
}

Vec2 Scene_Editor::screenToWorld(Vec2 screenPoint)
{
    const RenderView view = worldRenderView();
    const float windowScale = static_cast<float>(m_game->getScale());
    return {
        (screenPoint.x * windowScale - view.originX) / view.scale + view.cameraX,
        (screenPoint.y * windowScale - view.originY) / view.scale + view.cameraY
    };
}

Vec2 Scene_Editor::snappedGrid(Vec2 worldPoint) const
{
    return {
        std::floor(worldPoint.x / m_gridSize.x),
        std::floor(worldPoint.y / m_gridSize.y)
    };
}

bool Scene_Editor::isValidGrid(Vec2 gridPoint) const
{
    const Vec2 levelSize = m_levelLoader.getLevelSize();
    return gridPoint.x >= 0.0f && gridPoint.y >= 0.0f &&
        gridPoint.x < levelSize.x && gridPoint.y < levelSize.y;
}

std::optional<EntityID> Scene_Editor::previewAt(Vec2 worldPoint) const
{
    std::optional<EntityID> hit;
    int topLayer = -1;
    for (auto [entity, placement, transform, sprite] : m_ECS.constView<CEditorPlacement, CTransform, CSprite>()) {
        const Vec2 size = sprite.size() * transform.scale;
        const RectF bounds{
            transform.pos.x - size.x / 2.0f,
            transform.pos.y - size.y / 2.0f,
            size.x,
            size.y
        };
        if (contains(bounds, worldPoint) && sprite.layer >= topLayer) {
            hit = entity;
            topLayer = sprite.layer;
        }
    }
    return hit;
}

void Scene_Editor::placeAt(Vec2 worldPoint)
{
    if (m_selectedDefinition.empty()) {
        m_status = "Choose an entity from the palette before placing.";
        return;
    }
    const Vec2 grid = snappedGrid(worldPoint);
    if (!isValidGrid(grid)) {
        m_status = "Placement must stay inside the terrain bounds.";
        return;
    }
    const EntityID entity = spawnPreview(LayoutPlacement{
        m_selectedDefinition,
        static_cast<int>(grid.x),
        static_cast<int>(grid.y)
    });
    if (entity == static_cast<EntityID>(-1)) {
        m_status = "Selected definition is not available.";
        return;
    }
    m_selectedEntity = entity;
    m_dirty = true;
    m_status = "Placed " + m_selectedDefinition;
}

void Scene_Editor::deleteAt(Vec2 worldPoint)
{
    const std::optional<EntityID> hit = previewAt(worldPoint);
    if (!hit) {
        return;
    }
    m_ECS.queueRemoveEntity(*hit);
    if (m_selectedEntity == hit) {
        m_selectedEntity.reset();
    }
    m_dirty = true;
    m_status = "Removed entity.";
}

void Scene_Editor::updateCamera()
{
    Vec2 direction{0.0f, 0.0f};
    if (m_cameraUp) direction.y -= 1.0f;
    if (m_cameraDown) direction.y += 1.0f;
    if (m_cameraLeft) direction.x -= 1.0f;
    if (m_cameraRight) direction.x += 1.0f;
    if (direction.isNull()) {
        return;
    }
    m_camera.position += direction.norm(EditorCameraSpeed);
    const Vec2 worldSize = m_levelLoader.getWorldSize();
    const RenderView view = worldRenderView();
    const float viewScale = std::max(1.0f, view.scale);
    const float visibleWidth = static_cast<float>(m_game->getWidth()) / viewScale;
    const float visibleHeight = static_cast<float>(m_game->getHeight()) / viewScale;
    m_camera.position.x = std::clamp(m_camera.position.x, 0.0f, std::max(0.0f, worldSize.x - visibleWidth));
    m_camera.position.y = std::clamp(m_camera.position.y, 0.0f, std::max(0.0f, worldSize.y - visibleHeight));
}

void Scene_Editor::updateDrag()
{
    if (!m_leftHeld || !m_dragging || !m_selectedEntity || !m_ECS.isAlive(*m_selectedEntity)) {
        return;
    }
    const Vec2 grid = snappedGrid(screenToWorld(m_mousePosition));
    if (!isValidGrid(grid)) {
        return;
    }
    CTransform& transform = m_ECS.getComponent<CTransform>(*m_selectedEntity);
    const Vec2 target = gridToMidPixel(grid * m_gridSize, *m_selectedEntity);
    if (transform.pos != target) {
        transform.pos = target;
        transform.prevPos = target;
        CEditorPlacement& placement = m_ECS.getComponent<CEditorPlacement>(*m_selectedEntity);
        placement.x = static_cast<int>(grid.x);
        placement.y = static_cast<int>(grid.y);
        m_dirty = true;
    }
}

void Scene_Editor::handlePickerClick(Vec2 point)
{
    if (!m_ready) {
        return;
    }
    if (contains({24.0f, 46.0f, 110.0f, 22.0f}, point)) {
        createLayout();
        return;
    }

    const std::vector<LayoutInfo> layouts = m_layouts.layouts();
    float y = 84.0f;
    for (const LayoutInfo& info : layouts) {
        if (contains({342.0f, y + 3.0f, 58.0f, 20.0f}, point)) {
            openLayout(info.id);
            return;
        }
        if (contains({404.0f, y + 3.0f, 58.0f, 20.0f}, point)) {
            setActiveLayout(info.id);
            return;
        }
        if (contains({466.0f, y + 3.0f, 66.0f, 20.0f}, point)) {
            duplicateLayout(info.id);
            return;
        }
        if (contains({536.0f, y + 3.0f, 72.0f, 20.0f}, point)) {
            deleteLayout(info.id);
            return;
        }
        y += 30.0f;
    }
}

void Scene_Editor::handleWorkspaceClick(Vec2 point)
{
    if (m_confirmExit) {
        if (contains({175.0f, 184.0f, 100.0f, 22.0f}, point)) {
            saveCurrentLayout();
            if (!m_dirty) returnToMenu();
        }
        else if (contains({290.0f, 184.0f, 90.0f, 22.0f}, point)) {
            m_dirty = false;
            returnToMenu();
        }
        else if (contains({395.0f, 184.0f, 75.0f, 22.0f}, point)) {
            m_confirmExit = false;
        }
        return;
    }

    if (point.y < ToolbarHeight) {
        if (contains({8.0f, 7.0f, 68.0f, 20.0f}, point)) m_mode = EditMode::Select;
        else if (contains({80.0f, 7.0f, 62.0f, 20.0f}, point)) m_mode = EditMode::Place;
        else if (contains({146.0f, 7.0f, 66.0f, 20.0f}, point)) m_mode = EditMode::Delete;
        else if (contains({220.0f, 7.0f, 52.0f, 20.0f}, point)) saveCurrentLayout();
        else if (contains({276.0f, 7.0f, 66.0f, 20.0f}, point)) requestExit();
        else if (contains({8.0f, 35.0f, 100.0f, 18.0f}, point)) {
            m_selectedCategory = EditorEntityCategory::Character;
            m_paletteOffset = 0;
        }
        else if (contains({112.0f, 35.0f, 70.0f, 18.0f}, point)) {
            m_selectedCategory = EditorEntityCategory::Item;
            m_paletteOffset = 0;
        }
        else if (contains({186.0f, 35.0f, 96.0f, 18.0f}, point)) {
            m_selectedCategory = EditorEntityCategory::Structure;
            m_paletteOffset = 0;
        }
        else if (point.y >= 60.0f && point.y <= 80.0f) {
            const int index = static_cast<int>((point.x - 8.0f) / 90.0f);
            const std::vector<EditorEntityDefinition>& entries = m_catalog.category(m_selectedCategory);
            const int selectedIndex = m_paletteOffset + index;
            if (index >= 0 && index < 7 && selectedIndex >= 0 && selectedIndex < static_cast<int>(entries.size())) {
                m_selectedDefinition = entries[selectedIndex].id;
                m_mode = EditMode::Place;
                m_status = "Selected " + entries[selectedIndex].displayName;
            }
        }
        return;
    }

    const Vec2 worldPoint = screenToWorld(point);
    if (m_mode == EditMode::Place) {
        placeAt(worldPoint);
    }
    else if (m_mode == EditMode::Delete) {
        deleteAt(worldPoint);
    }
    else {
        m_selectedEntity = previewAt(worldPoint);
        m_dragging = m_selectedEntity.has_value();
        m_status = m_selectedEntity ? "Drag the selected entity to move it." : "Nothing selected.";
    }
}

void Scene_Editor::handleWorkspaceRightClick(Vec2 point)
{
    if (!m_confirmExit && point.y >= ToolbarHeight) {
        deleteAt(screenToWorld(point));
    }
}

void Scene_Editor::requestExit()
{
    if (m_view == EditorView::LayoutPicker) {
        returnToMenu();
    }
    else if (m_dirty) {
        m_confirmExit = true;
    }
    else {
        returnToMenu();
    }
}

void Scene_Editor::returnToMenu()
{
    m_game->changeScene("MENU", std::make_shared<Scene_Menu>(m_game), true);
}

void Scene_Editor::onEnd()
{
    requestExit();
}

void Scene_Editor::sDoAction(const Action& action)
{
    if (action.type() == "START") {
        if (action.name() == "PRIMARY") {
            m_leftHeld = true;
            if (m_confirmExit) handleWorkspaceClick(m_mousePosition);
            else if (m_view == EditorView::LayoutPicker) handlePickerClick(m_mousePosition);
            else handleWorkspaceClick(m_mousePosition);
        }
        else if (action.name() == "SECONDARY" && m_view == EditorView::Workspace) handleWorkspaceRightClick(m_mousePosition);
        else if (action.name() == "UP") m_cameraUp = true;
        else if (action.name() == "DOWN") m_cameraDown = true;
        else if (action.name() == "LEFT") m_cameraLeft = true;
        else if (action.name() == "RIGHT") m_cameraRight = true;
        else if (action.name() == "SAVE") saveCurrentLayout();
        else if (action.name() == "ESC") requestExit();
        else if (action.name() == "DELETE" && m_selectedEntity && m_ECS.isAlive(*m_selectedEntity)) {
            m_ECS.queueRemoveEntity(*m_selectedEntity);
            m_selectedEntity.reset();
            m_dirty = true;
        }
        else if (action.name() == "TOGGLE_GRID") m_drawDrawGrid = !m_drawDrawGrid;
        else if (action.name() == "ZOOM_IN") m_camera.stepCameraZoom(-1, m_game->getScale());
        else if (action.name() == "ZOOM_OUT") m_camera.stepCameraZoom(1, m_game->getScale());
    }
    else if (action.type() == "END") {
        if (action.name() == "PRIMARY") {
            m_leftHeld = false;
            m_dragging = false;
        }
        else if (action.name() == "UP") m_cameraUp = false;
        else if (action.name() == "DOWN") m_cameraDown = false;
        else if (action.name() == "LEFT") m_cameraLeft = false;
        else if (action.name() == "RIGHT") m_cameraRight = false;
    }
    else if (action.name() == "PALETTE_SCROLL" && m_view == EditorView::Workspace && m_mousePosition.y < ToolbarHeight) {
        const int size = static_cast<int>(m_catalog.category(m_selectedCategory).size());
        m_paletteOffset = std::clamp(m_paletteOffset - getMouseState().scroll, 0, std::max(0, size - 7));
    }
}

void Scene_Editor::update()
{
    if (m_view == EditorView::Workspace) {
        updateCamera();
        updateDrag();
        const RenderView view = worldRenderView();
        const float viewScale = std::max(1.0f, view.scale);
        const Vec2 focus = m_camera.position + Vec2{
            static_cast<float>(m_game->getWidth()) / (2.0f * viewScale),
            static_cast<float>(m_game->getHeight()) / (2.0f * viewScale)
        };
        m_levelLoader.update(focus);
        updateAnimations();
        ++m_currentFrame;
    }
    sRender();
    m_ECS.update();
    m_rendererManager.update();
}

Vec2 Scene_Editor::getCameraPosition()
{
    return m_camera.position;
}
