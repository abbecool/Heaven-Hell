#include "TestSupport.hpp"
#include "ecs/ECS.hpp"

#include <array>
#include <algorithm>
#include <type_traits>

namespace {

using TestSupport::require;

struct TestComponent {
    int value = 0;
};

struct SecondaryComponent {
    int value = 0;
};

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
    TestSupport::TestCase{"queued_removal", testEcsQueuedRemoval},
    TestSupport::TestCase{"entity_zero_queued_component_removal", testEcsEntityZeroQueuedComponentRemoval}
};

} // namespace

int main(int argc, char* argv[])
{
    return TestSupport::runNamedTest(argc, argv, Tests);
}
