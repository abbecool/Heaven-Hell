#include "world/EntityCatalog.hpp"

#include "ecs/Components.hpp"
#include "external/json.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

namespace {

std::vector<std::filesystem::path> jsonFiles(const std::string& directory)
{
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

json readJsonFile(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not load entity definition: " + path.string());
    }
    json data;
    file >> data;
    return data;
}

void sortDefinitions(std::vector<EditorEntityDefinition>& definitions)
{
    std::sort(definitions.begin(), definitions.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.displayName < rhs.displayName;
    });
}

} // namespace

EntityCatalog::EntityCatalog(std::string mobsPath, std::string entitiesPath, std::string itemsPath)
    : m_mobsPath(std::move(mobsPath))
    , m_entitiesPath(std::move(entitiesPath))
    , m_itemsPath(std::move(itemsPath))
{
}

void EntityCatalog::addDefinition(EditorEntityDefinition definition)
{
    if (find(definition.id)) {
        throw std::runtime_error("Duplicate editor entity definition: " + definition.id);
    }
    m_definitions.push_back(definition);
    switch (definition.category) {
    case EditorEntityCategory::Character: m_characters.push_back(std::move(definition)); break;
    case EditorEntityCategory::Item: m_items.push_back(std::move(definition)); break;
    case EditorEntityCategory::Structure: m_structures.push_back(std::move(definition)); break;
    }
}

void EntityCatalog::load()
{
    m_definitions.clear();
    m_characters.clear();
    m_items.clear();
    m_structures.clear();

    const auto addJsonDefinitions = [this](const std::string& directory, EditorEntityCategory category) {
        for (const std::filesystem::path& path : jsonFiles(directory)) {
            const std::string id = path.stem().string();
            if (id == "player" || id == "item") {
                continue;
            }
            const json data = readJsonFile(path);
            if (!data.contains(id) || !data.at(id).contains("components")) {
                throw std::runtime_error("Invalid entity definition: " + path.string());
            }
            const json& definition = data.at(id);
            const json& components = definition.at("components");
            if (!components.contains("CAnimation")) {
                continue;
            }
            const json& animation = components.at("CAnimation");
            if (!animation.contains("animation") || !animation.contains("layer")) {
                throw std::runtime_error("Visual definition is incomplete: " + path.string());
            }
            const CShadow shadow = components.contains("CShadow")
                ? CShadow(components.at("CShadow"))
                : CShadow{};
            addDefinition(EditorEntityDefinition{
                id,
                components.value("CName", id),
                animation.at("animation").get<std::string>(),
                renderLayerFromJson(animation.at("layer")),
                shadow.scale,
                shadow.offset,
                category
            });
        }
    };

    addJsonDefinitions(m_mobsPath, EditorEntityCategory::Character);
    addJsonDefinitions(m_entitiesPath, EditorEntityCategory::Structure);

    for (const std::filesystem::path& path : jsonFiles(m_itemsPath)) {
        const std::string id = path.stem().string();
        const json data = readJsonFile(path);
        if (!data.contains(id)) {
            throw std::runtime_error("Invalid item definition: " + path.string());
        }
        const json& item = data.at(id);
        const std::string spriteName = item.value("iconPath", "");
        if (spriteName.empty()) {
            continue;
        }
        const CShadow shadow = item.contains("shadow")
            ? CShadow(item.at("shadow"))
            : CShadow{};
        addDefinition(EditorEntityDefinition{
            id,
            item.value("name", id),
            spriteName,
            RenderLayer::GroundItem,
            shadow.scale,
            shadow.offset,
            EditorEntityCategory::Item
        });
    }

    sortDefinitions(m_definitions);
    sortDefinitions(m_characters);
    sortDefinitions(m_items);
    sortDefinitions(m_structures);
}

const std::vector<EditorEntityDefinition>& EntityCatalog::all() const
{
    return m_definitions;
}

const std::vector<EditorEntityDefinition>& EntityCatalog::category(EditorEntityCategory selectedCategory) const
{
    switch (selectedCategory) {
    case EditorEntityCategory::Character: return m_characters;
    case EditorEntityCategory::Item: return m_items;
    case EditorEntityCategory::Structure: return m_structures;
    }
    return m_structures;
}

const EditorEntityDefinition* EntityCatalog::find(const std::string& id) const
{
    const auto it = std::find_if(m_definitions.begin(), m_definitions.end(), [&id](const auto& definition) {
        return definition.id == id;
    });
    return it == m_definitions.end() ? nullptr : &*it;
}
