# Adaptive Element + Target-Swap Procs Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** Add two “non-vanilla-enchant” hit-proc mechanics to the Calamity runtime: (1) adaptive element spell selection based on target resistances, and (2) per-target proc cooldowns to reward target swapping.

**Architecture:** Extend the SKSE runtime config parser to support new `runtime.perTargetICDSeconds` and a new action type `CastSpellAdaptiveElement`. At runtime, procs are still driven by existing triggers (`Hit`, etc.), but `ProcessTrigger` gains a per-target gating layer. The adaptive action chooses a spell (Fire/Frost/Shock) from config based on the target’s current resistances and casts it via `CastSpellImmediate`.

**Tech Stack:** C++ (SKSE + CommonLibSSE-NG), nlohmann::json runtime config, static-assert style tests in `skse/CalamityAffixes/tests`, .NET generator unchanged except for new `affixes/affixes.json` content.

---

### Task 1: Add adaptive selection utility + tests

**Files:**
- Create: `skse/CalamityAffixes/include/CalamityAffixes/AdaptiveElement.h`
- Test: `skse/CalamityAffixes/tests/test_adaptive_element.cpp`

**Step 1: Write failing tests (static_assert)**
- Add `PickAdaptiveElement(...)` returning `Element` for (fire,frost,shock) resist values.
- Add tests for:
  - picks lowest resist for `WeakestResist`
  - picks highest resist for `StrongestResist`
  - stable tie-breaking (Fire > Frost > Shock or explicit order)

**Step 2: Build SKSE tests target**
Run: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j 4 --target CalamityAffixesTests`
Expected: FAIL until implementation exists.

**Step 3: Implement minimal utility to pass**
- Keep it pure (no RE types), unit-testable via `static_assert`.

---

### Task 2: Add per-target proc cooldown support

**Files:**
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- Modify: `skse/CalamityAffixes/src/EventBridge.Config.cpp`
- Modify: `skse/CalamityAffixes/src/EventBridge.Triggers.cpp`

**Step 1: Extend `AffixRuntime`**
- Add `std::chrono::milliseconds perTargetIcd{0};`

**Step 2: Parse config**
- Read `runtime.perTargetICDSeconds` (float seconds) and store as ms.

**Step 3: Gate in `ProcessTrigger`**
- Before rolling chance, check per-target gate keyed by `(affix.token, targetFormID)`.
- If allowed and proc triggers, write next-allowed = `now + perTargetIcd`.
- Add pruning to prevent long-session growth (time-based prune when size exceeds threshold).

---

### Task 3: Add `CastSpellAdaptiveElement` runtime action

**Files:**
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`
- Modify: `skse/CalamityAffixes/src/EventBridge.Config.cpp`
- Modify: `skse/CalamityAffixes/src/EventBridge.Actions.cpp`

**Step 1: Extend enums and structs**
- Add `ActionType::kCastSpellAdaptiveElement`.
- Add `AdaptiveElementMode` (`WeakestResist`, `StrongestResist`).
- Add spell slots to `Action`:
  - `RE::SpellItem* adaptiveFire`, `adaptiveFrost`, `adaptiveShock`

**Step 2: Parse config**
Example JSON shape:
```json
{
  "type": "CastSpellAdaptiveElement",
  "mode": "StrongestResist",
  "spells": {
    "Fire": "CAFF_SPEL_FIRE_SHRED",
    "Frost": "CAFF_SPEL_FROST_SHRED",
    "Shock": "CAFF_SPEL_SHOCK_SHRED"
  },
  "applyTo": "Target"
}
```
- Parse each `spells.<Element>` using existing `parseSpellFromString` (supports EditorID or `Mod|0xFormID`).

**Step 3: Execute**
- Choose element with `PickAdaptiveElement`.
- Cast chosen spell using the same magnitude scaling logic used by `CastSpell` when configured.

---

### Task 4: Add example affixes (hit-based, not “vanilla enchant” clones)

**Files:**
- Modify: `affixes/affixes.json`

**Step 1: Adaptive Exposure (target analysis)**
- A weapon affix that procs on hit and applies *the correct exposure* (fire/frost/shock resist shred) by picking element based on target resistance.
- Use `runtime.perTargetICDSeconds` to make it feel like a “jackpot” per target and reward target swapping.

**Step 2: Replace `dot_bloom` TODO**
- Implement `DotApply` → `SpawnTrap` version (DoT apply creates a short-lived hazard), keeping strong ICD to avoid spam.

**Step 3: Regenerate Data outputs**
Run: `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`

---

### Task 5: Verify

**Step 1: Generator tests**
Run: `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
Expected: PASS

**Step 2: SKSE build**
Run: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j 4`
Expected: PASS

