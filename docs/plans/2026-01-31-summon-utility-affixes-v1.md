# Summon & Utility Affixes (v1) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add several gameplay-centric “Summon/Utility(3번)” affixes without CK by proccing **vanilla spells** via `spellForm`.

**Architecture:** Extend `affixes/affixes.json` with new weapon affixes. At runtime, the SKSE plugin procs on `Hit` / `IncomingHit` and executes `CastSpell` using vanilla `spellForm` (e.g. `Skyrim.esm|000640B6`). The generator produces `CalamityAffixes_Keywords.esp` keywords, KID distribution (`Data/CalamityAffixes_KID.ini`), and Inventory Injector tooltip text (from each affix `name`).

**Tech Stack:** .NET generator (Mutagen), SKSE (CommonLibSSE), KID, Inventory Injector.

---

### Task 1: Verify vanilla Spell FormIDs

**Step 1: Confirm Spell IDs from UESP**

Run:
`curl -L -A 'Mozilla/5.0' -s 'https://en.uesp.net/wiki/Skyrim:Conjuration_Spells' | rg -n 'Soul Trap' -m 1 -C 3`

Expected: includes `Spell ID ... 0004dba4`

Run:
`curl -L -A 'Mozilla/5.0' -s 'https://en.uesp.net/wiki/Skyrim:Illusion_Spells' | rg -n '<span id=\"Muffle\"' -m 1 -C 2`

Expected: includes `Spell ID ... 0008f3eb`

---

### Task 2: Add summon/utility affixes to spec

**Files:**
- Modify: `affixes/affixes.json`

**Step 1: Add new affix entries (no `records`)**

Add affixes using `runtime.action.type = "CastSpell"` and `spellForm`:
- Conjure Familiar: `Skyrim.esm|000640B6`
- Conjure Flame Atronach: `Skyrim.esm|000204C3`
- Conjure Frost Atronach: `Skyrim.esm|000204C4`
- Conjure Storm Atronach: `Skyrim.esm|000204C5`
- Conjure Dremora Lord: `Skyrim.esm|0010DDEC`
- Soul Trap: `Skyrim.esm|0004DBA4`
- Invisibility: `Skyrim.esm|00027EB6`
- Muffle: `Skyrim.esm|0008F3EB`

Keep proc rates low and use long ICDs for summons (PoE-style “저발동”).

---

### Task 3: Regenerate Data outputs

**Step 1: Run generator**

Run:
`dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data`

Expected: updates
- `Data/CalamityAffixes_Keywords.esp`
- `Data/CalamityAffixes_KID.ini`
- `Data/SKSE/Plugins/InventoryInjector/CalamityAffixes_Keywords.json`
- `Data/SKSE/Plugins/CalamityAffixes/affixes.json`

---

### Task 4: Run generator tests

Run:
`dotnet test tools/CalamityAffixes.Tools.sln`

Expected: PASS

---

### Task 5: Build MO2 zip

Run:
`tools/build_mo2_zip.sh`

Expected: `dist/CalamityAffixes_MO2_*.zip`

