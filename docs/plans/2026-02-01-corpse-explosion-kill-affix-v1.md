# Corpse Explosion (Kill Trigger) + Safe Chaining Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** Add a PoE/Diablo-style *Corpse Explosion* affix that procs on **Kill** with **moderate chaining** and **hard safety limits** (low-trigger, CTD-resistant), while keeping CK usage minimal and tooltips driven by LoreBox.

**Architecture:** Add `Kill` trigger support via `RE::TESDeathEvent` in the SKSE plugin, and implement a new runtime action `CorpseExplosion` that (1) computes damage as **flat + % of corpse max health**, (2) selects nearby hostile actors, (3) applies damage via existing dynamic damage spells using `CastSpellImmediate`, and (4) enforces **chain falloff + depth cap + global rate limits**. New affix tiers live in `affixes/affixes.json`, and generator outputs (KID + LoreBox translations) provide in-game tooltip text.

**Tech Stack:** SKSE + CommonLibSSE-NG (event sinks, `ProcessLists::ForEachHighActor`), nlohmann/json, spdlog, .NET 8 generator, KID, LoreBox.

---

## Web / Reference Notes (why these hooks)

- `RE::TESDeathEvent` exposes `actorDying`, `actorKiller`, and `dead` fields (CommonLibSSE struct docs / headers).
- `RE::ScriptEventSourceHolder` can provide a `BSTEventSource<TESDeathEvent>` and supports `AddEventSink<T>()` (CommonLibSSE-NG docs).
- `RE::ProcessLists::ForEachHighActor` exists for bounded iteration over loaded actors (CommonLibSSE-NG headers).

---

## Implementation Plan

### Task 1: Add a RED test asserting the repo spec contains CorpseExplosion (Kill)

**Files:**
- Add: `tools/CalamityAffixes.Generator.Tests/RepoSpecRegressionTests.cs`

**Step 1: Write failing test (RED)**
- Load `affixes/affixes.json` using `AffixSpecLoader.Load(...)`.
- Assert there is at least one affix with:
  - `runtime.trigger == "Kill"`
  - `runtime.action.type == "CorpseExplosion"`

**Step 2: Run tests to verify failure**
- Run: `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
- Expected: FAIL (until the affix is added to `affixes/affixes.json`).

---

### Task 2: Implement Kill trigger plumbing in SKSE

**Files:**
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`

**Steps:**
- Add `RE::BSTEventSink<RE::TESDeathEvent>` to `EventBridge`.
- Register it in `EventBridge::Register()`.
- Add `Trigger::kKill` and parse `"Kill"` in `LoadConfig()`.
- In `ProcessEvent(TESDeathEvent)`:
  - Guard nulls, require `dead == true`.
  - Attribute summon kills to player via existing `IsPlayerOwned` / `GetPlayerOwner`.
  - Call the kill handler (CorpseExplosion uses the dying actor as origin).

---

### Task 3: Implement the `CorpseExplosion` runtime action with safety limits

**Files:**
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- Modify: `skse/CalamityAffixes/src/EventBridge.cpp`

**Design defaults (Option 3 + Option 2):**
- Damage: `flat + pctOfCorpseMaxHealth`
- Chain: moderate chaining with per-explosion falloff (30–50%), depth cap, and global rate limiting.

**Implementation notes:**
- Add `ActionType::kCorpseExplosion` and parse action fields from runtime JSON:
  - `spellEditorId` (dynamic damage spell, e.g. `CAFF_SPEL_DMG_FIRE_DYNAMIC`)
  - `flatDamage`, `pctOfCorpseMaxHealth`
  - `radius`, `maxTargets`
  - `maxChainDepth`, `chainFalloff`, `chainWindowSeconds`
  - `rateLimitWindowSeconds`, `maxExplosionsPerWindow`
  - `noHitEffectArt`, `effectiveness`
- Execute by:
  - Compute corpse “max health” via `GetPermanentActorValue(Health)` (non-negative clamp).
  - Collect nearby **hostile** living actors within radius using `ProcessLists::ForEachHighActor`.
  - Cap targets by distance (`maxTargets`).
  - Apply damage by `CastSpellImmediate` per target with `magnitudeOverride = computedDamage`.
- Safety:
  - Per-corpse one-shot (formID → timestamp cache)
  - Global window cap (`maxExplosionsPerWindow`)
  - Chain depth cap + falloff multiplier (`pow(chainFalloff, chainDepth)`)
  - Optional minimum interval between explosions (derived from `chainWindowSeconds` or explicit field)

---

### Task 4: Add tiered Corpse Explosion affixes to the spec (GREEN for Task 1)

**Files:**
- Modify: `affixes/affixes.json`

**Steps:**
- Add `corpse_explosion_t1..t5` affixes:
  - `kid.type`: weapon (or armor) — pick one initial surface; expand later.
  - `runtime.trigger`: `"Kill"`
  - `runtime.action.type`: `"CorpseExplosion"`
  - `name`: Korean tooltip text describing proc + scaling + chain + safety cap.
- Re-run `dotnet test ...` and confirm Task 1 now passes.

---

### Task 5: Regenerate outputs + package MO2 zip

**Commands:**
- `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`
- `tools/build_mo2_zip.sh`

---

### Task 6: Verify

**Commands:**
- `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
- (If toolchain available) rebuild SKSE plugin in `skse/CalamityAffixes/build.linux-clangcl-rel` (or Windows/VS).

**In-game checklist (user-run):**
- Kill a hostile with an affix-equipped item; confirm:
  - LoreBox tooltip text is visible for the affix.
  - Explosion triggers on kill, damages nearby hostiles.
  - Chaining occurs but stops at the cap (no proc storm / no stutter / no CTD).
