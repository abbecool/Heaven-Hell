# Agent Notes

This file is for AI coding sessions working in this repository. Human setup
details live in `README.md`; use this as the fast path for building and testing.

## How To Use This File

Read this file before making changes. A root `AGENTS.md` applies to the whole
repo and is the right place for short, durable repo instructions. Use a
`.agents/` folder only for longer optional notes that are linked from here;
do not assume every coding agent will discover `.agents/` automatically.

## Windows Build Path

This project uses the MSYS2 UCRT64 toolchain on Windows. Before running CMake,
put the UCRT64 `bin` directory at the front of `PATH` in the current
PowerShell session:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
```

Use the explicit MSYS2 CMake executable so the command works even when regular
`cmake` is not on `PATH`:

```powershell
& C:\msys64\ucrt64\bin\cmake.exe --preset windows-ninja-debug
& C:\msys64\ucrt64\bin\cmake.exe --build --preset windows-ninja-debug
& C:\msys64\ucrt64\bin\ctest.exe --preset windows-ninja-debug --output-on-failure
```

The debug executable is written to:

```text
run/Debug/heavenhell.exe
```

## Existing Build Tree

If a preconfigured `build` directory is already being used in the session, the
same `PATH` step is still required before compiling:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
& C:\msys64\ucrt64\bin\cmake.exe --build build --target sync_assets
& C:\msys64\ucrt64\bin\ctest.exe --test-dir build --output-on-failure
```

## Common Failure Mode

If `C:\msys64\ucrt64\bin` is missing from `PATH`, `g++.exe` may fail with exit
code `1` and no useful compiler diagnostic, even for tiny syntax checks. This
usually means the compiler frontend or MSYS2 runtime helpers are not being
found, not that the edited C++ file is necessarily broken.

Do not spend time debugging source changes until the build command has been
rerun with the UCRT64 `bin` directory prepended to `PATH`.

## Validation Checklist

For C++ changes:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
& C:\msys64\ucrt64\bin\cmake.exe --build build --target sync_assets
& C:\msys64\ucrt64\bin\ctest.exe --test-dir build --output-on-failure
```

For JSON/data changes, also parse the touched JSON files before building. In
PowerShell:

```powershell
Get-Content config_files\assets.json -Raw | ConvertFrom-Json > $null
```

Adjust the path for each edited JSON file.

## Do Not Edit Unless Asked

- Do not edit `build/`, `run/`, `dist/`, or generated/package outputs unless
  the user explicitly asks for build artifact or release work.
- Do not revert or clean unrelated dirty files. The user may have local work in
  progress.
- Keep changes scoped. Prefer data/config edits for content additions when the
  existing system already supports them.

## Data-Driven Content

Most gameplay content is JSON driven:

- Mob definitions: `config_files/mobs/*.json`
- Spawn lists: `config_files/mobs.json`
- Entity templates: `config_files/entities/*.json`
- Item definitions: `config_files/items/*.json`
- Asset and animation registry: `config_files/assets.json`

When adding a new PNG sprite or sheet, add the file under `assets/images/` and
register it in `config_files/assets.json`. The build copies assets and
`config_files` into the runtime directory through `sync_assets`.

## AI And Movement Gotchas

- `sAI()` processes non-player entities with `CAIAgent`, `CInput`, and
  `CTransform`. The current `m_player` must stay under player input even when
  it still has `CAIAgent` after possession.
- Possession removes `CAIAgent` from the possessed entity; keep the `sAI()`
  player guard as a safety net for queued removal and future content.
- Possession can happen through the `PLAYER_LAYER`/`FRIENDLY_LAYER` trigger
  handler or the `PLAYER_LAYER`/`ENEMY_LAYER` trigger handler, as long as the
  target has `CPossessable`.
- When possession should keep a mob's attack, copy the possessed mob's active
  inventory item before overwriting its `CInventory`, then add that copied item
  back to the new player inventory after `changePlayerID()`. Enemy attacks often
  target `PLAYER_LAYER`, so retarget copied player-usable mob weapons to
  `ENEMY_LAYER`.
- AI movement only affects physics-driven entities when they also have
  `CPhysicsBody`, which creates `CVelocity` during spawn.
- `sightRange <= 0` means the AI should never see or follow the player.
- Patrol behavior uses `CAIAgent::spawnPos`, `patrolRadius`, and
  `patrolWaitDuration`.
- Weapon behavior comes from inventory/active item weapon config. Do not give a
  mob `CInventory` with a weapon item unless it should be able to attack.

## Scene Play Order

`Scene_Play::update()` runs gameplay systems in this order:

```text
sLoader -> sAI -> sAttack -> sMovement -> sStatus -> sCollision -> sAnimation -> sAudio
```

This matters because AI writes movement/use intent before attack and movement,
and NPC input is reset during movement after it has been consumed.

## Notes

- Windows presets in `CMakePresets.json` already point at
  `C:/msys64/ucrt64/bin/g++.exe` and `ninja.exe`.
- Build commands sync assets and `config_files` into the run directory through
  the `sync_assets` target.
- Keep generated build outputs, `run/`, and `dist/` out of source edits unless
  the user explicitly asks for packaging or release work.
