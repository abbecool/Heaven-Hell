#pragma once

#include "physics/Vec2.hpp"

#include <string>
#include <vector>

enum class EditorEntityCategory
{
    Character,
    Item,
    Structure
};

struct EditorEntityDefinition
{
    std::string id;
    std::string displayName;
    std::string spriteName;
    int renderLayer = 0;
    float shadowScale = 1.0f;
    Vec2 shadowOffset = {0, 0};
    EditorEntityCategory category = EditorEntityCategory::Structure;
};

class EntityCatalog
{
public:
    EntityCatalog(
        std::string mobsPath = "config_files/mobs",
        std::string entitiesPath = "config_files/entities",
        std::string itemsPath = "config_files/items"
    );

    void load();
    const std::vector<EditorEntityDefinition>& all() const;
    const std::vector<EditorEntityDefinition>& category(EditorEntityCategory category) const;
    const EditorEntityDefinition* find(const std::string& id) const;

private:
    std::string m_mobsPath;
    std::string m_entitiesPath;
    std::string m_itemsPath;
    std::vector<EditorEntityDefinition> m_definitions;
    std::vector<EditorEntityDefinition> m_characters;
    std::vector<EditorEntityDefinition> m_items;
    std::vector<EditorEntityDefinition> m_structures;

    void addDefinition(EditorEntityDefinition definition);
};
