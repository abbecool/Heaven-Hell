#include "TestSupport.hpp"
#include "world/EntityCatalog.hpp"

#include <array>
#include <filesystem>

namespace {

using TestSupport::require;

void testCatalogFindsPlaceableDefinitions()
{
    const std::filesystem::path sourceRoot = HEAVENHELL_SOURCE_DIR;
    EntityCatalog catalog(
        (sourceRoot / "config_files/mobs").string(),
        (sourceRoot / "config_files/entities").string(),
        (sourceRoot / "config_files/items").string()
    );
    catalog.load();

    const EditorEntityDefinition* goblin = catalog.find("goblin");
    const EditorEntityDefinition* coin = catalog.find("coin");
    const EditorEntityDefinition* tree = catalog.find("tree");
    require(goblin && goblin->category == EditorEntityCategory::Character, "goblin is missing from character palette");
    require(coin && coin->category == EditorEntityCategory::Item, "coin is missing from item palette");
    require(tree && tree->category == EditorEntityCategory::Structure, "tree is missing from structure palette");
    require(catalog.find("player") == nullptr, "player should not be placeable from the editor palette");
}

constexpr std::array Tests = {
    TestSupport::TestCase{"placeable_definitions", testCatalogFindsPlaceableDefinitions}
};

} // namespace

int main(int argc, char* argv[])
{
    return TestSupport::runNamedTest(argc, argv, Tests);
}
