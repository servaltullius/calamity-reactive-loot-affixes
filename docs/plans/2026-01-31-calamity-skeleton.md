# Calamity Skeleton Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task.

**Goal:** Create a working repo skeleton for “Calamity - Reactive Loot & Affixes” (Skyrim SE/AE) including a `Data/` staging tree, Papyrus templates, MCM Helper config templates, SPID/KID templates, and an SKSE (CommonLibSSE-NG) plugin project scaffold that can later be compiled into `Data/SKSE/Plugins/CalamityAffixes.dll`.

**Architecture:** Use a data-driven affix system (keywords on items) with a centralized Papyrus `AffixManager` for proc/ICD logic. Use an optional SKSE plugin event sink (`TESHitEvent` + `TESMagicEffectApplyEvent`) for “any weapon + magic + summons + DoT apply/refresh” and forward triggers to Papyrus via `ModCallbackEvent` (low data volume; heavy filtering/rate-limits).

**Tech Stack:** Skyrim SE/AE, SKSE64, CommonLibSSE-NG (CMake + vcpkg), Papyrus, MCM Helper, KID, SPID.

---

### Task 1: Create repo directory layout

**Files:**
- Create: `Data/`
- Create: `Data/MCM/Config/CalamityAffixes/`
- Create: `Data/Scripts/Source/`
- Create: `Data/SKSE/Plugins/` (staging only; no DLL committed)
- Create: `skse/CalamityAffixes/`

**Step 1: Create directories**

Run:
```bash
mkdir -p Data/MCM/Config/CalamityAffixes Data/Scripts/Source Data/SKSE/Plugins skse/CalamityAffixes
```

Expected: directories exist (no output).

**Step 2: Verify tree exists**

Run:
```bash
find Data -maxdepth 3 -type d | sort
```

Expected: shows `Data/MCM/Config/CalamityAffixes`, `Data/Scripts/Source`, `Data/SKSE/Plugins`, etc.

---

### Task 2: Add Papyrus + MCM Helper + SPID/KID templates into `Data/`

**Files:**
- Create: `Data/Scripts/Source/CalamityAffixes_AffixManager.psc`
- Create: `Data/Scripts/Source/CalamityAffixes_PlayerAlias.psc`
- Create: `Data/Scripts/Source/CalamityAffixes_AffixEffectBase.psc`
- Create: `Data/Scripts/Source/CalamityAffixes_SkseBridge.psc`
- Create: `Data/MCM/Config/CalamityAffixes/config.json`
- Create: `Data/MCM/Config/CalamityAffixes/settings.ini`
- Create: `Data/MCM/Config/CalamityAffixes/keybinds.json`
- Create: `Data/CalamityAffixes_KID.ini`
- Create: `Data/CalamityAffixes_DISTR.ini`

**Step 1: Copy templates from the custom skill**

Source templates live under:
- `~/.codex/skills/calamity-skyrim-affix-devkit/assets/`

Copy them into `Data/` preserving the intended Skyrim paths.

**Step 2: Add a thin Papyrus SKSE bridge script**

Implement `CalamityAffixes_SkseBridge.psc` to:
- `RegisterForModEvent` on init and on load
- On mod event(s), cast `sender` to `Actor` and call the manager’s hit/DoT entry points

**Step 3: Validate JSON**

Run:
```bash
python3 -m json.tool Data/MCM/Config/CalamityAffixes/config.json >/dev/null
python3 -m json.tool Data/MCM/Config/CalamityAffixes/keybinds.json >/dev/null
```

Expected: no output (exit code 0).

---

### Task 3: Add SKSE plugin scaffold (CommonLibSSE-NG)

**Files:**
- Create: `skse/CalamityAffixes/CMakeLists.txt`
- Create: `skse/CalamityAffixes/vcpkg.json`
- Create: `skse/CalamityAffixes/vcpkg-configuration.json`
- Create: `skse/CalamityAffixes/src/main.cpp`
- Create: `skse/CalamityAffixes/src/EventBridge.cpp`
- Create: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h`

**Step 1: Add CommonLibSSE-NG dependency**
- Use `vcpkg.json` + `vcpkg-configuration.json` to pull `commonlibsse-ng` from the ColorGlass vcpkg repo.

**Step 2: CMake target**
- Use `find_package(CommonLibSSE CONFIG REQUIRED)`
- Use `add_commonlibsse_plugin(...)` to generate the SKSE plugin metadata automatically.

**Step 3: Implement event sinks**
- `TESHitEvent`: filter for player-owned aggressor (player or commanding actor is player), then emit a low-bandwidth `ModCallbackEvent` for Papyrus.
- `TESMagicEffectApplyEvent`: same filter, then treat it as “DoT apply/refresh” only when the `magicEffect` is tagged `CAFF_TAG_DOT` (data-driven via KID).

**Step 4: Rate-limit in C++**
- Add a conservative per-target cooldown (1–2s) for DoT apply events.
- (Optional) Add a global cap to protect Papyrus in edge cases.

**Step 5: Non-compilation verification**

Run:
```bash
rg -n "TESHitEvent|TESMagicEffectApplyEvent|ModCallbackEvent" skse/CalamityAffixes -S
```

Expected: matches in `EventBridge.cpp` and/or headers.

---

### Task 4: Add repo README (how to build + how to test)

**Files:**
- Create: `README.md`

**Step 1: Document prerequisites**
- Visual Studio 2022 (Desktop development with C++)
- vcpkg configured (`VCPKG_ROOT`)
- Correct SKSE + Address Library for target runtime(s)
- Mod manager output folder env vars (optional)

**Step 2: Document the folder layout**
- `Data/` is the staged mod root for packaging / MO2 testing
- `skse/CalamityAffixes/` is the C++ plugin project

**Step 3: Basic “smoke test” checklist**
- Install `Data/` into MO2 as a mod
- Confirm MCM loads
- Confirm Papyrus scripts compile
- Confirm SKSE plugin logs on boot once compiled and installed
