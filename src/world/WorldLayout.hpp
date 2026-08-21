#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct LayoutPlacement
{
    std::string definition;
    int x = 0;
    int y = 0;
};

struct WorldLayout
{
    int version = 1;
    std::vector<LayoutPlacement> placements;
};

struct LayoutInfo
{
    std::string id;
    std::string displayName;
    std::string terrainPath;
    std::string placementPath;
};

class LayoutRepository
{
public:
    explicit LayoutRepository(std::filesystem::path registryPath = "config_files/levels.json");

    void load();
    const std::vector<LayoutInfo>& layouts() const;
    const LayoutInfo& activeLayout() const;
    const LayoutInfo& layout(const std::string& id) const;

    WorldLayout loadLayout(const LayoutInfo& info) const;
    WorldLayout loadLayout(const std::string& id) const;
    void saveLayout(const LayoutInfo& info, const WorldLayout& layout) const;

    void setActiveLayout(const std::string& id);
    LayoutInfo createEmptyLayout();
    LayoutInfo duplicateLayout(const std::string& sourceId);
    void deleteLayout(const std::string& id);

private:
    std::filesystem::path m_registryPath;
    bool m_mirrorToSource = false;
    std::vector<LayoutInfo> m_layouts;
    std::string m_activeLayoutId;
    bool m_loaded = false;

    void saveRegistry() const;
    std::string nextLayoutId() const;
    const LayoutInfo* find(const std::string& id) const;
    static void writeJsonFile(const std::filesystem::path& path, const std::string& contents);
    void writeJsonFileAndMirror(const std::filesystem::path& path, const std::string& contents) const;
    void removeLayoutFileAndMirror(const std::filesystem::path& path) const;
    std::filesystem::path sourceMirrorPath(const std::filesystem::path& path) const;
};
