# Roadmap

Tracks milestone-level progress. Updated at the end of every milestone per the build pipeline in
`ARCHITECTURE.md` §20. See `CLAUDE.md` for session-to-session working state and open questions;
this file is the durable, high-level record of what's shipped.

## Milestone 0 — Project Scaffold

Goal: an empty, cleanly-buildable UE 5.8 C++ project matching the structure in
`ARCHITECTURE.md` §19, with the toolchain (VS2022 Build Tools + VS Code) verified working.

- [x] VS2022 Build Tools (C++ workload) installed, verified via `vswhere`.
- [x] `Source/ZombieGame/...` and `Content/...` folder structure created.
- [x] `ZombieGame.uproject`, target files, and minimal `ZombieGame` module created.
- [x] VS Code project files generated (`UnrealBuildTool -projectfiles -vscode`).
- [x] Clean build of `ZombieGameEditor` (Win64, Development) verified — `Result: Succeeded`,
      0 errors, 7/7 actions, ~128s total (first build, UnrealEditor-ZombieGame.dll linked).

**Milestone 0 complete.**

## Milestone 1 — Core Framework

- [x] `UZombieGameInstance` (`UGameInstance`) — bare skeleton + `LogZombieGame` category, logs on
      `Init()`. Save/Settings/Steam responsibilities land in their own later milestones.
- [x] `AZombieGameMode` (`AGameModeBase` — not the full match-state `AGameMode`; this game has no
      lobby/ready-up/spectator flow, so the simpler base avoids unused machinery). Wires
      `GameStateClass`/`PlayerStateClass`. Owns `AdvanceToNextSector()` (real, working: bumps
      `CurrentSector` and a simple linear `DifficultyLevel` — the real difficulty-curve Data
      Asset from `ARCHITECTURE.md` §8 is still future work, tracked under "Later" below).
- [x] `AZombieGameState` (`AGameStateBase`) — replicated `CurrentSector`, `DifficultyLevel`,
      `SpawnBudget`, `RemainingEnemies`, `ActiveZombies`. `OnSectorChanged` delegate for future
      HUD sector-number display.
- [x] `AZombiePlayerState` (`APlayerState`) — replicated `Money`, `Kills`, `BossesDefeated`,
      `DamageTaken`; `AddMoney`/`SpendMoney` (guarded, real logic) with `OnMoneyChanged` delegate
      for future HUD. **Perks and inventory references deliberately deferred** — spec lists them
      on PlayerState, but the Perk Data Asset system and InventoryComponent don't exist yet
      (Perks/Player milestones); adding placeholder fields now would just mean reworking them
      later for no benefit.
- [x] Wired via `Config/DefaultEngine.ini` (`GameInstanceClass`, `GlobalDefaultGameMode`).
- [x] Clean build verified (`ZombieGameEditor`, Win64 Development, `Result: Succeeded`, 0 errors).

**Milestone 1 complete.**

## Milestone 2 — Player

Not started. Planned scope: player `Character` built from `HealthComponent`,
`StaminaComponent`, `InventoryComponent`, `WeaponComponent`, `InteractionComponent`,
`DamageComponent`; Enhanced Input bindings for movement/aim/sprint/interact/pause.

## Milestone 3 — Weapons

Not started. Planned scope: `BaseWeapon` + Hitscan/Projectile split, Weapon Data Asset pipeline,
ammo, reload.

## Milestone 4 — Enemies & AI

Not started. Planned scope: zombie base Behavior Tree/Blackboard/Perception setup, Common
archetype, then Runner/Tank/Lobber.

## Later (not yet scoped in detail)

Spawn Director, Room Generation/streaming, Shop/Economy, Perks, Loot, Boss system, UI/HUD,
Audio (MetaSounds), Save System (mid-run + meta), Steam readiness (OnlineSubsystem),
Automation Test suite, performance pass (pooling, Niagara, async loading, ISM).
