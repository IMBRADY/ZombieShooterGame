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

Not started. Planned scope: `GameInstance`, `GameMode`, `GameState`, `PlayerState` skeletons per
`ARCHITECTURE.md` §3, wired together but not yet doing gameplay.

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
