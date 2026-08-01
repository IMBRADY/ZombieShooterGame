# Architecture Document — [Working Title] Zombie Roguelike

Phase 1 deliverable. Defines the system boundaries, patterns, and contracts that every later
phase must conform to. Nothing here is implementation — see `CLAUDE.md` for phase status.

## 1. Design Pillars (from spec)

1. Simple mechanics, difficult gameplay, meaningful progression, high replayability, satisfying combat.
2. Player should feel constantly under-equipped; every shop visit matters.
3. New weapons/perks/zombies/rooms/bosses addable via data only — no code changes.
4. Built for eventual Steam release and eventual multiplayer, without rewrites.

## 2. Engine & Tooling

- **Engine:** Godot 4.7.1 (.NET/Mono build), satisfies "4.4+" requirement.
- **Language:** C# only. GDScript avoided per spec, used only if Godot forces it (none expected).
- **Target Framework:** `net8.0` (Godot 4.3+ default). .NET 8 SDK installed alongside the
  existing .NET 6 SDK — do not remove .NET 6, other tooling on this machine may depend on it.
- **Version control:** git repo scoped to this folder (independent from the empty repo at
  `C:\Users\brade`).

## 3. Core Architectural Style

### 3.1 Engine-agnostic core, thin Godot adapters

This is the single most load-bearing decision in this document, because the spec explicitly
requires unit tests for inventory, weapon math, perk stacking, damage, procedural generation,
and economy — and Godot `Node`-derived classes are impractical to unit test without a running
engine.

Rule: **gameplay logic lives in plain C# classes (POCOs) with zero `Godot.Node` /
`Godot.GodotObject` dependencies wherever the logic is pure computation or state transition.**
Godot `Node` classes exist only as thin adapters that:

- read engine input (`_Input`, `_PhysicsProcess`) and forward it into the POCO layer as intents,
- read POCO state and drive scene-tree side effects (sprites, sounds, particles, `Signal` emission),
- never contain branching gameplay rules themselves.

Examples:
- `WeaponCalculator` (POCO) computes damage/spread/crit from `WeaponData` + `PerkState` →
  tested with plain xUnit/GdUnit4 tests, no scene tree required.
- `WeaponNode` (Godot `Node2D`) owns a `WeaponCalculator`, plays muzzle-flash/sound on fire,
  reads mouse position for aim.

This split is mandatory for: `EconomyManager`, `InventorySystem`, `PerkManager`'s stacking math,
`SpawnDirector`'s budget allocation, `RoomGenerator`'s graph assembly/validation, and all damage
calculations. It is not required for pure presentation (VFX, audio triggers, camera).

### 3.2 Event-driven communication

A single Autoload, `EventBus` (`res://Core/EventBus.cs`), exposes strongly-typed C# `event`
members (not stringly-typed signals) for cross-system communication, e.g.:

```csharp
event Action<ZombieDiedEvent> ZombieDied;
event Action<SectorCompletedEvent> SectorCompleted;
event Action<MoneyChangedEvent> MoneyChanged;
```

Rules:
- Systems publish events; they never hold direct references to unrelated systems
  (e.g. `HealthSystem` does not know `UIManager` exists — it fires `HealthChanged`, `UIManager`
  subscribes).
- Godot node-local concerns (a weapon telling its own muzzle-flash particle to play) use Godot
  `[Signal]` directly — no need to route trivial local effects through the global bus.
- Event payloads are immutable `record` structs, not the emitting object itself, to keep
  subscribers decoupled from emitter internals.

### 3.3 Service access

Godot has no built-in DI container. To keep SOLID/testability without fighting the engine:

- Every manager-level system (`EconomyManager`, `PerkManager`, `ShopManager`, `SaveManager`,
  `GameStateManager`, `AudioManager`) is defined first as a C# **interface**
  (`IEconomyManager`, `IPerkManager`, ...) implemented by a concrete class.
- A single Autoload, `Services` (`res://Core/Services.cs`), is a minimal service locator:
  registers concrete instances at startup against their interfaces, resolved via
  `Services.Get<IEconomyManager>()`.
