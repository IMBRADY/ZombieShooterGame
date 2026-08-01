# Changelog

Entries are per-milestone (see `ROADMAP.md`), not per-commit — git history already covers
commit-level detail. Newest first.

## Unreleased

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
