# Changelog

Entries are per-milestone (see `ROADMAP.md`), not per-commit — git history already covers
commit-level detail. Newest first.

## Unreleased

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
