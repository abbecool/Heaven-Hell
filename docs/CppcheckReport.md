# Cppcheck findings and formatting review

## Run details

- Date: 2026-09-22.
- Source snapshot: `db04605` (`clang-format entire project`) plus the named-layout
  change in `Scene_Editor::openLayout`. Locations below refer to this working tree.
- Tool: Cppcheck 2.22.0; C++20; Windows x64.
- Completed: **38/38 translation units**, including the test targets.
- Exit code: **1**, as configured when findings are reported; the refreshed run
  completed normally. Remaining findings are warnings, performance, and style.
- Scope: the debug compilation database, with warning, style, performance,
  and portability checks enabled. Default error checks are also active.
- Exclusions: `src/external` source files; diagnostics from its included
  headers. External headers are still parsed to understand first-party code.
- Full diagnostic log: [CppcheckLog.md](CppcheckLog.md). This preserves every
  message, location, column, and diagnostic ID in original emission order.

This is a run of the configured project checks, not `--enable=all` or an
exhaustive proof of correctness. `unusedFunction`, missing-include reporting,
and inconclusive checks were not separately enabled. Findings are analyzer
reports, not necessarily confirmed bugs.

### Reproduce

Run from the repository root after configuring the debug preset:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
& C:/msys64/ucrt64/bin/cmake.exe --preset windows-ninja-debug
& 'C:/Program Files/Cppcheck/cppcheck.exe' `
  --project=build/Windows/Ninja/Debug/compile_commands.json `
  --platform=win64 --std=c++20 `
  --enable=warning,style,performance,portability `
  --inline-suppr --error-exitcode=1 `
  '--template={file}:{line}:{column}: {severity}: {message} [{id}]' `
  -isrc/external `
  '--suppress=*:src/external/*' `
  '--suppress=*:*/src/external/*' `
  --output-file=docs/cppcheck.log
```

The saved Markdown log wraps the resulting diagnostic output in a code block.
Unlike the VS Code task's display template, the log preserves Cppcheck's
original severity directly, rather than prefixing every entry with `warning`.

## Severity summary

The order below is a practical review order. Performance and style are
categories rather than a universal measure of risk; review notes may change
the practical priority of an individual finding.

| Rank | Cppcheck severity | Diagnostic occurrences |
| --- | --- | ---: |
| 1 | error | 0 |
| 2 | warning | 30 |
| 3 | performance | 29 |
| 4 | portability | 0 |
| 5 | style | 132 |
| | **Total** | **191** |

Counts are emitted occurrences, not distinct root causes. Template
instantiations can generate multiple messages at the same source location.
The current ranked inventory below includes all **23 diagnostic IDs**. Repeated
line locations are consolidated there; the full log retains every occurrence.

## Recommended attention order after source review

1. **Camera initialization/public API:** `Camera::panCamera()` computes a
   velocity using `panStepCount` and `panSpeed` at `Camera.cpp:142–143` before
   checking `panDuration`. The constructor leaves these scalars uninitialized.
   Calling this public method before `startPan()` is unsafe. The normal
   `update()` caller is guarded, so an actual current gameplay failure has not
   been demonstrated. This deserves attention before cosmetic improvements.
2. **Other uninitialized members:** audit default-constructed `Entity`,
   `ScriptableEntity::m_physics`, and `PlayerConfig` members. Confirm initialization
   at call sites before treating every declaration as a runtime defect.
3. **Duplicated inventory state:** the derived mouse position and rendering
   flags hide separate base-class members. This can cause state divergence if
   future code uses the wrong member; the current derived duplicates appear
   unused.
4. **Potential logic/intent issues:** the unreachable ECS log, boolean bitwise
   operators, and the reported unused ECS assignments deserve source review.
   Do not remove component mutations just because Cppcheck labels them unread.
5. **Performance and routine style:** address selectively after correctness.
   Passing a value can be intentional, and changing copies into references can
   change aliasing or lifetime behavior.

