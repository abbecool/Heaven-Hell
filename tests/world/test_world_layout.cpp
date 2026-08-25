#include "TestSupport.hpp"
#include "world/WorldLayout.hpp"

#include <array>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

using TestSupport::require;

class ScopedWorkspace
{
public:
    ScopedWorkspace()
    {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() / ("heavenhell-layout-test-" + std::to_string(suffix));
        std::filesystem::create_directories(path / "layouts");
    }

    ~ScopedWorkspace()
    {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }

    std::filesystem::path path;
};

class ScopedCurrentPath
{
public:
    explicit ScopedCurrentPath(const std::filesystem::path& next)
        : previous(std::filesystem::current_path())
    {
        std::filesystem::current_path(next);
    }

    ~ScopedCurrentPath()
    {
        std::error_code error;
        std::filesystem::current_path(previous, error);
    }

private:
    std::filesystem::path previous;
};

void writeFile(const std::filesystem::path& path, const std::string& contents)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Could not create test file");
    }
    file << contents;
}

void writeRegistry(const ScopedWorkspace& workspace, const std::string& active = "first")
{
    writeFile(workspace.path / "levels.json", R"({
  "version": 1,
  "activeLayoutId": ")" + active + R"(",
  "layouts": [
    {"id":"first","displayName":"First","terrainPath":"terrain.png","placementPath":")" +
        (workspace.path / "layouts/first.json").string() + R"("},
    {"id":"second","displayName":"Second","terrainPath":"terrain.png","placementPath":")" +
        (workspace.path / "layouts/second.json").string() + R"("}
  ]
})");
    writeFile(workspace.path / "layouts/first.json", R"({"version":1,"placements":[{"definition":"tree","x":3,"y":4}]})");
    writeFile(workspace.path / "layouts/second.json", R"({"version":1,"placements":[{"definition":"goblin","x":7,"y":8}]})");
}

void testActiveLayoutPersists()
{
    ScopedWorkspace workspace;
    writeRegistry(workspace);

    LayoutRepository repository(workspace.path / "levels.json");
    repository.load();
    repository.setActiveLayout("second");

    LayoutRepository reloaded(workspace.path / "levels.json");
    reloaded.load();
    require(reloaded.activeLayout().id == "second", "active layout was not persisted");
    require(reloaded.loadLayout(reloaded.activeLayout()).placements.front().definition == "goblin",
        "active layout did not load its placements");
}

void testSaveAndReloadLayout()
{
    ScopedWorkspace workspace;
    writeRegistry(workspace);

    LayoutRepository repository(workspace.path / "levels.json");
    repository.load();
    WorldLayout layout;
    layout.placements = {
        {"tree", 10, 12},
        {"coin", 14, 15}
    };
    repository.saveLayout(repository.layout("first"), layout);

    const WorldLayout loaded = repository.loadLayout("first");
    require(loaded.placements.size() == 2, "saved placement count changed");
    require(loaded.placements[0].definition == "tree" && loaded.placements[0].x == 10 && loaded.placements[0].y == 12,
        "first placement changed after reload");
    require(loaded.placements[1].definition == "coin" && loaded.placements[1].x == 14 && loaded.placements[1].y == 15,
        "second placement changed after reload");
}

void testInvalidPlacementIsRejected()
{
    ScopedWorkspace workspace;
    writeRegistry(workspace);
    writeFile(workspace.path / "layouts/first.json", R"({"version":1,"placements":[{"definition":"tree","x":"bad","y":4}]})");

    LayoutRepository repository(workspace.path / "levels.json");
    repository.load();
    bool rejected = false;
    try {
        (void)repository.loadLayout("first");
    }
    catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "invalid placement was accepted");
}

void testDefaultLayoutMigration()
{
    const std::filesystem::path sourceRoot = HEAVENHELL_SOURCE_DIR;
    const ScopedCurrentPath sourceDirectory(sourceRoot);
    LayoutRepository repository;
    repository.load();
    const WorldLayout layout = repository.loadLayout(repository.activeLayout());
    require(!layout.placements.empty(), "default layout has no placements");
}

constexpr std::array Tests = {
    TestSupport::TestCase{"active_layout_persists", testActiveLayoutPersists},
    TestSupport::TestCase{"save_and_reload", testSaveAndReloadLayout},
    TestSupport::TestCase{"invalid_placement_rejected", testInvalidPlacementIsRejected},
    TestSupport::TestCase{"default_layout_migration", testDefaultLayoutMigration}
};

} // namespace

int main(int argc, char* argv[])
{
    return TestSupport::runNamedTest(argc, argv, Tests);
}
