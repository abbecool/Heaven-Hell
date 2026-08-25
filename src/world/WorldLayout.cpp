#include "world/WorldLayout.hpp"

#include "external/json.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <unordered_set>

using json = nlohmann::json;

#ifndef HEAVENHELL_SOURCE_DIR
#define HEAVENHELL_SOURCE_DIR ""
#endif

namespace {

json readJsonFile(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open JSON file: " + path.string());
    }

    json data;
    file >> data;
    return data;
}

void requireString(const json& value, const char* field, const std::filesystem::path& source)
{
    if (!value.contains(field) || !value.at(field).is_string() || value.at(field).get<std::string>().empty()) {
        throw std::runtime_error(
            "Missing or invalid '" + std::string(field) + "' in " + source.string()
        );
    }
}

} // namespace

LayoutRepository::LayoutRepository(std::filesystem::path registryPath)
    : m_registryPath(std::move(registryPath))
    , m_mirrorToSource(m_registryPath == std::filesystem::path("config_files/levels.json"))
{
}

void LayoutRepository::load()
{
    const json registry = readJsonFile(m_registryPath);
    if (!registry.is_object() || registry.value("version", 0) != 1) {
        throw std::runtime_error("Unsupported level registry: " + m_registryPath.string());
    }
    requireString(registry, "activeLayoutId", m_registryPath);
    if (!registry.contains("layouts") || !registry.at("layouts").is_array() || registry.at("layouts").empty()) {
        throw std::runtime_error("Level registry must contain at least one layout: " + m_registryPath.string());
    }

    std::unordered_set<std::string> ids;
    std::vector<LayoutInfo> layouts;
    for (const json& entry : registry.at("layouts")) {
        if (!entry.is_object()) {
            throw std::runtime_error("Invalid layout entry in " + m_registryPath.string());
        }
        requireString(entry, "id", m_registryPath);
        requireString(entry, "displayName", m_registryPath);
        requireString(entry, "terrainPath", m_registryPath);
        requireString(entry, "placementPath", m_registryPath);

        LayoutInfo info{
            entry.at("id").get<std::string>(),
            entry.at("displayName").get<std::string>(),
            entry.at("terrainPath").get<std::string>(),
            entry.at("placementPath").get<std::string>()
        };
        if (!ids.insert(info.id).second) {
            throw std::runtime_error("Duplicate layout id in " + m_registryPath.string() + ": " + info.id);
        }
        layouts.push_back(std::move(info));
    }

    const std::string activeLayoutId = registry.at("activeLayoutId").get<std::string>();
    if (std::none_of(layouts.begin(), layouts.end(), [&activeLayoutId](const LayoutInfo& info) {
        return info.id == activeLayoutId;
    })) {
        throw std::runtime_error("Active layout does not exist: " + activeLayoutId);
    }

    m_layouts = std::move(layouts);
    m_activeLayoutId = activeLayoutId;
    m_loaded = true;
}

const std::vector<LayoutInfo>& LayoutRepository::layouts() const
{
    if (!m_loaded) {
        throw std::logic_error("LayoutRepository::load() must be called before layouts()");
    }
    return m_layouts;
}

const LayoutInfo* LayoutRepository::find(const std::string& id) const
{
    const auto it = std::find_if(m_layouts.begin(), m_layouts.end(), [&id](const LayoutInfo& info) {
        return info.id == id;
    });
    return it == m_layouts.end() ? nullptr : &*it;
}

const LayoutInfo& LayoutRepository::layout(const std::string& id) const
{
    if (!m_loaded) {
        throw std::logic_error("LayoutRepository::load() must be called before layout()");
    }
    const LayoutInfo* info = find(id);
    if (!info) {
        throw std::runtime_error("Unknown layout: " + id);
    }
    return *info;
}

const LayoutInfo& LayoutRepository::activeLayout() const
{
    return layout(m_activeLayoutId);
}

WorldLayout LayoutRepository::loadLayout(const LayoutInfo& info) const
{
    const std::filesystem::path path = info.placementPath;
    const json data = readJsonFile(path);
    if (!data.is_object() || data.value("version", 0) != 1 ||
        !data.contains("placements") || !data.at("placements").is_array()) {
        throw std::runtime_error("Invalid layout file: " + path.string());
    }

    WorldLayout layout;
    for (const json& entry : data.at("placements")) {
        if (!entry.is_object() || !entry.contains("definition") || !entry.at("definition").is_string() ||
            !entry.contains("x") || !entry.at("x").is_number_integer() ||
            !entry.contains("y") || !entry.at("y").is_number_integer()) {
            throw std::runtime_error("Invalid placement in " + path.string());
        }
        layout.placements.push_back(LayoutPlacement{
            entry.at("definition").get<std::string>(),
            entry.at("x").get<int>(),
            entry.at("y").get<int>()
        });
    }
    return layout;
}

WorldLayout LayoutRepository::loadLayout(const std::string& id) const
{
    return loadLayout(layout(id));
}

