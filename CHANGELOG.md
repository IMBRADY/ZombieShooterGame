# Changelog

Entries are per-milestone (see `ROADMAP.md`), not per-commit — git history already covers
commit-level detail. Newest first.

## Unreleased

### Crosshair alignment, the New Run freeze, and a shank in its own melee slot

Second playtest report (prompt.txt), item by item.

- **Bullet path not lining up with the crosshair.** Mouse aim deprojected the cursor onto a plane at
  capsule-centre height, but tracers (since the previous fix) are drawn on the sprite plane, ~72 units
  lower. Under the -75° camera those two planes put the same screen point ~19 units apart on the
  ground. So every tracer ran slightly beside the cursor, by an amount that changed with the aim
  direction. The cursor is now resolved on the same plane the sprites and tracers are drawn on
  (`AZombiePlayerCharacter::UpdateMouseAim`, `AZombieWeapon::VisualShotHeightAboveFeet`). Weapon
  spread is unchanged and still scatters shots on purpose (pistol ±2°, SMG ±6°, shotguns ±11–13°).
- **Stuck after New Run: no crosshair, can't move or turn.** `UZombieUIManager` is a local-player
  subsystem and lives for the whole session. Unreal creates player-owned widgets with the
  GameInstance as their outer, so they also survive level loads. The main menu (or death screen) left
  on the menu stack in the previous level was therefore still "on top" in the new one. That blocked
  gameplay input and kept the default cursor instead of the crosshair. The death screen's "no pause
  menu" setting also leaked into the next run. The manager now resets its menus, HUD, crosshair
  cursor and pause flags on `FWorldDelegates::OnWorldCleanup`. Verified headlessly by launching into
  the main menu and travelling to a sector. With the reset disabled, the log showed the stale
  `ZombieMainMenuWidget` on top with input blocked. With it enabled, the stack is empty, input is
  unblocked and the crosshair cursor is active.
- **Shank: a melee weapon in its own slot.**
  - Every player always carries it (`DA_RunSettings.MeleeWeapon` → `W_Shank`), in a melee slot
    separate from the gun slots. It is never counted, sold, traded, saved or dropped as a gun. Draw it
    with **V** (gamepad **Y**). The mouse wheel cycles guns → shank → guns. When *every* gun is
    completely dry, pulling the trigger draws the shank automatically instead of dry-clicking.
  - `UWeaponFireMode_Melee` strikes the single nearest enemy within reach (150 units to the target's
    edge) inside a 70° half-arc, with no wall in between. It favours whatever is closest to the aim line.
  - Stats: 35 damage, 1.6 attacks/s (hold to keep stabbing), 10% crit at ×1.5.
    `OneHitKillTiers = [Low]` kills shamblers in one blow at any sector, whatever their scaled health.
    Tougher zombies take several.
  - `bUsesAmmo = false`: no magazine, no reload, never runs dry. Stabbing is a small ordinary noise,
    not a gunshot, so it doesn't aggravate the floor.
  - The HUD shows a `V SHANK` slot after the gun slots (highlighted while drawn) and `--` in place of
    the ammo counter. New held-weapon sprite frame (the weapon sheet grew from 4×2 to 4×3) and three
    new swing sounds (`SFX_Shank_Swing_0..2`).
  - Trade-ins (shop and loot) always swap the last-drawn *gun*, never the shank.
    `UInventoryComponent::ActiveIndex` keeps naming that gun while the shank is out.
  - New `ZombieMeleeTest` cheat, also run at the start of `ZombieSmokeTest`. It confirms a full-health
    shambler dies to one stab and a Runner survives one (42 → 7 HP).

### Playtest fixes: the game is now called Rotshot, plus AI, hit detection, spawning and audio

User playtest report, item by item.

- **Renamed "Dead Sector" → "Rotshot".** Main-menu title (`ZombieMainMenuWidget.cpp`) and the window
  title (`ProjectDisplayedTitle` in `Config/DefaultGame.ini`). The module, target and `.uproject`
  are still `ZombieGame`. Renaming those is a disruptive rename with no player-visible effect.
- **Zombies pausing in place next to the player.** Four separate causes, all fixed:
  - The combat service measured attack reach in 3D while the chase task measured arrival in 2D. When
    they disagreed, the chase said "arrived", the attack branch said "out of range", and the zombie
    stood still. Both are now planar (`BTService_ZombieCombatState`, `BTTask_ZombieAttack`).
  - An unreachable noise froze the investigate branch. The move failed, the key stayed set, and the
    key's own decorator re-picked the branch every frame. The branch is now "investigate, or give up
    and forget the noise" (`ZombieAIAssetSubsystem::BuildInvestigateBranch`).
  - A zombie shoved onto an obstacle or wall cell got no flow direction and steered straight at the
    player, into the obstacle. `UZombieFlowFieldSubsystem::GetFlowDirection` now heads for the
    walkable neighbour cell nearest a player.
  - Stuck detection in `UBTTask_ZombieChaseTarget`: a chasing zombie that moves less than
    `StuckDistanceThreshold` (30) in `StuckCheckInterval` (0.75 s) follows a real navmesh path for
    `StuckRecoverySeconds` (1.5 s), then returns to the flow field.