## Ranked diagnostic inventory

### 1. Errors — 0 occurrences

No errors remain in the refreshed full-project run.

#### Resolved: four `danglingTempReference` reports

The previous run reported four errors at `src/scenes/Scene_Editor.cpp:373`
(columns 35 and 48), `:374:30`, and `:376:50`. All came from the range expression
`m_layouts.loadLayout(info).placements`. The code now explicitly owns the layout:

```cpp
const WorldLayout loadedLayout = m_layouts.loadLayout(info);
for (const LayoutPlacement& placement : loadedLayout.placements)
```

`loadLayout()` returns `WorldLayout` by value, and `placements` is a direct,
non-reference vector member. The original range-for's implicit `auto&&` binding
already extended the complete temporary's lifetime through the loop in C++20.
These were high-confidence false positives; the named local makes ownership
clear to readers and Cppcheck without changing the loop body or adding a copy
of the placements vector.

This differs from iterating through a reference-returning accessor on a
temporary. Relevant C++20 rules: [range-for expansion](https://timsong-cpp.github.io/cppwp/n4861/stmt.ranged#1)
and [temporary lifetime extension, including direct member access](https://timsong-cpp.github.io/cppwp/n4861/class.temporary#6).
Targeted analysis and the refreshed **38/38 translation-unit** run confirm
that all four reports disappeared without suppressions. Comparing full logs
shows exactly those four diagnostic lines removed, with no added or changed
remaining messages. The source passes clang-format. The build passed; tests
passed **7/8 suites**, with the previously observed unescaped Windows-path JSON
fixture failure in `world_layout_tests` still present.

### 2. Warnings — 30 occurrences

| ID | Count | Locations | Assessment |
| --- | ---: | --- | --- |
| `uninitMemberVar` | 9 | `src/ecs/Entity.hpp:16` (2); `src/physics/Camera.cpp:8` (7) | Potential reads of indeterminate scalar members. Camera's public pan method has the conditional unsafe path described above. |
| `uninitMemberVarNoCtor` | 18 | `src/ecs/ScriptableEntity.hpp:50` (1); `src/scenes/Scene_Menu.hpp:12` (8); `src/scenes/Scene_GameOver.hpp:11` (8); `tests/TestSupport.hpp:24` (1) | Missing declaration-level initialization. Actual severity depends on construction and assignment at use sites. Aggregate test cases explicitly supplying their function pointer are not demonstrated failures. |
| `duplInheritedMember` | 3 | `src/scenes/Scene_Inventory.hpp:13,17,18` | `m_mousePosition`, `m_drawTextures`, and `m_drawCollision` hide base-class storage. Maintainability/state-divergence risk. |

For `Entity`, `Entity e;` leaves the scalar members indeterminate, while
`Entity e{};` zero-initializes them because its default constructor is defaulted
on its first declaration. A zero pointer still is not a usable ECS handle.
Observed active construction sites supply an ID and manager; no existing
invalid read was established during this review.

### 3. Performance — 29 occurrences

| ID | Count | Locations | Meaning / caveat |
| --- | ---: | --- | --- |
| `passedByValue` | 14 | `src/assets/Assets.cpp:93`; `src/assets/SpriteDefinition.cpp:8,14`; `src/ecs/Components.hpp:558,589,598,864`; `src/physics/Vec2.cpp:174`; `src/scenes/Scene.cpp:37` (2), `:103`; `src/scenes/Scene_Inventory.cpp:87`; `src/scenes/Scene_Play.cpp:105,1659` | Possible avoidable parameter copies. Check ownership and move semantics before replacing with const references. |
| `stlFindInsert` | 8 | `src/core/Game.cpp:119`; `src/ecs/ECS.hpp:620` (7 template-related reports) | Lookup followed by insertion may do redundant map work. A blind `try_emplace(key, make_unique(...))` rewrite still eagerly evaluates `make_unique` on an existing key. |
| `iterateByValue` | 3 | `src/assets/Assets.cpp:48,128`; `src/physics/Quadtree.hpp:70` | Range loop copies each element; a const reference may be cheaper. |
| `useInitializationList` | 3 | `src/ecs/Components.hpp:302,835`; `src/physics/Level_Loader.cpp:120` | Initialize members directly rather than assigning in the constructor body, where appropriate. |
| `returnByReference` | 1 | `src/assets/SpriteDefinition.hpp:31` | Possible copied member return; review the size of `TextureHandle` and caller lifetime requirements before changing the API. |

### 4. Portability — 0 occurrences

No portability diagnostics were emitted with the selected Windows/C++20
configuration. This does not establish that every platform is supported.

### 5. Style — 132 occurrences

Listed with behavior/intent-related items first, then routine API/style items.

| ID | Count | Locations | Meaning / review note |
| --- | ---: | --- | --- |
| `unreachableCode` | 1 | `src/ecs/ECS.hpp:611` | Logging follows `return emptyPool;` and can never execute. Confirmed dead statement. |
| `bitwiseOnBoolean` | 4 | `src/physics/Vec2.cpp:181,186,191,196` | Boolean comparisons use bitwise operators. With these boolean operands the truth values match logical operators, but evaluation is not short-circuited. Verify intent. |
| `unreadVariable` | 3 | `src/physics/CollisionManager.cpp:774,787,814` | Writes through ECS component references are reported unread. They may intentionally affect later systems; inspect consumers before deleting. |
| `knownConditionTrueFalse` | 1 | `src/physics/Quadtree.hpp:64` | `!m_divided` is true here because the earlier divided branch returns. Redundant guard, not evidence of a broken quadtree. |
| `missingOverride` | 21 | `src/physics/Camera.cpp:200,202,204`; `src/scenes/Scene_Finish.hpp:17,18,23`; `Scene_GameOver.hpp:28,29,34`; `Scene_Inventory.hpp:32,33,37`; `Scene_Menu.hpp:28,29,34`; `Scene_Pause.hpp:26,30,31`; `Scene_Play.hpp:77,78,112` (all latter headers under `src/scenes/`) | Add `override` to make the compiler check the intended overrides. |
| `noExplicitConstructor` | 42 | `src/core/Game.hpp:62`; `src/ecs/Components.hpp:121,175,182,209,229,284,299,325,337,341,412,427,442,459,482,490,525,558,568,581,589,597,625,642,689,690,733,748,767,829,864,884`; `src/physics/InventoryManager.hpp:75,113`; `src/physics/Vec2.hpp:22`; `src/scenes/Scene_Finish.hpp:22`; `Scene_GameOver.hpp:33`; `Scene_Inventory.hpp:36`; `Scene_Menu.hpp:33`; `Scene_Pause.hpp:29` (scene headers under `src/scenes/`); `src/world/EntityCatalog.hpp:29` | Single-argument constructors permit implicit conversions. Add `explicit` only where those conversions are not intended. |
| `constVariableReference` | 17 | `src/physics/CollisionManager.cpp:652,744`; `src/physics/Quadtree.cpp:74`; `src/scenes/Scene.cpp:50`; `Scene_Finish.cpp:83`; `Scene_GameOver.cpp:96`; `Scene_Menu.cpp:133`; `Scene_Pause.cpp:102`; `Scene_Play.cpp:630,988,1000,1185,1220,1316,1417,2271` (scene files under `src/scenes/`); `tests/ecs/test_ecs.cpp:106` | Local reference could be const. |
| `useStlAlgorithm` | 16 | `src/debug/EntityInspector.cpp:283`; `src/ecs/Components.hpp:345,449`; `src/ecs/ECS.hpp:138,270,444`; `src/physics/CollisionManager.cpp:619`; `src/physics/Physics.cpp:20`; `src/physics/Quadtree.cpp:133`; `src/physics/RandomArray.cpp:24`; `src/physics/Renderer.hpp:99`; `src/scenes/Scene_Editor.cpp:376`; `src/story/StoryManager.cpp:478,522`; `tests/TestSupport.hpp:56,67` | Optional replacement of loops with standard algorithms. Readability preference, not a correctness requirement. |
| `functionStatic` | 13 | `src/core/SDLPlatform.cpp:187,199,347`; `src/physics/CollisionManager.cpp:194,224`; `src/physics/InventoryManager.hpp:59`; `src/physics/Level_Loader.cpp:187`; `src/physics/Physics.cpp:8,34,141`; `src/scenes/Scene_Play.cpp:1996,2074`; `src/story/StoryManager.cpp:307` | Member function does not need instance state; consider API design before making static. |
| `shadowFunction` | 5 | `src/physics/CollisionManager.cpp:198`; `src/physics/Physics.cpp:34`; `src/world/WorldLayout.cpp:71,159,285` | Local or parameter names shadow function names. |
| `constParameterReference` | 3 | `src/assets/Assets.cpp:56`; `src/scenes/Scene_Play.cpp:895,944` | Parameter could be a reference to const. |
| `uselessOverride` | 3 | `src/physics/Camera.cpp:200,202,204` | Empty overrides repeat base behavior; cleanup opportunity if deliberate extension points are not needed. |
| `constParameterPointer` | 1 | `src/ecs/ECS.hpp:110` | Pointer parameter could point to const. |
| `constVariablePointer` | 1 | `src/ecs/ECS.hpp:657` | Local pointer could point to const. |
| `variableScope` | 1 | `src/physics/Level_Loader.cpp:287` | Local variable could have narrower scope. |

## Formatting diff review

The formatting changes were committed while this review was underway, so the
review uses those commits rather than an empty working-tree diff.

### Whole-project formatting: `833f563..db04605`

**All 78 changed files exactly match the output of the installed clang-format
when applied to their pre-format contents using the repository's configuration**
(comparison normalizes CRLF/LF only). This checks the entire content of every
changed file, not just selected hunks. No extra edits beyond formatter output
were found in that commit. Vendor files were not changed.

The general result is consistent with the requested Allman braces, four-space
indentation, 80-column wrapping, pointer/reference alignment, and preserved
include order. Most of the large diff is brace placement and line wrapping.
Namespace-closing comments and adjacent string-literal splitting can also be
normal clang-format output; a whitespace-only diff is not sufficient to review
those cases.

Some output is visually awkward but not evidence of changed behavior:

- `src/physics/damageTypes.cpp:29–70`: deeply indented nested initializer maps.
- Long trailing comments can push otherwise short assignments onto two lines,
  as in the camera shake code.
- LLVM-derived bin-packing combines some argument and initializer lists.

These are optional style-tuning points, not reasons to revert the format pass.
Formatting replay establishes provenance, not a formal proof of semantic
equivalence. The preceding validation built successfully; 7/8 test suites
passed, with `world_layout_tests` failing on an unescaped Windows path in a
JSON fixture. The fixture constructs JSON using raw `.string()` paths; the
formatting replay shows that formatting did not introduce that construction.

### Earlier camera commit: `6e39ec7..833f563`

This commit is **not purely formatting**, despite its title:

- `Camera.cpp:52–58` replaces nested integer casts in camera snapping with
  `std::floor(playerPos / cellSize) * cellSize`. Negative coordinates behave
  differently: floor rounds toward negative infinity, whereas the old integer
  casts truncate toward zero. Confirm that this behavior is intended.
- The member `i` is renamed to `panStepCount` consistently in the shown camera
  implementation/header. This is a readability change rather than formatter
  output.

Neither change was produced by the whole-project formatting pass. They have
been reported for awareness rather than modified by this review.
