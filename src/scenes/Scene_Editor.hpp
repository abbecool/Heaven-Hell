#pragma once

#include "physics/Level_Loader.hpp"
#include "scenes/Scene.hpp"
#include "world/EntityCatalog.hpp"
#include "world/WorldLayout.hpp"

#include <optional>
#include <string>
#include <vector>

class Scene_Editor : public Scene
{
    enum class EditorView
    {
        LayoutPicker,
        Workspace
    };

    enum class EditMode
    {
        Select,
        Place,
        Delete
    };

    EditorView m_view = EditorView::LayoutPicker;
    EditMode m_mode = EditMode::Select;
    LayoutRepository m_layouts;
    EntityCatalog m_catalog;
    LevelLoader m_levelLoader;
    LayoutInfo m_currentLayout;
    std::vector<LayoutPlacement> m_unresolvedPlacements;
    std::optional<EntityID> m_selectedEntity;
    EditorEntityCategory m_selectedCategory = EditorEntityCategory::Character;
    std::string m_selectedDefinition;
    std::string m_status;
    int m_paletteOffset = 0;
    bool m_ready = false;
    bool m_dirty = false;
    bool m_leftHeld = false;
    bool m_dragging = false;
    bool m_confirmExit = false;
    bool m_cameraUp = false;
    bool m_cameraDown = false;
    bool m_cameraLeft = false;
    bool m_cameraRight = false;

    void sRender();
    void sRenderPicker();
    void sRenderWorkspace();
    void sRenderConfirmation();
    void sDoAction(const Action& action) override;
    void onEnd() override;

    void handlePickerClick(Vec2 point);
    void handleWorkspaceClick(Vec2 point);
    void handleWorkspaceRightClick(Vec2 point);
    void openLayout(const std::string& id);
    void createLayout();
    void duplicateLayout(const std::string& id);
    void deleteLayout(const std::string& id);
    void setActiveLayout(const std::string& id);
    void saveCurrentLayout();
    void requestExit();
    void returnToMenu();
    void clearWorld();

    EntityID spawnPreview(const LayoutPlacement& placement);
    EntityID spawnPreviewShadow(EntityID parentID, const EditorEntityDefinition& definition);
    std::optional<EntityID> previewAt(Vec2 worldPoint) const;
    Vec2 screenToWorld(Vec2 screenPoint);
    Vec2 snappedGrid(Vec2 worldPoint) const;
    bool isValidGrid(Vec2 gridPoint) const;
    void placeAt(Vec2 worldPoint);
    void deleteAt(Vec2 worldPoint);
    void updateCamera();
    void updateDrag();
    Vec2 scenePlayStartPosition() const;
    WorldLayout buildLayout() const;

    void drawButton(const RectF& rect, const std::string& label, bool active = false) const;
    void drawText(const std::string& text, RectF rect, Color color = {255, 255, 255, 255}) const;
    RectF screenRect(const RectF& virtualRect) const;
    static bool contains(const RectF& rect, Vec2 point);

public:
    explicit Scene_Editor(Game* game);

    void update() override;
    Vec2 getCameraPosition() override;
};
