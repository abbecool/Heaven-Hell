#include "TestSupport.hpp"
#include "ecs/ECS.hpp"
#include "physics/Quadtree.hpp"

#include <array>
#include <algorithm>
#include <string>
#include <type_traits>
#include <vector>

namespace {

using TestSupport::require;

struct TestComponent {
    int value = 0;
};

struct SecondaryComponent {
    int value = 0;
};

struct MissingExcludedComponent {
    int value = 0;
};

size_t countQuadtreeObjectOccurrences(Quadtree& tree, size_t objectIndex)
{
    size_t occurrences = 0;
    auto leaves = tree.createQuadtreeVector();
    for (const auto& leaf : leaves) {
        const auto objects = leaf->getObjects();
        occurrences += static_cast<size_t>(std::count(objects.begin(), objects.end(), objectIndex));
    }
    return occurrences;
}

void testPoolAddAndGet()
{
    ComponentPool<TestComponent> pool;
    pool.addComponent(1, TestComponent{42});

    require(pool.hasComponent(1), "added component was not present");
    require(pool.getComponent(1).value == 42, "added component had the wrong value");
    require(pool.getLength() == 1, "pool length was incorrect");
}

void testPoolReplaceExisting()
{
    ComponentPool<TestComponent> pool;
    pool.addComponent(1, TestComponent{1});
    pool.addComponent(1, TestComponent{2});

    require(pool.hasComponent(1), "replaced component was not present");
    require(pool.getComponent(1).value == 2, "replacement did not update the component");
    require(pool.getLength() == 1, "replacement created a duplicate component");
}

void testPoolEntityZeroComponentRemoval()
{
    ComponentPool<TestComponent> pool;
    pool.addComponent(0, TestComponent{1});
    pool.addComponent(0, TestComponent{2});

    require(pool.hasComponent(0), "entity 0 component was not present");
    require(pool.getComponent(0).value == 2, "entity 0 replacement did not update the component");
    require(pool.getLength() == 1, "entity 0 replacement created a duplicate component");

    pool.removeComponent(0);

    require(!pool.hasComponent(0), "entity 0 component removal was ignored");
    require(pool.getLength() == 0, "entity 0 component removal left stale dense data");
}

void testPoolRemoveCompacts()
{
    ComponentPool<TestComponent> pool;
    pool.addComponent(1, TestComponent{10});
    pool.addComponent(2, TestComponent{20});
    pool.removeComponent(1);

    require(!pool.hasComponent(1), "removed component was still present");
    require(pool.hasComponent(2), "component moved during compaction was lost");
    require(pool.getComponent(2).value == 20, "compacted component had the wrong value");
    require(pool.getLength() == 1, "pool length was incorrect after removal");
}

void testPoolMove()
{
    ComponentPool<TestComponent> pool;
    pool.addComponent(1, TestComponent{10});
    pool.addComponent(2, TestComponent{20});
    TestComponent& moved = pool.moveComponent(3, 1);

    require(moved.value == 10, "moved component had the wrong value");
    require(!pool.hasComponent(1), "source component remained after move");
    require(pool.hasComponent(3), "destination component was not created");
    require(pool.getComponent(3).value == 10, "destination component had the wrong value");
    require(pool.getComponent(2).value == 20, "unrelated component changed during move");
    require(std::find(pool.getEntities().begin(), pool.getEntities().end(), 3) != pool.getEntities().end(),
        "moved entity was missing from the dense entity list");
}

void testEcsEntityLifecycle()
{
    ECS ecs;
    const EntityID player = ecs.addEntity();
    const EntityID removed = ecs.addEntity();
    const EntityID survivor = ecs.addEntity();

    require(player == 0 && removed == 1 && survivor == 2, "entity IDs were not sequential");
    ecs.removeEntity(removed);
    require(!ecs.isAlive(removed), "removed entity was still alive");
    require(ecs.isAlive(player) && ecs.isAlive(survivor), "unrelated entity was removed");

    const EntityID recycled = ecs.addEntity();
    require(recycled == removed, "removed entity ID was not recycled");
}

void testEcsCopyAndView()
{
    ECS ecs;
    const EntityID first = ecs.addEntity();
    const EntityID second = ecs.addEntity();
    const EntityID copied = ecs.addEntity();

    ecs.addComponent<TestComponent>(first, TestComponent{10});
    ecs.addComponent<SecondaryComponent>(first, SecondaryComponent{20});
    ecs.addComponent<TestComponent>(second, TestComponent{30});
    ecs.copyComponent<TestComponent>(copied, second);

    require(ecs.getComponent<TestComponent>(copied).value == 30, "copied component had the wrong value");

    size_t matches = 0;
    for (auto [entity, test, secondary] : ecs.View<TestComponent, SecondaryComponent>()) {
        require(entity == first, "view returned the wrong entity");
        require(test.value == 10, "view returned the wrong primary component");
        require(secondary.value == 20, "view returned the wrong secondary component");
        matches++;
    }
    require(matches == 1, "view returned the wrong number of entities");
}

void testEcsViewMutatesComponents()
{
    ECS ecs;
    const EntityID entity = ecs.addEntity();
    ecs.addComponent<TestComponent>(entity, TestComponent{10});

    for (auto [id, test] : ecs.View<TestComponent>()) {
        require(id == entity, "view returned the wrong entity for mutation");
        test.value = 42;
    }

    require(ecs.getComponent<TestComponent>(entity).value == 42, "view component reference did not mutate ECS storage");
}

void testEcsViewMissingPoolIsEmpty()
{
    ECS ecs;
    const EntityID entity = ecs.addEntity();
    ecs.addComponent<TestComponent>(entity, TestComponent{10});

    size_t matches = 0;
    for (auto [id, secondary] : ecs.View<SecondaryComponent>()) {
        matches++;
    }
    require(matches == 0, "single-component view with a missing pool was not empty");

    for (auto [id, test, secondary] : ecs.View<TestComponent, SecondaryComponent>()) {
        matches++;
    }
    require(matches == 0, "multi-component view with a missing pool was not empty");
}

void testEcsViewEntitiesLegacy()
{
    ECS ecs;
    const EntityID first = ecs.addEntity();
    const EntityID second = ecs.addEntity();

    ecs.addComponent<TestComponent>(first, TestComponent{10});
    ecs.addComponent<SecondaryComponent>(first, SecondaryComponent{20});
    ecs.addComponent<TestComponent>(second, TestComponent{30});

    const std::vector<EntityID>& view = ecs.ViewEntities<TestComponent, SecondaryComponent>();
    require(view.size() == 1, "legacy view returned the wrong number of entities");
    require(view.front() == first, "legacy view returned the wrong entity");
}

void testEcsConstView()
{
    ECS ecs;
    const EntityID entity = ecs.addEntity();
    ecs.addComponent<TestComponent>(entity, TestComponent{10});
    ecs.addComponent<SecondaryComponent>(entity, SecondaryComponent{20});

    auto view = ecs.constView<TestComponent, SecondaryComponent>();
    auto row = *view.begin();
    static_assert(std::is_same_v<decltype(std::get<1>(row)), const TestComponent&>);
    static_assert(std::is_same_v<decltype(std::get<2>(row)), const SecondaryComponent&>);

    size_t matches = 0;
    for (auto [id, test, secondary] : view) {
        require(id == entity, "const view returned the wrong entity");
        require(test.value == 10, "const view returned the wrong primary component");
        require(secondary.value == 20, "const view returned the wrong secondary component");
        matches++;
    }
    require(matches == 1, "const view returned the wrong number of entities");
}

void testEcsViewExcludesComponents()
{
    ECS ecs;
    const EntityID included = ecs.addEntity();
    const EntityID excluded = ecs.addEntity();
    const EntityID missingIncludedComponent = ecs.addEntity();

    ecs.addComponent<TestComponent>(included, TestComponent{10});
    ecs.addComponent<TestComponent>(excluded, TestComponent{20});
    ecs.addComponent<SecondaryComponent>(excluded, SecondaryComponent{30});
    ecs.addComponent<SecondaryComponent>(missingIncludedComponent, SecondaryComponent{40});

    size_t matches = 0;
    for (auto [id, test] : ecs.View<TestComponent>(ecs::Exclude<SecondaryComponent>{})) {
        require(id == included, "excluded view returned an entity with the excluded component");
        require(test.value == 10, "excluded view returned the wrong included component");
        matches++;
    }

    require(matches == 1, "excluded view returned the wrong number of entities");
}

void testEcsViewExcludeMutatesComponents()
{
    ECS ecs;
    const EntityID mutableEntity = ecs.addEntity();
    const EntityID filteredEntity = ecs.addEntity();

    ecs.addComponent<TestComponent>(mutableEntity, TestComponent{10});
    ecs.addComponent<TestComponent>(filteredEntity, TestComponent{20});
    ecs.addComponent<SecondaryComponent>(filteredEntity, SecondaryComponent{30});

    for (auto [id, test] : ecs.View<TestComponent>(ecs::Exclude<SecondaryComponent>{})) {
        require(id == mutableEntity, "excluded mutable view returned the wrong entity");
        test.value = 42;
    }

    require(ecs.getComponent<TestComponent>(mutableEntity).value == 42,
        "excluded view component reference did not mutate ECS storage");
    require(ecs.getComponent<TestComponent>(filteredEntity).value == 20,
        "excluded view mutated a filtered entity");
}

void testEcsViewMissingExcludedPoolIsIgnored()
{
    ECS ecs;
    const EntityID entity = ecs.addEntity();
    ecs.addComponent<TestComponent>(entity, TestComponent{10});

    const std::string beforeView = ecs.numberOfEntities();

    size_t matches = 0;
    for (auto [id, test] : ecs.View<TestComponent>(ecs::Exclude<MissingExcludedComponent>{})) {
        require(id == entity, "view with missing excluded pool returned the wrong entity");
        require(test.value == 10, "view with missing excluded pool returned the wrong component");
        matches++;
    }

    require(matches == 1, "missing excluded pool should not make the view empty");
    require(ecs.numberOfEntities() == beforeView, "missing excluded pool was created by View");
}

void testEcsConstViewExcludesComponents()
{
    ECS ecs;
    const EntityID included = ecs.addEntity();
    const EntityID excluded = ecs.addEntity();

    ecs.addComponent<TestComponent>(included, TestComponent{10});
    ecs.addComponent<TestComponent>(excluded, TestComponent{20});
    ecs.addComponent<SecondaryComponent>(excluded, SecondaryComponent{30});

    const ECS& constEcs = ecs;
    auto view = constEcs.constView<TestComponent>(ecs::Exclude<SecondaryComponent>{});
    auto row = *view.begin();
    static_assert(std::is_same_v<decltype(std::get<1>(row)), const TestComponent&>);

    size_t matches = 0;
    for (auto [id, test] : view) {
        require(id == included, "excluded const view returned an entity with the excluded component");
        require(test.value == 10, "excluded const view returned the wrong component");
        matches++;
    }

    require(matches == 1, "excluded const view returned the wrong number of entities");
}

void testColliderJsonParsesMultipleShapes()
{
    json colliderJson = {
        {"shapes", {
            {
                {"offset", {{"x", 2}, {"y", -1}}},
                {"size", {{"x", 8}, {"y", 10}}},
                {"layer", "PLAYER_LAYER"},
                {"targetMask", {"ENEMY_LAYER", "OBSTACLE_LAYER"}},
                {"debugColor", {{"r", 1}, {"g", 2}, {"b", 3}, {"a", 4}}},
                {"isTrigger", false}
            },
            {
                {"offset", {{"x", 0}, {"y", 0}}},
                {"size", {{"x", 4}, {"y", 6}}},
                {"layer", "LOOT_LAYER"},
                {"targetMask", {"PLAYER_LAYER"}},
                {"debugColor", {{"r", 0}, {"g", 0}, {"b", 255}, {"a", 255}}},
                {"isTrigger", true}
            }
        }}
    };

    CCollider collider(colliderJson);

    require(collider.shapes.size() == 2, "collider json did not create both shapes");
    require(collider.shapes[0].offset == Vec2{2, -1}, "collider shape offset parsed incorrectly");
    require(collider.shapes[0].halfSize == Vec2{4, 5}, "collider shape half size was not calculated");
    require((collider.shapes[0].targetMask & ENEMY_LAYER) == ENEMY_LAYER, "collider target mask missed enemy layer");
    require((collider.shapes[0].targetMask & OBSTACLE_LAYER) == OBSTACLE_LAYER, "collider target mask missed obstacle layer");
    require(collider.shapes[0].debugColor.r == 1, "collider debug color parsed incorrectly");
    require(collider.shapes[1].isTrigger, "trigger shape did not parse isTrigger");
}

void testColliderConstructorsCalculateHalfSize()
{
    CCollider collider(Vec2{10, 20}, PLAYER_LAYER, ENEMY_LAYER);

    require(collider.shapes.size() == 1, "single-shape collider constructor created the wrong shape count");
    require(collider.shapes[0].size == Vec2{10, 20}, "single-shape collider constructor stored the wrong size");
    require(collider.shapes[0].halfSize == Vec2{5, 10}, "single-shape collider constructor did not calculate half size");
    require(collider.shapes[0].layer == PLAYER_LAYER, "single-shape collider constructor stored the wrong layer");
    require(collider.shapes[0].targetMask == ENEMY_LAYER, "single-shape collider constructor stored the wrong target mask");
    require(!collider.shapes[0].isTrigger, "single-shape collider constructor should default to solid");
}

void testStaticMarkerParticipatesInColliderTransformView()
{
    ECS ecs;
    const EntityID staticEntity = ecs.addEntity();
    const EntityID dynamicEntity = ecs.addEntity();

    ecs.addComponent<CTransform>(staticEntity, Vec2{8, 8});
    ecs.addComponent<CCollider>(staticEntity, Vec2{16, 16}, OBSTACLE_LAYER, PLAYER_LAYER);
    ecs.addComponent<CStatic>(staticEntity);

    ecs.addComponent<CTransform>(dynamicEntity, Vec2{24, 8});
    ecs.addComponent<CCollider>(dynamicEntity, Vec2{16, 16}, PLAYER_LAYER, OBSTACLE_LAYER);

    size_t matches = 0;
    for (auto [entity, collider, transform, staticMarker] : ecs.constView<CCollider, CTransform, CStatic>()) {
        require(entity == staticEntity, "static view returned a non-static entity");
        require(transform.pos == Vec2{8, 8}, "static view returned the wrong transform");
        require(collider.shapes.size() == 1, "static view returned the wrong collider");
        matches++;
    }

    require(matches == 1, "static collider transform view returned the wrong number of entities");
}

void testRenderLayerJsonParsing()
{
    require(
        renderLayerFromJson("TERRAIN_TILE_LOW_RENDER_LAYER") == RenderLayer::TerrainTilesLow,
        "named low terrain render layer parsed incorrectly"
    );
    require(
        renderLayerFromJson("TERRAIN_TILE_HIGH_RENDER_LAYER") == RenderLayer::TerrainTilesHigh,
        "named high terrain render layer parsed incorrectly"
    );
    require(
        renderLayerFromJson(7) == 7,
        "numeric render layer parsed incorrectly"
    );

    bool threw = false;
    try {
        renderLayerFromJson("UNKNOWN_RENDER_LAYER");
    }
    catch (const std::out_of_range&) {
        threw = true;
    }
    require(threw, "unknown render layer name should throw");
}

void testQuadtreeStoresColliderProxyIndices()
{
    Quadtree tree({0, 0}, {256, 256});
    tree.insert(0, {0, 0}, {16, 16});
    tree.insert(1, {24, 0}, {16, 16});

    const auto objects = tree.getObjects();
    require(objects.size() == 2, "quadtree did not store both proxy indices");
    require(std::find(objects.begin(), objects.end(), 0) != objects.end(), "quadtree missed proxy index 0");
    require(std::find(objects.begin(), objects.end(), 1) != objects.end(), "quadtree missed proxy index 1");
}

void testQuadtreeCanReturnDuplicateProxyLeafAppearances()
{
    Quadtree tree({0, 0}, {256, 256});
    tree.insert(0, {0, 0}, {160, 160});
    tree.insert(1, {-72, -72}, {16, 16});
    tree.insert(2, {-48, -72}, {16, 16});
    tree.insert(3, {48, -72}, {16, 16});
    tree.insert(4, {72, -72}, {16, 16});
    tree.insert(5, {-72, 72}, {16, 16});
    tree.insert(6, {-48, 72}, {16, 16});
    tree.insert(7, {48, 72}, {16, 16});
    tree.insert(8, {72, 72}, {16, 16});

    auto leaves = tree.createQuadtreeVector();
    size_t occurrences = 0;
    for (const auto& leaf : leaves) {
        const auto objects = leaf->getObjects();
        occurrences += static_cast<size_t>(std::count(objects.begin(), objects.end(), 0));
    }

    require(occurrences > 1, "quadtree test setup did not produce duplicate proxy leaf appearances");
}

void testQuadtreeClonePreservesBaseAndAllowsIndependentInserts()
{
    Quadtree base({0, 0}, {256, 256});
    base.insert(0, {0, 0}, {160, 160});
    base.insert(1, {-72, -72}, {16, 16});
    base.insert(2, {-48, -72}, {16, 16});
    base.insert(3, {48, -72}, {16, 16});
    base.insert(4, {72, -72}, {16, 16});
    base.insert(5, {-72, 72}, {16, 16});
    base.insert(6, {-48, 72}, {16, 16});
    base.insert(7, {48, 72}, {16, 16});
    base.insert(8, {72, 72}, {16, 16});

    auto cloned = base.clone();

    require(cloned->Divided(), "cloned quadtree did not preserve child subdivision");
    require(
        countQuadtreeObjectOccurrences(*cloned, 0) == countQuadtreeObjectOccurrences(base, 0),
        "cloned quadtree did not preserve existing proxy appearances"
    );

    cloned->insert(99, {96, 96}, {16, 16});

    require(
        countQuadtreeObjectOccurrences(*cloned, 99) > 0,
        "cloned quadtree did not accept a dynamic insert"
    );
    require(
        countQuadtreeObjectOccurrences(base, 99) == 0,
        "dynamic insert into cloned quadtree mutated the static base"
    );
}

void testWorldCenteredQuadtreeCoversLevelEdges()
{
    const Vec2 worldSize{16000.0f, 8000.0f};
    Quadtree tree(worldSize / 2.0f, worldSize);

    tree.insert(0, {8.0f, 8.0f}, {16.0f, 16.0f});
    tree.insert(1, {worldSize.x - 8.0f, worldSize.y - 8.0f}, {16.0f, 16.0f});

    auto objects = tree.getObjects();
    require(std::find(objects.begin(), objects.end(), 0) != objects.end(),
        "world-centered quadtree missed collider at the origin edge");
    require(std::find(objects.begin(), objects.end(), 1) != objects.end(),
        "world-centered quadtree missed collider at the far world edge");

    tree.clear();
    tree.insert(2, {worldSize.x - 8.0f, 8.0f}, {16.0f, 16.0f});

    objects = tree.getObjects();
    require(std::find(objects.begin(), objects.end(), 0) == objects.end(),
        "quadtree clear left an old collider proxy behind");
    require(std::find(objects.begin(), objects.end(), 2) != objects.end(),
        "cleared world-centered quadtree did not accept a new edge collider");
}

void testEcsQueuedRemoval()
{
    ECS ecs;
    const EntityID player = ecs.addEntity();
    const EntityID target = ecs.addEntity();
    ecs.addComponent<TestComponent>(target, TestComponent{10});

    ecs.queueRemoveComponent<TestComponent>(target);
    ecs.update();
    require(!ecs.hasComponent<TestComponent>(target), "queued component removal was not applied");

    ecs.addComponent<TestComponent>(target, TestComponent{20});
    ecs.queueRemoveEntity(target);
    ecs.update();

    require(ecs.isAlive(player), "queued removal affected the player entity");
    require(!ecs.isAlive(target), "queued entity removal was not applied");
    require(!ecs.hasComponent<TestComponent>(target), "entity removal left its component behind");
}

void testEcsRemovalObserverSeesCascade()
{
    ECS ecs;
    const EntityID player = ecs.addEntity();
    const EntityID parent = ecs.addEntity();
    const EntityID child = ecs.addEntity();
    std::vector<EntityID> removed;

    ecs.addComponent<TestComponent>(parent, TestComponent{10});
    ecs.addComponent<TestComponent>(child, TestComponent{20});
    ecs.attachChild(parent, child, Vec2{4, 8});

    ecs.setEntityRemovalObserver([&](EntityID entity) {
        removed.push_back(entity);
        if (entity == parent || entity == child) {
            require(ecs.hasComponent<TestComponent>(entity),
                "removal observer ran after components were stripped");
        }
    });

    ecs.queueRemoveEntity(parent);
    ecs.update();

    require(ecs.isAlive(player), "observer cascade removal affected entity 0");
    require(!ecs.isAlive(parent), "parent was not removed");
    require(!ecs.isAlive(child), "child was not removed by parent cascade");
    require(std::find(removed.begin(), removed.end(), parent) != removed.end(),
        "removal observer did not see parent removal");
    require(std::find(removed.begin(), removed.end(), child) != removed.end(),
        "removal observer did not see child cascade removal");
}

void testEcsEntityZeroQueuedComponentRemoval()
{
    ECS ecs;
    const EntityID player = ecs.addEntity();
    ecs.addComponent<TestComponent>(player, TestComponent{10});

    ecs.queueRemoveComponent<TestComponent>(player);
    ecs.update();

    require(ecs.isAlive(player), "queued component removal affected entity 0 lifetime");
    require(!ecs.hasComponent<TestComponent>(player), "queued component removal ignored entity 0");

    ecs.queueRemoveEntity(player);
    ecs.update();

    require(ecs.isAlive(player), "queued entity removal should still protect entity 0");
}

constexpr std::array Tests = {
    TestSupport::TestCase{"pool_add_and_get", testPoolAddAndGet},
    TestSupport::TestCase{"pool_replace_existing", testPoolReplaceExisting},
    TestSupport::TestCase{"pool_entity_zero_component_removal", testPoolEntityZeroComponentRemoval},
    TestSupport::TestCase{"pool_remove_compacts", testPoolRemoveCompacts},
    TestSupport::TestCase{"pool_move", testPoolMove},
    TestSupport::TestCase{"entity_lifecycle", testEcsEntityLifecycle},
    TestSupport::TestCase{"copy_and_view", testEcsCopyAndView},
    TestSupport::TestCase{"view_mutates_components", testEcsViewMutatesComponents},
    TestSupport::TestCase{"view_missing_pool_is_empty", testEcsViewMissingPoolIsEmpty},
    TestSupport::TestCase{"view_entities_legacy", testEcsViewEntitiesLegacy},
    TestSupport::TestCase{"const_view", testEcsConstView},
    TestSupport::TestCase{"view_excludes_components", testEcsViewExcludesComponents},
    TestSupport::TestCase{"view_exclude_mutates_components", testEcsViewExcludeMutatesComponents},
    TestSupport::TestCase{"view_missing_excluded_pool_is_ignored", testEcsViewMissingExcludedPoolIsIgnored},
    TestSupport::TestCase{"const_view_excludes_components", testEcsConstViewExcludesComponents},
    TestSupport::TestCase{"collider_json_parses_multiple_shapes", testColliderJsonParsesMultipleShapes},
    TestSupport::TestCase{"collider_constructors_calculate_half_size", testColliderConstructorsCalculateHalfSize},
    TestSupport::TestCase{"static_marker_participates_in_collider_transform_view", testStaticMarkerParticipatesInColliderTransformView},
    TestSupport::TestCase{"render_layer_json_parsing", testRenderLayerJsonParsing},
    TestSupport::TestCase{"quadtree_stores_collider_proxy_indices", testQuadtreeStoresColliderProxyIndices},
    TestSupport::TestCase{"quadtree_can_return_duplicate_proxy_leaf_appearances", testQuadtreeCanReturnDuplicateProxyLeafAppearances},
    TestSupport::TestCase{"quadtree_clone_preserves_base_and_allows_independent_inserts", testQuadtreeClonePreservesBaseAndAllowsIndependentInserts},
    TestSupport::TestCase{"world_centered_quadtree_covers_level_edges", testWorldCenteredQuadtreeCoversLevelEdges},
    TestSupport::TestCase{"queued_removal", testEcsQueuedRemoval},
    TestSupport::TestCase{"removal_observer_sees_cascade", testEcsRemovalObserverSeesCascade},
    TestSupport::TestCase{"entity_zero_queued_component_removal", testEcsEntityZeroQueuedComponentRemoval}
};

} // namespace

int main(int argc, char* argv[])
{
    return TestSupport::runNamedTest(argc, argv, Tests);
}
