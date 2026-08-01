# Changelog

Entries are per-milestone (see `ROADMAP.md`), not per-commit — git history already covers
commit-level detail. Newest first.

## Unreleased

### Milestone 3 — Sector Generation, Zombies & AI

Rooms, enemies and the Spawn Director. First build in which a sector actually plays: the level is
assembled from handcrafted modules, zombies spawn on a budget, hunt the player, and kill them.

**Room generation** (`Source/ZombieGame/Rooms/`)

- `FRoomGrid` — parses a handcrafted tile layout (`#` wall, `.` floor, `D` doorway, `O` obstacle,
  `S` zombie spawn, `P` player start, ` ` outside) into tiles, doorways and spawn cells; validates
  that doorways sit on module edges and that no walkable pocket is sealed off. Supports rotation.
- `FSectorLayoutBuilder` — seeded assembly of modules into a sector: picks a start room, attaches
  weighted modules to open doorways in any rotation, rejects overlapping footprints, and validates
  the result independently (all rooms reachable from the start, a reachable exit room, no
  overlaps). Plain C++, no UObject/world/asset dependency, so ARCHITECTURE.md §17's room-graph
  tests can run without loading a level.
- `URoomTemplateDataAsset` — a handcrafted module as content. New room = new Data Asset.
- `ARoomModule` — realises a module as Instanced Static Mesh geometry; unconnected doorways are
  sealed back into wall so a sector never opens onto the void.
- `USectorGeneratorComponent` + `USectorGenerationSettings` — runs the builder, spawns modules,
  places the `APlayerStart` in the generated start room, and collects zombie spawn points.

**Zombies** (`Source/ZombieGame/Characters/Zombies/`)

- `UZombieArchetypeDataAsset` — stats, spawn economy (tier/cost/weight/min sector), perception
  ranges, and an optional Behavior Tree override, all as content.
- `AZombieCharacter` — Health/Damage components, archetype-driven stats applied before BeginPlay
  via deferred spawn, melee through Unreal's standard damage path, death → reward + corpse cleanup.
- Content: `DA_Zombie_Common` (cost 1), `DA_Zombie_Runner` (cost 2), `DA_Zombie_Tank` (cost 8,
  unlocks sector 2) — costs follow the design brief's worked example.

**Zombie AI** (`Source/ZombieGame/AI/`)

- `AZombieAIController` — AI Perception (sight + hearing, archetype-configured), translating
  stimuli into Blackboard state and nothing more; all decisions live in the tree.
- `UZombieAIAssetSubsystem` — assembles the shared Blackboard and Behavior Tree in C++ (see
  ARCHITECTURE.md §6 for why, and for the editor-authored override seam).
- BT nodes: `UBTService_ZombieCombatState`, `UBTTask_ZombieAttack`,
  `UBTTask_ZombieFindRoamLocation`, `UBTTask_ZombieMoveTo`, `UBTTask_ZombieClearBlackboardValue`,
  `UBTDecorator_ZombieBlackboardKeySet`. Behaviour priority: chase/attack a seen target →
  investigate a heard noise → roam.

**Spawn Director** (`Source/ZombieGame/Core/`)

- `USpawnDirectorComponent` + `USpawnDirectorSettings` — difficulty budget → zombie cost table →
  weighted selection → spawn queue, drained on a timer with a concurrency cap and a minimum spawn
  distance from the player. Sector clears when the budget is spent *and* nothing is left alive.
- `AZombieGameMode` — sequences level generation and encounter start, pays out money/kills to
  `AZombiePlayerState`, and advances sectors.

**Supporting changes**

- `FZombiePrimaryAssetLoader` + `Config/DefaultGame.ini` Asset Manager scan rules: room templates
  and zombie archetypes are discovered by type, so new content needs no registration step.
- `UHealthComponent::SetMaxHealth`, `UDamageComponent::GetLastDamageInstigator` (kill credit).
- `Config/DefaultEngine.ini`: runtime-dynamic Recast navmesh, since rooms are spawned at runtime.
- `L_TestSector`: legacy greybox floor plane removed (modules bring their own floors), navmesh
  bounds volume added.
- Verified at runtime headlessly: sector assembled from 7 modules with 16 spawn points, budget 60
  spent on 56 zombies, tree assembled, zombies roamed, acquired the player and took them from
  100 HP to 0.

### Fix — room floors were visible but not solid

- Room geometry ISM components were `Static` mobility. Static mobility means "final at level
  load"; these components are created at runtime, and instances added to a registered static ISM
  get half-built collision — some rooms' floors blocked, others silently did not, and pawns
  spawned in those rooms fell through the world forever. Now `Movable`, and room actors are spawned
  deferred so their geometry exists before component registration.

### Tweak — angled top-down camera

- `AZombiePlayerCharacter` camera pitch is now a tunable `EditDefaultsOnly` property
  (`CameraPitch`, default -75°) instead of a hardcoded -90°, per user request for a slightly
  angled top-down look rather than a flat straight-down shot.

