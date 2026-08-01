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

- [x] `UHealthComponent` — 100 max HP + armor pool, armor absorbs first (`ApplyDamage`),
      `Heal`/`AddArmor` hooks for the future Shop/Loot milestones, `OnHealthChanged`/
      `OnArmorChanged`/`OnDeath` delegates, replicated.
- [x] `UStaminaComponent` — 100 max, 0.5s recharge delay, 20/sec recharge (both from spec);
      drain rate is a tunable default (25/sec) since the spec didn't give one. Force-stops
      sprinting on exhaustion even without an external release event. Replicated.
- [x] `UDamageComponent` — thin bridge from `AActor::OnTakeAnyDamage` (standard Unreal damage
      entry point — `UGameplayStatics::ApplyDamage` et al. will drive this from Weapons later)
      to `HealthComponent::ApplyDamage`. Keeps damage *routing* separate from health/armor
      *state* so future modifiers (crits, armor-piercing, poison/fire from the PDF's zombie
      types) plug in here without touching `HealthComponent`.
- [x] `IInteractable` interface + `UInteractionComponent` — finds the nearest overlapping
      interactable each tick, `TryInteract()` calls it. No interactable actors exist yet (Shop/
      Loot/Rooms milestones), but the contract is real and functional now.
- [x] `AZombiePlayerCharacter` — top-down orthographic camera (`SpringArm` pitched -90°,
      `Camera` in `Orthographic` projection mode, no lag so the player stays exactly centered
      per spec), Enhanced Input for Move (WASD, world-space so it stays decoupled from mouse
      aim), Sprint (Left Shift), Interact (E), Pause (Escape) — all Input Actions/Mapping
      Context constructed in C++ (`CreateDefaultSubobject`), not editor-authored `.uasset`
      files, so input setup lives in source control like every other gameplay system. Mouse-aim
      rotation via cursor deprojection onto the character's own height plane.
- [x] Wired as `DefaultPawnClass` on `AZombieGameMode`.
- [x] Clean build verified (0 errors).

**`InventoryComponent` and `WeaponComponent` deliberately deferred to Milestone 3** — both are
fundamentally about holding/firing weapons, and no weapon type exists yet. Building them now
would mean either empty shells or loosely-typed placeholders that get reworked anyway; better to
build them together with `BaseWeapon`/`Hitscan`/`Projectile`.

No visual representation yet (no sprite/flipbook, no mesh) — `ACharacter`'s default mesh slot is
unset, so the capsule is invisible. That's a separate art-integration pass (Paper2D or billboard
setup), not part of this milestone's mechanical scope, and there's no level to test in yet
either. **Still can't "press Play and see something"** after this milestone — that needs at
minimum a test level with a floor + PlayerStart, which was intentionally skipped earlier.

**Milestone 2 complete.**

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