- This is deliberately the *only* place that looks like a locator/singleton soup. Everything
  downstream depends on interfaces, so tests can register fakes instead of real managers.

### 3.4 Composition over inheritance for entities

Player and zombies are not deep class hierarchies. Each is a Godot scene (`CharacterBody2D` root)
composed of small, focused component nodes, each implementing a narrow interface:

- `HealthComponent : IDamageable`
- `StaminaComponent`
- `ArmorComponent : IDamageable` (intercepts before `HealthComponent`)
- `WeaponHolderComponent`
- `AIComponent` (zombies only — owns a `StateMachine`)
- `HitboxComponent` / `HurtboxComponent`

New enemy archetypes are assembled by combining components in the scene editor plus swapping a
`ZombieData` Resource — not by subclassing a `Zombie` base class per archetype.

## 4. Data-Driven Content Pipeline

All tunable content is a `Godot.Resource` subclass, authored as `.tres` files, loaded at
runtime. No gameplay constant is hardcoded in a script.

| Content | Resource type | Notes |
|---|---|---|
| Weapons | `WeaponData` | damage, spread, fire rate, mag size, reload speed, ammo type, rarity, upgrade curve, special-effect IDs |
| Perks | `PerkData` | tiered cost curve (exponential), effect IDs, stacking rules |
| Zombies | `ZombieData` | HP, damage, speed, spawn budget cost, FSM state overrides, special-state IDs |
| Room templates | `RoomTemplateData` + `.tscn` | room type (Start/Combat/Hallway/DeadEnd/Treasure/Boss/Exit), door sockets, difficulty tags |
| Shop pools | `ShopPoolData` | weighted probability tables per item category |
| Bosses | `BossData` | phase list, per-phase attack pattern references |