- **Alerted zombies calming down mid-firefight.** Hearing a gunshot only set an investigate point.
  It never refreshed the chase, so a zombie that had lost sight dropped its target after 6 s even
  while the player kept shooting. Gunshots are now tagged (`AZombieAIController::GunshotNoiseTag`).
  Every zombie within `GunshotAlertRadius` (3200, walls don't muffle it) is **aggravated**: it hunts
  the shooter directly for `AggravatedMemorySeconds` (12 s), and every further shot restarts that
  timer. Archetype hearing ranges are raised to at least the alert radius. A target that is still
  visible, or within `ProximityAwarenessRadius` (550), is never forgotten.
- **Zombies shot from far away not reacting.** Taking damage now aggravates the zombie toward
  whoever shot it, from any range (`AZombieCharacter::HandleDamageReceived` →
  `AZombieAIController::Aggravate`). Related: a player within `ProximityAwarenessRadius` with a clear
  line to the zombie is noticed whichever way the zombie is facing. Sight has a cone, and a zombie
  walking away from you used to ignore you right behind it.
- **Shots rendering through a zombie's shoulder without hitting.** Two causes:
  - The collision capsule (radius 34) is narrower than the sprite drawn on it (~50 at the
    shoulders). Hitscan rounds now sweep a sphere of `UWeaponDataAsset::ShotHitRadius` (20) against
    bodies. Walls still use a thin line, so rounds still thread gaps.
  - Tracers were drawn at capsule-centre height, about 80 units above the flat character sprites.
    Under the angled camera that shifted every tracer on screen compared with the floor-level sprites,
    so a tracer could appear to cross a zombie the round had actually missed. Tracers, impacts and
    muzzle flashes are now drawn just above the sprite plane (`AZombieWeapon::ToVisualShotHeight`).
- **Zombies colliding with corpses.** The corpse's capsule collision was already off, but the corpse
  stayed registered with RVO avoidance, so the living horde steered around and jostled against
  every body. Avoidance is now switched off on death.
- **Odd shadows on zombies standing over corpses.** Corpse and live sprites were flat masked quads
  at the same height, so they z-fought and the corpse's drop shadow and blood pool flickered through
  the live zombie. Corpses now lie lower (3 units above the feet instead of 10), with a lower
  translucency sort priority.
- **Zombie sounds fade with distance.** New `UZombieAudioSubsystem::PlayCreatureSoundAtLocation`.
  Full volume within `CreatureFullVolumeRadius` (350), quadratic fade to silence at
  `CreatureAudibleDistance` (2200), measured from the player pawn rather than the high camera
  listener. Every zombie sound goes through it: groans, screams, attacks, hurt, death, abilities,
  hazard puddles, exploder blasts. Sounds that would be inaudible aren't started at all.
- **No spawning on screen, and cleared rooms stay clear.** The Spawn Director now works with rooms
  (`FZombieSpawnArea`: a room's bounds plus its spawn points, from
  `USectorGeneratorComponent::GetZombieSpawnAreas`), not a flat point list.
  - A spawn point inside any player's camera view, plus `OffscreenSpawnMargin` (250), is never
    used. This is tested in camera space against the real orthographic view, so it also works
    headless.
  - A room is **cleared** once the player has been in it and no zombie is left alive inside it.
    After that nothing spawns there again (`bKeepClearedRoomsClear`).
  - Two fallbacks so a sector can always be finished. If *every* room is cleared, the remaining
    budget comes in through cleared rooms, still off screen. If every point stays on screen for
    `MaxOffscreenWaitSeconds` (10 s), the point furthest from the player is used.
- **New zombie and gunshot sounds** (`Tools/AssetGen/generate_audio.py`). The old gunshots were
  bit-crushed (the pistol had 246 distinct sample levels, which is exactly the 8-bit sound). The
  zombie voices were a raw sawtooth. Both are now modelled acoustically:
  - Gunshots: a broadband crack, then a muzzle blast whose filter collapses within ~20 ms, then a low
    body thump. Pistol, SMG and rifle add the clack of the action cycling. All of it is played
    through a synthetic concrete-room reverb.
  - Voices: a glottal pulse source with jitter, shimmer and vocal fry, through morphing vowel formant
    filters, with breath noise, throat rattle, low-pass and room reverb. Separate recipes cover
    groans, shrieks, tank roars, poison gargles, necromancer hisses, hurt grunts, deaths (with a gore
    layer), swipes, boss roars, acid splashes and casts.
  - File names are unchanged, so no Data Asset changes are needed. Pickup and UI blips stay
    deliberately retro.

### Automation test suite, and a unity-build clash it exposed

- **`Source/ZombieGame/Tests/`** — 20 Unreal Automation Tests under `ZombieGame.*`, covering the
  ARCHITECTURE.md §17 list: room grid parsing / malformed-layout rejection / rotation / optional
  obstacles; the sector generation rules checked independently of the builder over 250 seeded
  layouts across sectors 1-10 (no overlapping footprints, every room reachable, start and exit
  distinct, combat quota, min-sector gating, boss arena is the exit on boss sectors); seed
  determinism (mid-run saves depend on it); the weapon stat pipeline (rarity, upgrade, perks, crit
  cap); perk stacking (flat before percent, percents add) and restored-tier clamping; armor absorbs
  before health, death and healing rules; perk / shop / slot / weapon pricing; spawn budget,
  per-sector scaling and elite tier mix. Each test builds its own transient Data Assets, so
  rebalancing content never breaks a test. All pass.
- **Fix:** `ZombieCharacter.cpp` and `ZombiePlayerCharacter.cpp` both defined
  `SpriteHeightAboveFeet` / `WalkAnimationSpeedThreshold` in anonymous namespaces. Harmless until
  adding files regrouped the unity build and put them in the same translation unit (C2374). The
  zombie's copies are now `Zombie`-prefixed.
- `ZombieSmokeTest` now logs every shop transaction result and reports slot 0 (the slot it
  upgrades) rather than the active weapon, which made a working upgrade look broken.
- `AZombieGameMode::EndRun` logs the run summary, so a headless run shows the run actually ended.

### Complete game loop (commit `b3c6f91`)

Weapons (hitscan/projectile, rarity, upgrades), inventory, event-driven UMG HUD, key → exit →
intermission shop → next sector, perks, loot tables, status effects, Lobber / Exploder / Poison /
Armored / Necromancer, two multi-phase bosses every 5 sectors, main/pause/settings/death menus,
mid-run and meta saves, achievements, audio manager with combat-intensity music, generated pixel
art. See `ROADMAP.md` Milestone 4 for the itemised list.

### Milestone 3 follow-up — shared chase pathing, and the reason zombies stood still

User report: "zombies spawn but they don't try to chase and kill the player". Two things, one a
requested design change and one a real bug.

**Chasing now uses one shared flow field** (`Source/ZombieGame/AI/ZombieFlowFieldSubsystem.cpp`),
per the user's direction to compute pathing as a radius outward from the player so every zombie
reads the same data instead of solving its own path.

- `FSectorNavigationGrid` — sector walkability, built from the room layout (which is already an
  exact tile map of the spawned geometry) rather than queried back out of the navmesh.
- `UZombieFlowFieldSubsystem` — floods that grid outward from **every player pawn at once**,
  recording each cell's distance to the nearest player, and derives a downhill direction per cell
  from the local gradient (so movement is diagonal, not staircased). One flood per rebuild
  interval serves the entire horde, so cost no longer scales with zombie count. Multi-source
  seeding means a second player needs no changes.
- `UBTTask_ZombieChaseTarget` replaces the per-zombie `MoveTo` in the combat branch, falling back
  to direct steering when the field cannot answer. Roaming and investigating still use ordinary
  navigation queries — they go to arbitrary points and happen rarely.
- RVO avoidance on zombies, so a horde descending one shared field spreads across the corridor
  instead of stacking into a column.
- `UZombieAISettings` Data Asset — roam radius, idle pacing, field rebuild interval, and a switch
  to A/B the field against per-zombie pathing.

**Idle behaviour retuned** to the requested shape: shorter wander hops (800uu) with a short pause
after each, so zombies that have not seen anything mill around their patch instead of trekking.

**Fix — zombies spawned and then never moved at all.** A `RecastNavMesh` actor had been saved into
`L_TestSector`. The level is empty at edit time (every room is spawned at runtime), so the baked
navmesh was empty, and its serialized settings overrode the `RuntimeGeneration=Dynamic` project
default — config defaults only apply to freshly created instances. Every navigation query failed,
so the roam branch failed instantly and the Behavior Tree spun with no active task. Navigation
data is no longer saved into the level; `bAutoCreateNavigationData` recreates it at runtime.

**Fix — `SetAvoidanceEnabled` called from a constructor.** It needs a character owner and a world
to register with the avoidance manager and has neither during construction, leaving avoidance
flagged on but unregistered. Moved to `BeginPlay`.

**Tuning** — zombie sight radii raised (Common 1400 → 2600) so a zombie notices the player from
across the room it is standing in rather than only at close range.

Verified headlessly: 29 zombies, 21 moving, 6 holding targets, closing from 1431uu to 158uu, player
100 → 64 → 4 HP.

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