### Fix — "no character on Play" (GameMapsSettings ini section)

- Root cause: `GameInstanceClass`/`GlobalDefaultGameMode` were under `[/Script/Engine.Engine]`
  in `Config/DefaultEngine.ini` since Milestone 1 — wrong section (confirmed against
  `BaseEngine.ini`), silently ignored, no error anywhere. Every build was actually running
  vanilla `AGameModeBase`, so `DefaultPawnClass` never applied. Moved both keys (plus new
  `EditorStartupMap`/`GameDefaultMap`) to `[/Script/EngineSettings.GameMapsSettings]`.
- Added `Content/Maps/L_TestSector.umap` (Floor, PlayerStart, DirectionalLight, SkyLight) — the
  project previously had no map at all, so the editor was opening an unrelated fallback
  "Untitled" scene. Set as `EditorStartupMap`/`GameDefaultMap`.
- Added a `BodyMesh` (`UStaticMeshComponent`, engine basic cylinder) to `AZombiePlayerCharacter`
  as a temporary visible placeholder — the capsule had no visual representation at all before.
- Verified with two independent runtime log checks (not just a successful compile):
  `LogZombieGame: ZombieGameInstance initialized` and `LogLoad: Game class is 'ZombieGameMode'`
  both now appear on launch. Also directly logged the live camera transform from `BeginPlay`,
  confirming `Pitch=-90°` (true top-down) on the correctly-spawned `ZombiePlayerCharacter` pawn.

### Fix — fatal error loading the uproject

- `AZombiePlayerCharacter`'s constructor built Enhanced Input modifiers with `NewObject<>()`
  instead of `CreateDefaultSubobject<>()`, which compiles but fatals the engine on CDO
  construction (i.e. the moment the project loads). Fixed by switching the four modifier
  instances (`MoveNegateA`, `MoveSwizzleW`, `MoveSwizzleS`, `MoveNegateS`) to
  `CreateDefaultSubobject`. Verified by rebuilding and launching `UnrealEditor.exe` headlessly —
  log now reaches asset registry completion with no fatal/error lines.

### Milestone 2 — Player

- Added `UHealthComponent`, `UStaminaComponent`, `UDamageComponent`, `IInteractable` +
  `UInteractionComponent` under `Source/ZombieGame/Components/`.
- Added `AZombiePlayerCharacter` under `Source/ZombieGame/Characters/Player/` — top-down
  orthographic camera, Enhanced Input (Move/Sprint/Interact/Pause) built entirely in C++, mouse
  cursor deprojection for aim rotation.
- Added `PublicIncludePaths.Add(ModuleDirectory)` to `ZombieGame.Build.cs` so module-root-relative
  includes (e.g. `"Components/HealthComponent.h"`) work from any subfolder, instead of fragile
  `../../` chains.
- Wired `AZombiePlayerCharacter` as `DefaultPawnClass` on `AZombieGameMode`.
- Verified clean build: `ZombieGameEditor` Win64 Development, `Result: Succeeded`, 0 errors.
  **Milestone 2 complete.** `InventoryComponent`/`WeaponComponent` deferred to Milestone 3.

### Milestone 1 — Core Framework

- Added `UZombieGameInstance`, `AZombieGameMode` (`AGameModeBase`), `AZombieGameState`
  (`AGameStateBase`), `AZombiePlayerState` under `Source/ZombieGame/Core/`.
- Added `LogZombieGame` log category to the `ZombieGame` module.
- Wired `GameInstanceClass`/`GlobalDefaultGameMode` via new `Config/DefaultEngine.ini`.
- Verified clean build: `ZombieGameEditor` Win64 Development, `Result: Succeeded`, 0 errors.
  **Milestone 1 complete.**

### Milestone 0 — Project Scaffold

- Pivoted engine from Godot 4.7.1/C# to Unreal Engine 5.8/C++ per updated project spec.
- Installed VS2022 Build Tools (C++ desktop workload) alongside pre-existing VS2019 (untouched).
- Created `Source/ZombieGame/...` and `Content/...` folder structure per `ARCHITECTURE.md` §19.
- Added `ZombieGame.uproject`, `ZombieGame.Target.cs`, `ZombieGameEditor.Target.cs`, and a
  minimal `ZombieGame` runtime module (`ZombieGame.Build.cs`/`.h`/`.cpp`) with dependencies on
  Core, CoreUObject, Engine, InputCore, EnhancedInput, AIModule, GameplayTags, UMG, Niagara.
- Generated VS Code project files via `UnrealBuildTool -projectfiles -vscode`.
- Rewrote `.gitignore` for Unreal (Binaries/, Intermediate/, Saved/, DerivedDataCache/, etc.),
  replacing the Godot-specific version.
- Verified clean build: `ZombieGameEditor` Win64 Development via `Build.bat`, `Result: Succeeded`,
  0 compile errors, 7/7 actions in ~128s. **Milestone 0 complete.**
