# ECS View

`ECS::View<Components...>()` returns a lightweight range over entities that have every requested component.
Each loop item contains the entity ID first, then references to the requested components in the same order.

```cpp
for (auto [id, transform, velocity] : ecs.View<CTransform, CVelocity>()) {
    transform.pos += velocity.vel;
}
```

The component values are references to the real ECS storage. Changes made through the structured binding update the original components.

```cpp
for (auto [id, input] : ecs.View<CInput>()) {
    input.direction = {0, 0};
}
```

`ECS::constView<Components...>()` works the same way, but yields `const` component references. Use it when a system only reads components.

```cpp
for (auto [id, transform, sprite] : ecs.constView<CTransform, CSprite>()) {
    drawSprite(sprite, transform.pos);
    // transform.pos.x += 1; // compile error
}
```

Pass `ecs::Exclude<Components...>{}` to skip entities that have any of those components.
Excluded components are filters only. They are not returned in the structured binding.

```cpp
for (auto [id, transform] : ecs.View<CTransform>(ecs::Exclude<CStatic>{})) {
    transform.pos.x += 1;
}

for (auto [id, collider, transform] : ecs.constView<CCollider, CTransform>(ecs::Exclude<CStatic>{})) {
    drawCollider(collider, transform);
}
```

## Under The Hood

`ECS::View<Components...>()` creates an `ECSView<Components...>` object:

```cpp
template<typename... Components>
ECSView<Components...> View()
{
    return ECSView<Components...>(findComponentPool<Components>()...);
}
```

`findComponentPool<T>()` looks up each requested component pool without creating missing pools.
If any requested pool does not exist, `ECSView` has no driver pool and iterates as empty.
The exclusion overload also uses `findComponentPool<T>()`, so excluding a component type with no pool is a no-op and does not create an empty pool.

`ECS::constView<Components...>()` creates an `ECSConstView<Components...>` instead. Its internals mirror `ECSView`, but it stores `const ComponentPool<Components>*` pointers and dereferences to `const Components&...`.

Inside `ECSView`, the component pool pointers are stored in a tuple:

```cpp
using PoolTuple = std::tuple<ComponentPool<Components>*...>;
PoolTuple m_pools;
```

The constructor chooses a driver pool from the requested pools. The driver is the pool with the fewest entities, so iteration does the smallest possible scan.

```cpp
if ((pools && ...)) {
    (selectDriverPool(pools), ...);
}
```

The driver only provides entity IDs. The iterator then checks each candidate entity against all requested pools:

```cpp
bool matches(EntityID entity) const {
    return hasAllComponents(entity) && hasNoExcludedComponents(entity);
}
```

When the iterator lands on an entity with every requested component, dereferencing it builds a tuple containing the ID and component references:

```cpp
auto operator*() const {
    const EntityID entity = m_view->entityAt(m_index);
    return std::tuple<EntityID, Components&...>{
        entity,
        std::get<ComponentPool<Components>*>(m_view->m_pools)->getComponent(entity)...
    };
}
```

The const view dereference is the same shape with const references:

```cpp
return std::tuple<EntityID, const Components&...>{
    entity,
    std::get<const ComponentPool<Components>*>(m_view->m_pools)->getComponent(entity)...
};
```

That `Components&...` part is what makes this mutate the real ECS storage:

```cpp
for (auto [id, transform] : ecs.View<CTransform>()) {
    transform.pos.x += 1;
}
```

Even though the structured binding is written as `auto [id, transform]`, `transform` is bound from a tuple element whose type is `CTransform&`, so it still refers to the component in the pool.

`View` does not allocate a result vector. It walks the driver pool lazily and skips non-matching entities as the loop advances.

## Legacy Entity ID View

Use `ViewEntities<Components...>()` when you need the old ID-vector behavior, such as a stable snapshot you can store, sort, index, or iterate after changing the matching component pools.

```cpp
const auto& ids = ecs.ViewEntities<CTransform, CVelocity>();
```

## Removal During Iteration

When removing components that are part of the active `View`, prefer queued removal so the pool is not compacted while it is being iterated.

```cpp
for (auto [id, audio] : ecs.View<CAudio>()) {
    playAudio(audio.audioName);
    ecs.queueRemoveComponent<CAudio>(id);
}
```
