#include "TestSupport.hpp"
#include "ecs/ECS.hpp"

#include <array>
#include <algorithm>

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

    const std::vector<EntityID>& view = ecs.View<TestComponent, SecondaryComponent>();
    require(view.size() == 1, "view returned the wrong number of entities");
    require(view.front() == first, "view returned the wrong entity");
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

constexpr std::array Tests = {
    TestSupport::TestCase{"pool_add_and_get", testPoolAddAndGet},
    TestSupport::TestCase{"pool_replace_existing", testPoolReplaceExisting},
    TestSupport::TestCase{"pool_remove_compacts", testPoolRemoveCompacts},
    TestSupport::TestCase{"pool_move", testPoolMove},
    TestSupport::TestCase{"entity_lifecycle", testEcsEntityLifecycle},
    TestSupport::TestCase{"copy_and_view", testEcsCopyAndView},
    TestSupport::TestCase{"queued_removal", testEcsQueuedRemoval}
};

} // namespace

int main(int argc, char* argv[])
{
    return TestSupport::runNamedTest(argc, argv, Tests);
}