void LayoutRepository::writeJsonFile(const std::filesystem::path& path, const std::string& contents)
{
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        throw std::runtime_error("Could not create directory for " + path.string() + ": " + error.message());
    }

    const std::filesystem::path tempPath = path.string() + ".tmp";
    {
        std::ofstream file(tempPath, std::ios::trunc);
        if (!file) {
            throw std::runtime_error("Could not write temporary file: " + tempPath.string());
        }
        file << contents;
        if (!file) {
            throw std::runtime_error("Could not finish writing temporary file: " + tempPath.string());
        }
    }

    std::filesystem::rename(tempPath, path, error);
    if (!error) {
        return;
    }

    // Windows does not replace an existing file during rename. Removing only
    // after the replacement file has been written keeps interrupted saves safe.
    std::filesystem::remove(path, error);
    error.clear();
    std::filesystem::rename(tempPath, path, error);
    if (error) {
        std::filesystem::remove(tempPath);
        throw std::runtime_error("Could not replace " + path.string() + ": " + error.message());
    }
}

std::filesystem::path LayoutRepository::sourceMirrorPath(const std::filesystem::path& path) const
{
    if (!m_mirrorToSource || path.is_absolute() || std::string(HEAVENHELL_SOURCE_DIR).empty()) {
        return {};
    }

    const std::filesystem::path sourcePath = std::filesystem::path(HEAVENHELL_SOURCE_DIR) / path;
    std::error_code error;
    const std::filesystem::path runtimePath = std::filesystem::absolute(path, error).lexically_normal();
    if (error || runtimePath == sourcePath.lexically_normal()) {
        return {};
    }
    return sourcePath;
}

void LayoutRepository::writeJsonFileAndMirror(const std::filesystem::path& path, const std::string& contents) const
{
    writeJsonFile(path, contents);

    const std::filesystem::path mirrorPath = sourceMirrorPath(path);
    if (!mirrorPath.empty()) {
        writeJsonFile(mirrorPath, contents);
    }
}

void LayoutRepository::removeLayoutFileAndMirror(const std::filesystem::path& path) const
{
    std::error_code error;
    if (!std::filesystem::remove(path, error) && error) {
        throw std::runtime_error("Could not delete layout file " + path.string() + ": " + error.message());
    }

    const std::filesystem::path mirrorPath = sourceMirrorPath(path);
    if (!mirrorPath.empty()) {
        error.clear();
        if (!std::filesystem::remove(mirrorPath, error) && error) {
            throw std::runtime_error("Could not delete source layout file " + mirrorPath.string() + ": " + error.message());
        }
    }
}

void LayoutRepository::saveLayout(const LayoutInfo& info, const WorldLayout& layout) const
{
    json data = {
        {"version", 1},
        {"placements", json::array()}
    };
    for (const LayoutPlacement& placement : layout.placements) {
        data["placements"].push_back({
            {"definition", placement.definition},
            {"x", placement.x},
            {"y", placement.y}
        });
    }
    writeJsonFileAndMirror(info.placementPath, data.dump(4) + "\n");
}

void LayoutRepository::saveRegistry() const
{
    json data = {
        {"version", 1},
        {"activeLayoutId", m_activeLayoutId},
        {"layouts", json::array()}
    };
    for (const LayoutInfo& info : m_layouts) {
        data["layouts"].push_back({
            {"id", info.id},
            {"displayName", info.displayName},
            {"terrainPath", info.terrainPath},
            {"placementPath", info.placementPath}
        });
    }
    writeJsonFileAndMirror(m_registryPath, data.dump(4) + "\n");
}

void LayoutRepository::setActiveLayout(const std::string& id)
{
    (void)layout(id);
    m_activeLayoutId = id;
    saveRegistry();
}

std::string LayoutRepository::nextLayoutId() const
{
    for (int index = 1;; ++index) {
        std::ostringstream id;
        id << "layout_" << std::setw(3) << std::setfill('0') << index;
        if (!find(id.str())) {
            return id.str();
        }
    }
}

LayoutInfo LayoutRepository::createEmptyLayout()
{
    const LayoutInfo& templateLayout = activeLayout();
    const std::string id = nextLayoutId();
    LayoutInfo info{
        id,
        "Layout " + id.substr(id.find_last_of('_') + 1),
        templateLayout.terrainPath,
        "config_files/layouts/" + id + ".json"
    };
    saveLayout(info, WorldLayout{});
    m_layouts.push_back(info);
    saveRegistry();
    return info;
}

LayoutInfo LayoutRepository::duplicateLayout(const std::string& sourceId)
{
    const LayoutInfo source = layout(sourceId);
    createEmptyLayout();
    LayoutInfo& copy = m_layouts.back();
    copy.terrainPath = source.terrainPath;
    copy.displayName = source.displayName + " Copy";
    saveLayout(copy, loadLayout(source));
    saveRegistry();
    return copy;
}

void LayoutRepository::deleteLayout(const std::string& id)
{
    if (id == m_activeLayoutId) {
        throw std::runtime_error("Choose another active layout before deleting " + id);
    }
    if (m_layouts.size() <= 1) {
        throw std::runtime_error("At least one layout must remain");
    }

    const LayoutInfo info = layout(id);
    removeLayoutFileAndMirror(info.placementPath);
    m_layouts.erase(std::remove_if(m_layouts.begin(), m_layouts.end(), [&id](const LayoutInfo& candidate) {
        return candidate.id == id;
    }), m_layouts.end());
    saveRegistry();
}
