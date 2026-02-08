# Zone Denial Bloom Variants + Chaos Mine + Swap Jackpot Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** Add three “hit-based ARPG-feeling” weapon affix candidates: (1) DoT-triggered zone denial bloom variants, (2) chaos mine trap with random curse outcome, and (3) target-swap “jackpot” buff using per-target ICD.

**Architecture:** This is primarily **data-driven** (`affixes/affixes.json`) using existing runtime actions: `SpawnTrap`, `CastSpell`, and the already-implemented `runtime.perTargetICDSeconds`. Two extra safety checks are added to the `DotApply` pipeline so broad KID tagging cannot turn into “any spell cast triggers blooms”.

**Tech Stack:** C# generator (Mutagen) → `Data/*` outputs, SKSE plugin (CommonLibSSE-NG) runtime reads `Data/SKSE/Plugins/CalamityAffixes/affixes.json`.

---

### Task 1: Define concrete designs (numbers + spells)

**Affixes:**
- `dot_bloom_tar`: DoT apply → delayed tar burst (slow + armor sunder) in an AoE.
- `dot_bloom_siphon`: DoT apply → delayed siphon burst (magicka/stamina regen suppression) in an AoE.
- `hit_chaos_mine`: On hit → plant mine; triggers once; snare + **random** curse (one of several debuffs).
- `hit_swap_jackpot`: On hit → per-target ICD makes target swapping pay off; grants short **self** weapon speed buff.

**Implementation constraints:**
- Generator spell records are currently single-effect; for 2-effect blooms, use `SpawnTrap.extraSpells` with a single fixed extra spell (casts base + extra).
- Keep effects “not a vanilla enchant clone”: focus on debuffs/utility rather than direct elemental bonus damage/absorb/paralyze/etc.

---

### Task 2: Update `affixes/affixes.json`

**Files:**
- Modify: `affixes/affixes.json`

**Steps:**
1. Add the 4 new affix entries with `runtime.trigger` and actions (`SpawnTrap` / `CastSpell`) and appropriate ICD/per-target ICD.
2. Add required generated records (`records.magicEffect`/`records.spell`).
3. Add INTERNAL spell entries for shared curse spells and any “fixed extra spell” used by blooms.

---

### Task 3: Harden DotApply filtering (safety)

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.Triggers.cpp`

**Steps:**
1. In `TESMagicEffectApplyEvent` handling, after verifying `CAFF_TAG_DOT`, require:
   - magic effect is hostile, and
   - magic effect is not flagged `NoDuration`
2. Keep existing per-(target,mgef) ICD guard.

---

### Task 4: Regenerate + verify

**Steps:**
1. Run generator:
   - `dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`
2. Run generator tests:
   - `dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release`
3. Build SKSE plugin (release):
   - `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j 4`
4. Copy DLL to staging:
   - `cp -f skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.dll Data/SKSE/Plugins/CalamityAffixes.dll`
5. Package MO2 zip:
   - `tools/build_mo2_zip.sh`

