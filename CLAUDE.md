# Project Progress Log

Zombie roguelike shooter, Godot 4.7.1 (C#/.NET), per `prompt.txt` (authoritative instructions)
and `Zombie Video Game Sketch (3).pdf` (design pitch). Read both before resuming work.

## Decisions locked in (do not re-litigate without asking the user)

- Engine: Godot 4.7.1 mono, found pre-downloaded at
  `C:\Users\brade\Desktop\Godot_v4.7.1-stable_mono_win64\Godot_v4.7.1-stable_mono_win64.exe`.
  Not installed via installer — it's a portable extracted zip.
- .NET 8 SDK installed via winget alongside the pre-existing .NET 6 SDK (both present; do not
  remove .NET 6). Godot 4.7 C# projects target `net8.0`.
- Git: this folder (`Desktop\Zombie Video Game`) has its own independent git repo. **Do not**
  confuse this with the separate, empty, no-commit git repo rooted at `C:\Users\brade` — they
  are unrelated, leave the home-directory one alone.
- Save system scope: **mid-run save/resume is in scope** (user explicitly chose this over the
  PDF/prompt.txt's conflicting meta-only guidance — see ARCHITECTURE.md §9). Meta-data
  (settings/stats/highscores/unlocks) also persists independently.
- Core architectural pattern: engine-agnostic POCO gameplay logic + thin Godot Node adapters
  (ARCHITECTURE.md §3.1) — this is required for the spec's unit-testing requirement and should
  not be abandoned for convenience later.

## Phase status

- [x] Phase 0 — Environment: .NET 8 SDK installed, git repo initialized, initial commit made.
- [x] Phase 1 — Architecture document: `ARCHITECTURE.md` written and complete.
- [ ] Phase 2 — Folder structure (next up)
- [ ] Phase 3 — Class diagrams
- [ ] Phase 4 — Resources
- [ ] Phase 5 — Core engine implementation
- [ ] Phase 6 — Player implementation
- [ ] Phase 7 — Weapons implementation
- [ ] Phase 8 — Enemies implementation
- [ ] ... continues incrementally per prompt.txt Workflow section (procedural generation, shop,
      perks, save system, UI, audio, debug tooling, Steam-readiness pass, testing pass)

## Open questions for the user (none blocking right now)

None outstanding. If a new spec conflict or ambiguity is found in a later phase, log it here
under this heading before proceeding, per the "ask questions when necessary" instruction in
`prompt.txt`.

## Next-session resume point

Start Phase 2: create the professional Godot project folder structure per ARCHITECTURE.md §13
(this is a preview — expand it as needed during Phase 2, updating both this file and
ARCHITECTURE.md §13 if the structure changes). Then generate the actual Godot project file
(`project.godot`) targeting the found Godot 4.7.1 mono executable and `net8.0`.