**Special effects / perk effects problem:** rarity modifiers, weapon special effects, and perk
effects can't be pure data (they're behavior). Solved via a **strategy registry**: gameplay code
defines small `IWeaponEffect` / `IPerkEffect` implementations (e.g. `RicochetEffect`,
`PoisonBulletsEffect`, `LifeStealEffect`), each tagged with a string ID via an attribute. A
`res://Resources/*.tres` file references effects by ID string; a startup registry
(`EffectRegistry`) reflects over the assembly once to map ID → implementation. Adding a new
*combination* of existing effects to a new weapon/perk is data-only. Adding a genuinely new
effect requires one small class implementing the interface (unavoidable — it's new behavior),
but zero changes to `WeaponSystem`, `PerkManager`, or any switch statement.

Perks and zombie archetypes are additionally **auto-discovered**: `PerkManager` and
`EnemyManager` scan `res://Resources/Perks/` and `res://Resources/Zombies/` at startup and
register every Resource found by its ID field. Adding a perk or zombie type is: drop a new
`.tres` file in the folder (+ an effect class only if new behavior is needed). No switch
statements, per spec.

## 5. Core Systems

Each system below: responsibility, key interface, events published/consumed, Godot-vs-POCO split.

- **GameStateManager** — top-level state machine (MainMenu → InSector → Intermission → Death →
  MainMenu). Owns run lifecycle. Publishes `GameStateChanged`.
- **PlayerController** (Node) — WASD movement, mouse aim, sprint input, weapon-slot input,
  interact/pause input. Forwards intents to `StaminaSystem`/`WeaponSystem`.
- **HealthSystem / ArmorSystem / StaminaSystem** (POCO + thin Node) — HP 100 max, armor absorbs
  first, stamina 100 max / 20 per sec regen / 0.5s recharge delay per spec.
- **InventorySystem** (POCO) — 3 weapon slots, per-weapon ammo pools, swap/reload rules.
- **WeaponSystem** (POCO `WeaponCalculator` + Node `WeaponNode`) — fire-rate gating, spread,
  crit roll, piercing, auto-reload when a magazine empties and reserve ammo exists.
- **EnemyManager** (Node, owns pooled zombie instances) — spawn/despawn, delegates AI ticking.
- **AI / StateMachine** — see §6.
- **SpawnDirector** (POCO) — budget-based encounter composition (§7).
- **RoomGenerator / Navigation** (POCO graph logic + Node instancing) — see §7.
- **LootSystem** (POCO) — drop tables (money, rare health drops, mystery box contents).
- **ShopManager** (Node/UI-facing) + **EconomyManager** (POCO) — shop inventory generation
  (weighted pools), pricing, purchase transactions, money persistence across sectors, money loss
  on death.
- **PerkManager** (POCO registry, §4) — active perk state, tier costs, stacking application order.
- **AudioManager** (Autoload Node) — music transition/layering by combat intensity, volume
  buses (Master/Music/SFX/UI), randomized SFX variation pools, 3D-ready (positional `AudioStreamPlayer2D`
  now, structured so a future 3D remaster only swaps player types).
- **SaveManager** (POCO serialization + Node for file I/O) — see §9.
- **UIManager** (Node) — HUD, pause menu, shop UI, death summary. **Strictly observes** game
  state via `EventBus`/`Services` reads; never mutates gameplay state directly (button handlers
  call into manager interfaces, they don't set gameplay fields themselves).

## 6. AI Architecture

- **Standard zombies:** custom lightweight FSM (`StateMachine` + `IState`), states per spec:
  `Idle → Roam → Investigate(Noise) → Chase → Attack → Stunned → Dead`. Gunshots register as
  noise events on the `EventBus` with a radius; `Investigate` state subscribes.
- **Special zombies** (Exploder, Lobber, Poison, Necromancer, Armored) add states via
  **composition**, not subclassing the FSM: each special behavior is an additional `IState` (e.g.
  `ExplodeOnDeathState` replacing `Dead`) wired in via `ZombieData`, so the base FSM class never
  branches on zombie type.
- **Bosses:** Hierarchical State Machine — a top-level `PhaseState` per boss phase, each phase
  owning its own child `StateMachine` of attack states. Boss phase transitions (HP thresholds)
  are data-driven via `BossData`.

## 7. Procedural Generation Architecture

Per spec: **no random geometry generation.** Handcrafted room scenes only, assembled
procedurally.

- **Authoring:** each room is a hand-built `.tscn` + a paired `RoomTemplateData` declaring its
  type and door sockets (position + direction) for connection matching.
- **Assembly (POCO, testable without the engine):** a graph-building step chooses a sequence of
  room templates (Start → N combat rooms/hallways/dead-ends → optional Treasure → optional Boss
  → Exit) and validates: full connectivity, no unreachable rooms, exactly one guaranteed path
  Start→Exit, no socket left dangling without either a connection or a capped dead-end piece.
  This graph/validation logic is pure data (room IDs + socket graph) — instantiation into actual
  `Node2D` scenes is a separate, later step. This split is what makes "no overlapping rooms" and
  "guaranteed exit path" unit-testable.
- **Instancing (Node):** once the graph validates, rooms are instanced and positioned using
  socket alignment (deterministic given the graph — no physics-based placement needed, so
  overlap is structurally impossible rather than checked-and-retried).
- **New content:** a new room template = new `.tscn` + `.tres` pair dropped in
  `res://Scenes/Rooms/<Category>/`. No code changes.

## 8. Progression & Economy

- **Spawn Director budget system** exactly per spec: each sector has a difficulty budget; each
  `ZombieData` declares a spawn cost; the director spends the budget across the zombie pool
  available at the current sector tier until exhausted. Sector ends when budget is exhausted AND
  all spawned zombies are dead.
- **Difficulty scaling:** per-sector multipliers (HP, damage, spawn count, elite chance, boss
  chance, reward multiplier) are POCO-computed from sector index via a data-driven curve
  (`DifficultyCurveData`), not hardcoded per-sector `if` statements.
- **Boss sectors:** every 5 sectors, per spec.
- **Shop generation:** weighted-probability pool draws (`ShopPoolData`) for 1–3 weapons, 1–3
  perks, plus fixed slots (ammo refill, armor, med kit, weapon upgrade, mystery box, leave).
- **Currency:** money auto-collected in pickup radius, persists across sectors, lost on death.

## 9. Save System

Per clarified scope: **the game supports mid-run save/resume**, in addition to persistent
meta-data.

- **Meta-save** (always persisted, survives death): settings, cumulative statistics, high
  scores, unlocks.
- **Run-state save** (mid-run checkpoint): current sector index, difficulty state, player
  inventory (weapons + upgrade levels + ammo), perks + tiers, armor, health, money. Written at
  sector/intermission boundaries (not mid-combat, to avoid mid-fight save-scumming or corrupt
  states) and on explicit quit-to-menu.
- Both are POCO-serializable DTOs (`RunSaveData`, `MetaSaveData`) versioned with a schema-version
  int from day one, so future field additions don't break old saves. `SaveManager` is the only
  system touching the filesystem.

## 10. Multiplayer-Readiness Guidelines (not implemented now)

Per spec: do not implement multiplayer, but do not paint into a single-player corner.

- Gameplay systems mutate state only through manager methods, never through direct field writes
  from `PlayerController`/UI. This is exactly the seam a future server-authoritative model
  intercepts (client sends intent → server-side manager validates/applies → broadcasts result).
- No system assumes exactly one player: `PlayerController`/`HealthSystem`/`InventorySystem`
  instances are tracked in collections keyed by player ID, even though that collection holds one
  entry today.
- `EnemyManager`/`SpawnDirector` state (spawn budget, zombie list) is a single authoritative
  instance, not per-player — matches a future host-authoritative model.

## 11. Steam-Readiness Guidelines (not implemented now)

- All platform integration (achievements, cloud saves, leaderboards, rich presence, Steam Input)
  is accessed through an `IPlatformService` interface. A `NullPlatformService` (no-op) is the
  only implementation for now. `SaveManager` calls `IPlatformService.SyncCloudSave()` etc.
  unconditionally — today it's a no-op, later it's a real Steamworks call, with zero changes to
  gameplay code.
- Controller support: input goes through Godot's `InputMap` action layer, never raw
  keyboard/mouse checks in gameplay code, so controller remapping/Steam Input is additive later.

## 12. Testing Strategy

- POCO layer (§3.1) is tested with plain xUnit-style tests requiring no running Godot engine, for:
  inventory rules, weapon damage/spread/crit math, perk stacking order and totals, damage
  application (armor-then-health), procedural generation graph validation (connectivity, no
  overlap, guaranteed path), economy math (pricing, exponential perk tier costs, transactions).
- Engine-dependent integration smoke tests (does a scene instance without erroring, does the FSM
  Node wire up signals correctly) are a smaller secondary suite using GdUnit4 (Godot-native test
  runner), added once core scenes exist — not part of Phase 1.

## 13. Folder Structure (preview — finalized in Phase 2)

```
res://
  Core/            EventBus, Services (locator), GameStateManager
  Gameplay/
    Player/
    Enemies/
    Weapons/
    Perks/
    Economy/
  Procedural/
    RoomGraph/       (POCO)
    RoomInstancing/  (Node)
  UI/
  Audio/
  Resources/         .tres data assets (Weapons/, Perks/, Zombies/, Rooms/, Shop/, Bosses/)
  Scenes/
    Rooms/<Category>/
  Save/
  Platform/          IPlatformService + NullPlatformService
```

## 14. Open Risks / Watch Items

- `.godot/` engine cache and C# `bin/`/`obj/` output are gitignored — first editor open will
  regenerate them; expect a pause on first launch while Godot imports assets.
- Godot 4.7 vs "4.4+" in the spec: using 4.7.1 since it's already downloaded and is a superset of
  4.4 requirements. Flagging in case a specific 4.4.x was intended for a compatibility reason.
- xUnit vs GdUnit4 split adds a second test runner to the toolchain; accepted tradeoff for
  testability given the spec explicitly requires unit tests for logic that would otherwise be
  hard to test inside Godot's Node lifecycle.
