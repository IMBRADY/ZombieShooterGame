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

**Milestone 2 complete.**

**Update (post-Milestone-2 fix pass):** pressing Play now actually shows something. This
required fixing a real bug (not part of Milestone 2's original scope, but blocking verification
of it): `GameInstanceClass`/`GlobalDefaultGameMode` were in the wrong `DefaultEngine.ini` section
since Milestone 1 (`[/Script/Engine.Engine]` instead of
`[/Script/EngineSettings.GameMapsSettings]`), so the custom GameMode/GameInstance never actually
applied — every "Play" was silently vanilla Unreal. Also added `Content/Maps/L_TestSector.umap`
(the project had no map at all before this) and a temporary `BodyMesh` placeholder (engine basic
cylinder) since the capsule had zero visual representation. See `CLAUDE.md` gotcha #3 for full
detail and `CHANGELOG.md` for the fix entry. Real sprite/flipbook art still pending (§2 direction
in ARCHITECTURE.md) — the cylinder is a greybox stand-in, not final.

Camera pitch adjusted from a flat -90° (straight down) to a tunable -75° default
(`CameraPitch` property) per user request — slightly angled top-down rather than pure
orthographic-from-above.

## Milestone 3 — Sector Generation, Zombies & AI

Brought forward ahead of Weapons at the user's request: the game had no rooms and no enemies, so
there was nothing to shoot at and nothing to verify a weapon against.

### Room generation

- [x] `FRoomGrid` — handcrafted tile-layout parser + validator (doorways on module edges, no
      sealed-off walkable pockets), with rotation support.
- [x] `FSectorLayoutBuilder` — seeded module assembly with overlap rejection, plus independent
      validation of the design's generation rules (all rooms connected, reachable exit, no
      overlapping geometry). Plain C++, no engine dependency — testable per ARCHITECTURE.md §17.
- [x] `URoomTemplateDataAsset` — a handcrafted module as content; new room = new Data Asset.
- [x] `ARoomModule` — ISM geometry from a tile grid; unconnected doorways sealed back to wall.
- [x] `USectorGeneratorComponent` + `USectorGenerationSettings` — spawns the sector, places the
      generated `APlayerStart`, collects zombie spawn points.
- [x] Content: 9 room modules (start, 3 combat, 3 hallway/corner, dead end, treasure).

### Zombies & AI

- [x] `UZombieArchetypeDataAsset` — stats, spawn economy, perception ranges, optional BT override.
- [x] `AZombieCharacter` — component-composed, archetype-driven, damage through Unreal's standard
      path, death → reward + corpse cleanup.
- [x] `AZombieAIController` — AI Perception (sight + hearing) writing Blackboard state only.
- [x] `UZombieAIAssetSubsystem` — shared Blackboard + Behavior Tree assembled in C++
      (ARCHITECTURE.md §6 covers the rationale and the editor-authored override seam).
- [x] BT nodes for attack, roam-point selection, move-to, key clearing, combat-state service and a
      key-is-set decorator. Priority: chase/attack → investigate noise → roam.
- [x] Content: Common / Runner / Tank archetypes (costs 1 / 2 / 8, per the design brief).

### Spawn Director

- [x] `USpawnDirectorComponent` + `USpawnDirectorSettings` — budget → cost table → weighted
      selection → spawn queue, timer-driven with a concurrency cap and minimum spawn distance.
- [x] Sector completion condition: budget spent **and** all zombies dead.
- [x] `AZombieGameMode` sequences generation → encounter → rewards → next sector.

### Chase pathing (follow-up pass)

- [x] `FSectorNavigationGrid` — sector walkability derived from the room layout.
- [x] `UZombieFlowFieldSubsystem` — one distance field flooded outward from the players, shared by
      the whole horde, so chase pathing cost stops scaling with zombie count. Multi-source, so it
      is multiplayer-ready by construction. See ARCHITECTURE.md §6.2.
- [x] `UBTTask_ZombieChaseTarget` — reads the field; falls back to direct steering when it cannot
      answer. Roaming/investigating deliberately still use ordinary navigation queries.
- [x] RVO avoidance so a shared-field horde spreads out rather than stacking.
- [x] `UZombieAISettings` Data Asset for roam/idle pacing and field rebuild interval.

### Verification

- [x] Clean build (`ZombieGameEditor`, Win64 Development, `Result: Succeeded`, 0 errors).
- [x] Runtime-verified headlessly: 7-room sector, 17 spawn points, budget 60 → 56 queued zombies,
      tree assembled. Zombies roam and idle when nothing is seen, acquire the player, close from
      1431uu to 158uu, and kill them (100 → 64 → 4 → 0 HP).

**Milestone 3 complete.**

**Deliberately out of scope here:** Lobber/Exploder/Poison/Armored/Necromancer archetypes (they
need projectiles and status effects, i.e. the Weapons milestone), bosses, and the exit door / shop
hand-off. Until the shop exists, a cleared sector auto-advances after a delay
(`AZombieGameMode::bAutoAdvanceOnSectorCleared`).

## Milestone 4 — Weapons

Not started. Planned scope: `BaseWeapon` + Hitscan/Projectile split, Weapon Data Asset pipeline,
ammo, reload, `InventoryComponent`/`WeaponComponent` (deferred since Milestone 2). Gunshots should
call `UAISense_Hearing::ReportNoiseEvent` — the zombie investigate branch is already built and
waiting for it.

## Later (not yet scoped in detail)

Player death/run-end flow, Shop/Economy, Perks, Loot, Boss system, remaining zombie archetypes,
UI/HUD, Audio (MetaSounds), Save System (mid-run + meta), level streaming for room modules, Steam
readiness (OnlineSubsystem), Automation Test suite, performance pass (pooling, Niagara, async
loading).
