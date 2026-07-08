# Session TODO

Focused follow-ups from the ECS, collider, chunk, and batching work discussed in this session.
Ordered in the suggested implementation order.

## 1. Make OpenGL Batching Easier To Verify

- Add lightweight render stats for the OpenGL backend: sprite submissions, batch flushes, texture switches, and max batch size reached.
- Use the stats to confirm terrain sprites from `dual_sheets_atlas` are submitted consecutively where possible.
- Audit render layers so terrain atlas sprites are grouped before unrelated textures interrupt the batch.

## 2. Add Renderer-Level Sprite Batch Submission

- Add a backend API like `drawWorldSpriteBatch(texture, quads)` or equivalent.
- Keep the shared atlas path; do not bake one texture per chunk unless profiling proves it helps.
- SDL can loop internally at first, while OpenGL should append all quads to its existing instanced batch.

## 3. Rework Static Chunk Tile Rendering

- Add a lightweight `CStatic` marker to chunk-owned world entities.
- Replace non-animated per-tile sprite entities with chunk-owned static render data.
- Keep animated water tiles as normal entities with `CSprite`, `CAnimation`, and `CTransform`.
- Register chunk render data by layer/texture so static terrain is submitted before animated and dynamic sprites on the same layer.

## 4. Use `CStatic` For Static Collision Caching

- Split collision processing into cached static collider proxies and per-frame dynamic proxies.
- Rebuild static collision data only when chunks load, unload, or a static entity changes.
- Support future world edits by invalidating the static cache if an entity gains, loses, or mutates `CStatic`/`CCollider`.

## 5. Tighten Chunk Collider Merging

- Keep merged colliders owned by their chunk unless profiling proves cross-chunk merging is needed.
- Improve rectangle merging only if debug view shows too many collider rectangles.
- Avoid cross-chunk shared ownership until chunk lifetime rules are explicit.

## 6. Revisit Larger ECS Features Later

- Consider ECS groups only after profiling shows repeated multi-component views are a bottleneck.
- Consider multiple components of the same type per entity only when a concrete gameplay feature needs it.
- If repeated child visuals are the first use case, prefer a specific child/list component before changing core ECS storage.
