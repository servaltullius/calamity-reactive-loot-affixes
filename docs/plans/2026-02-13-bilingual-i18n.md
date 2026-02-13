# Bilingual (EN/KO) i18n Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Every user-facing affix name supports English, Korean, and dual-language display, driven by the existing MCM `iUiLanguage` setting (0=en, 1=ko, 2=both).

**Architecture:** Add `nameEn`/`nameKo` fields to JSON data. C++ reads both, selects at tooltip-build time based on a language-mode parameter passed from PrismaTooltip. ESP spell names use bilingual "한국어 / English" format (game engine can't switch at runtime). No Prisma panel JS changes needed — C++ sends already-resolved text.

**Tech Stack:** C++23 (clang-cl cross-compile), Python 3 (data migration), C# .NET 8 (Generator/Mutagen)

---

## Task 1: Add `nameEn`/`nameKo` to source catalog JSON

**Files:**
- Create: `/tmp/add_bilingual_names.py`
- Modify: `affixes/affixes.json`

**Step 1: Write the migration script**

Create a Python script that:
1. Loads `affixes/affixes.json` via `json.load()`
2. For each affix in `keywords.affixes[]`:
   - **Suffixes** (60): Sets `nameKo` from current `name` (Korean), sets `nameEn` from a hard-coded EN map (original English names like `"of Minor Vitality: Max Health +25"`)
   - **Prefixes** (83): Sets `nameKo` from current `name` (Korean), sets `nameEn` from a hard-coded EN translation map
   - Keeps `name` field as-is for backward compatibility
3. Writes back with `json.dump(ensure_ascii=False, indent=2)`

The script contains two dictionaries:

**Suffix EN map** (60 entries) — restore original English names:
```python
suffix_en = {
    "suffix_max_health_t1": "of Minor Vitality: Max Health +25",
    "suffix_max_health_t2": "of Vitality: Max Health +50",
    "suffix_max_health_t3": "of Grand Vitality: Max Health +75",
    # ... all 60 suffix entries (20 families × 3 tiers)
}
```

**Prefix EN map** (83 entries) — English translations of Korean display names:
```python
prefix_en = {
    "arc_lightning": "Arc Lightning (100% on hit): 10 Lightning Damage (Hit Damage Scaling)",
    "hit_fire_damage": "Fire Damage (100% on hit): 6 Fire Damage (Hit Damage Scaling)",
    # ... all 83 prefix entries
}
```

The full maps are provided in the script. Entries with `INTERNAL:` prefix in their name are skipped (they're never shown in tooltips).

```python
import json, sys

filepath = sys.argv[1]
with open(filepath, 'r', encoding='utf-8') as f:
    data = json.load(f)

suffix_en = { ... }  # 60 entries keyed by affix id
prefix_en = { ... }  # 83 entries keyed by affix id

affixes = data["keywords"]["affixes"]
count = 0
for a in affixes:
    aid = a.get("id", "")
    if aid in suffix_en:
        a["nameEn"] = suffix_en[aid]
        a["nameKo"] = a["name"]
        count += 1
    elif aid in prefix_en:
        a["nameEn"] = prefix_en[aid]
        a["nameKo"] = a["name"]
        count += 1

with open(filepath, 'w', encoding='utf-8') as f:
    json.dump(data, f, ensure_ascii=False, indent=2)

print(f"{filepath}: {count} affixes updated with nameEn/nameKo")
```

**Step 2: Run the script on source catalog**

```bash
python3 /tmp/add_bilingual_names.py "affixes/affixes.json"
```
Expected: `affixes/affixes.json: 143 affixes updated with nameEn/nameKo`

**Step 3: Verify the result**

```bash
python3 -c "
import json
with open('affixes/affixes.json','r') as f: data=json.load(f)
affixes = data['keywords']['affixes']
has_en = sum(1 for a in affixes if 'nameEn' in a)
has_ko = sum(1 for a in affixes if 'nameKo' in a)
print(f'nameEn: {has_en}, nameKo: {has_ko}')
# Print a sample
s = next(a for a in affixes if a['id']=='suffix_max_health_t2')
print(f'  name={s[\"name\"]}')
print(f'  nameEn={s[\"nameEn\"]}')
print(f'  nameKo={s[\"nameKo\"]}')
p = next(a for a in affixes if a['id']=='arc_lightning')
print(f'  name={p[\"name\"]}')
print(f'  nameEn={p[\"nameEn\"]}')
print(f'  nameKo={p[\"nameKo\"]}')
"
```
Expected: 143 each, with correct EN/KO values.

**Step 4: Copy to runtime JSON**

```bash
python3 /tmp/add_bilingual_names.py "Data/SKSE/Plugins/CalamityAffixes/affixes.json"
```
Expected: same count.

**Step 5: Commit**

```bash
git add affixes/affixes.json "Data/SKSE/Plugins/CalamityAffixes/affixes.json"
git commit -m "feat(i18n): add nameEn/nameKo fields to all 143 affixes"
```

---

## Task 2: Add `displayNameEn`/`displayNameKo` to C++ AffixRuntime

**Files:**
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h:331-354`

**Step 1: Add fields to AffixRuntime struct**

In `EventBridge.h`, inside `struct AffixRuntime` (line 331-354), add two new string fields after `displayName`:

```cpp
struct AffixRuntime
{
    std::string id{};
    std::uint64_t token{ 0 };
    std::string keywordEditorId{};
    std::string label{};
    std::string displayName{};
    std::string displayNameEn{};   // NEW
    std::string displayNameKo{};   // NEW
    RE::BGSKeyword* keyword{ nullptr };
    // ... rest unchanged
};
```

Add after line 337 (`std::string displayName{};`):
```cpp
std::string displayNameEn{};
std::string displayNameKo{};
```

**Step 2: Add `uiLanguageMode` parameter to `GetInstanceAffixTooltip`**

In `EventBridge.h`, modify the declaration at line 112-114:

Before:
```cpp
[[nodiscard]] std::optional<std::string> GetInstanceAffixTooltip(
    const RE::InventoryEntryData* a_item,
    std::string_view a_selectedDisplayName = {}) const;
```

After:
```cpp
[[nodiscard]] std::optional<std::string> GetInstanceAffixTooltip(
    const RE::InventoryEntryData* a_item,
    std::string_view a_selectedDisplayName = {},
    int a_uiLanguageMode = 2) const;
```

**Step 3: Verify build compiles**

```bash
cd skse/CalamityAffixes && cmake --build build.linux-clangcl-rel --config Release 2>&1 | tail -5
```
Expected: Build succeeds (no callers use the new default-valued param yet).

**Step 4: Commit**

```bash
git add skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h
git commit -m "feat(i18n): add displayNameEn/Ko to AffixRuntime + lang param to tooltip API"
```

---

## Task 3: Parse `nameEn`/`nameKo` in Config.cpp

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.Config.cpp:383-390`

**Step 1: Add parsing after existing `name` field parsing**

At line 383-390 in `EventBridge.Config.cpp`, replace:

```cpp
out.displayName = a.value("name", std::string{});
if (out.displayName.empty()) {
    out.displayName = out.keywordEditorId;
}
out.label = DeriveAffixLabel(out.displayName);
if (out.label.empty()) {
    out.label = out.displayName;
}
```

With:

```cpp
out.displayName = a.value("name", std::string{});
if (out.displayName.empty()) {
    out.displayName = out.keywordEditorId;
}
out.displayNameEn = a.value("nameEn", std::string{});
out.displayNameKo = a.value("nameKo", std::string{});
// Fall back: if only one language is present, use displayName for the other
if (out.displayNameEn.empty()) {
    out.displayNameEn = out.displayName;
}
if (out.displayNameKo.empty()) {
    out.displayNameKo = out.displayName;
}
out.label = DeriveAffixLabel(out.displayName);
if (out.label.empty()) {
    out.label = out.displayName;
}
```

**Step 2: Verify build compiles**

```bash
cd skse/CalamityAffixes && cmake --build build.linux-clangcl-rel --config Release 2>&1 | tail -5
```
Expected: Build succeeds.

**Step 3: Commit**

```bash
git add skse/CalamityAffixes/src/EventBridge.Config.cpp
git commit -m "feat(i18n): parse nameEn/nameKo from affixes.json"
```

---

## Task 4: Language-aware tooltip formatting in Runtime.cpp

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.Loot.Runtime.cpp:157-159` (function signature)
- Modify: `skse/CalamityAffixes/src/EventBridge.Loot.Runtime.cpp:280-318` (formatTooltip lambda)

**Step 1: Update function signature**

At line 157-159, change:

```cpp
std::optional<std::string> EventBridge::GetInstanceAffixTooltip(
    const RE::InventoryEntryData* a_item,
    std::string_view a_selectedDisplayName) const
```

To:

```cpp
std::optional<std::string> EventBridge::GetInstanceAffixTooltip(
    const RE::InventoryEntryData* a_item,
    std::string_view a_selectedDisplayName,
    int a_uiLanguageMode) const
```

**Step 2: Add local helper to resolve display name by language**

Insert before the `formatTooltip` lambda (before line 280):

```cpp
auto resolveDisplayName = [a_uiLanguageMode](const AffixRuntime& a_affix) -> std::string_view {
    switch (a_uiLanguageMode) {
    case 0:  // English
        return a_affix.displayNameEn;
    case 1:  // Korean
        return a_affix.displayNameKo;
    case 2:  // Both
    default: {
        // "both" mode needs concatenation, handled below
        return {};
    }
    }
};
```

**Step 3: Replace `affix.displayName` with language-resolved name**

In `formatTooltip` lambda, at line 296-298 and line 318, replace:

Before (line 296-298):
```cpp
if (affix.displayName.empty()) {
    continue;
}
```

After:
```cpp
const auto resolvedName = resolveDisplayName(affix);
std::string bothName;
if (a_uiLanguageMode == 2) {
    // "both" mode: "Korean / English" (skip if identical)
    if (affix.displayNameKo == affix.displayNameEn || affix.displayNameEn.empty()) {
        bothName = affix.displayNameKo;
    } else if (affix.displayNameKo.empty()) {
        bothName = std::string(affix.displayNameEn);
    } else {
        bothName = affix.displayNameKo;
        bothName.append(" / ");
        bothName.append(affix.displayNameEn);
    }
}
const std::string_view displayForThisAffix =
    a_uiLanguageMode == 2 ? std::string_view(bothName) : resolvedName;
if (displayForThisAffix.empty()) {
    continue;
}
```

Before (line 318):
```cpp
tooltip.append(affix.displayName);
```

After:
```cpp
tooltip.append(displayForThisAffix);
```

**Step 4: Verify build compiles**

```bash
cd skse/CalamityAffixes && cmake --build build.linux-clangcl-rel --config Release 2>&1 | tail -5
```
Expected: Build succeeds.

**Step 5: Commit**

```bash
git add skse/CalamityAffixes/src/EventBridge.Loot.Runtime.cpp
git commit -m "feat(i18n): language-aware affix tooltip formatting"
```

---

## Task 5: Pass language mode from PrismaTooltip.cpp

**Files:**
- Modify: `skse/CalamityAffixes/src/PrismaTooltip.cpp:1164`

**Step 1: Update the single call site**

At line 1164 in PrismaTooltip.cpp, change:

```cpp
result.tooltip = bridge->GetInstanceAffixTooltip(item->data.objDesc, result.itemName);
```

To:

```cpp
result.tooltip = bridge->GetInstanceAffixTooltip(item->data.objDesc, result.itemName, g_uiLanguageMode.load());
```

This passes the current MCM language setting (0=en, 1=ko, 2=both) from the PrismaTooltip module's global atomic to the EventBridge tooltip builder.

**Step 2: Verify build compiles and run tests**

```bash
cd skse/CalamityAffixes && cmake --build build.linux-clangcl-rel --config Release 2>&1 | tail -5
cd build.linux-clangcl-rel && ctest --output-on-failure
```
Expected: Build succeeds, all existing tests pass.

**Step 3: Commit**

```bash
git add skse/CalamityAffixes/src/PrismaTooltip.cpp
git commit -m "feat(i18n): pass MCM language mode to tooltip builder"
```

---

## Task 6: Update Generator for bilingual ESP spell names

**Files:**
- Modify: `tools/CalamityAffixes.Generator/Spec/AffixSpec.cs:82-107` (AffixDefinition)
- Modify: `tools/CalamityAffixes.Generator/Writers/KeywordPluginBuilder.cs:122-124,203-206`

**Step 1: Add `NameEn`/`NameKo` to AffixDefinition**

In `AffixSpec.cs`, add to `AffixDefinition` class (after line 91):

```csharp
[JsonPropertyName("nameEn")]
public string? NameEn { get; init; }

[JsonPropertyName("nameKo")]
public string? NameKo { get; init; }
```

**Step 2: Update KeywordPluginBuilder to use bilingual ESP record names**

In `KeywordPluginBuilder.cs`, after line 122-124 where `mgef.Name` is set:

```csharp
if (!string.IsNullOrWhiteSpace(spec.Name))
{
    mgef.Name = spec.Name;
}
```

This doesn't change — ESP MagicEffect/Spell names come from the `records.magicEffect.name` and `records.spell.name` fields in the JSON, not from the top-level `nameEn`/`nameKo`. The ESP record names are separate from tooltip display names.

For ESP record names, we update them in the source JSON via a separate migration script (Task 7).

**Step 3: Run Generator tests**

```bash
cd tools && dotnet test CalamityAffixes.Generator.Tests
```
Expected: All tests pass (adding optional properties is backward-compatible).

**Step 4: Commit**

```bash
git add tools/CalamityAffixes.Generator/Spec/AffixSpec.cs
git commit -m "feat(i18n): add NameEn/NameKo to Generator AffixDefinition"
```

---

## Task 7: Bilingual ESP record names in source JSON

**Files:**
- Create: `/tmp/bilingual_esp_names.py`
- Modify: `affixes/affixes.json` (records.magicEffect.name, records.spell.name)

**Step 1: Write script to create bilingual ESP record names**

For suffix ESP records, combine Korean and English:
- Current: `"Calamity 서픽스: 최대 체력 +25"` (Korean)
- Target: `"Calamity 서픽스: 최대 체력 +25 / Suffix: Max Health +25"`

For prefix ESP records, they're already English ("Calamity: Arc Lightning") — add Korean:
- Current: `"Calamity: Arc Lightning"` (English)
- Target: `"Calamity: Arc Lightning / 아크 번개"`

```python
import json, sys

filepath = sys.argv[1]
with open(filepath, 'r', encoding='utf-8') as f:
    data = json.load(f)

# Maps: affix_id -> short bilingual ESP record name
suffix_esp_bilingual = {
    "suffix_max_health_t1": "Calamity: Max Health +25 / 최대 체력 +25",
    # ... all 60 suffix ESP entries
}

prefix_esp_bilingual = {
    "arc_lightning": "Calamity: Arc Lightning / 아크 번개",
    # ... all prefix ESP entries that have records
}

affixes = data["keywords"]["affixes"]
count = 0
for a in affixes:
    aid = a.get("id", "")
    records = a.get("records")
    if not records:
        continue
    bilingual = suffix_esp_bilingual.get(aid) or prefix_esp_bilingual.get(aid)
    if not bilingual:
        continue
    me = records.get("magicEffect")
    if me and me.get("name"):
        me["name"] = bilingual
        count += 1
    sp = records.get("spell")
    if sp and sp.get("name"):
        sp["name"] = bilingual
        count += 1

with open(filepath, 'w', encoding='utf-8') as f:
    json.dump(data, f, ensure_ascii=False, indent=2)

print(f"{filepath}: {count} ESP record names updated to bilingual")
```

**Step 2: Run the script**

```bash
python3 /tmp/bilingual_esp_names.py "affixes/affixes.json"
```
Expected: ~180 ESP record names updated.

**Step 3: Regenerate ESP**

```bash
cd tools && dotnet run --project CalamityAffixes.Generator -- \
  --spec "../affixes/affixes.json" \
  --data "../Data"
```
Expected: `Data/CalamityAffixes.esp` regenerated with bilingual spell names.

**Step 4: Copy bilingual names to runtime JSON too**

```bash
python3 /tmp/bilingual_esp_names.py "Data/SKSE/Plugins/CalamityAffixes/affixes.json"
```

**Step 5: Commit**

```bash
git add affixes/affixes.json "Data/SKSE/Plugins/CalamityAffixes/affixes.json" Data/CalamityAffixes.esp
git commit -m "feat(i18n): bilingual ESP spell names + regenerate ESP"
```

---

## Task 8: Full build + test + verify

**Files:** (none — verification only)

**Step 1: C++ build**

```bash
cd skse/CalamityAffixes && cmake --build build.linux-clangcl-rel --config Release 2>&1 | tail -10
```
Expected: BUILD_ALL succeeded.

**Step 2: C++ tests**

```bash
cd skse/CalamityAffixes/build.linux-clangcl-rel && ctest --output-on-failure
```
Expected: All tests pass.

**Step 3: Generator tests**

```bash
cd tools && dotnet test CalamityAffixes.Generator.Tests
```
Expected: All tests pass.

**Step 4: Validate JSON integrity**

```bash
python3 -c "
import json
for path in ['affixes/affixes.json', 'Data/SKSE/Plugins/CalamityAffixes/affixes.json']:
    with open(path) as f:
        data = json.load(f)
    affixes = data['keywords']['affixes']
    has_en = sum(1 for a in affixes if 'nameEn' in a and a['nameEn'])
    has_ko = sum(1 for a in affixes if 'nameKo' in a and a['nameKo'])
    total = len(affixes)
    print(f'{path}: total={total}, nameEn={has_en}, nameKo={has_ko}')
    assert has_en >= 140, f'Expected >=140 nameEn, got {has_en}'
    assert has_ko >= 140, f'Expected >=140 nameKo, got {has_ko}'
print('OK')
"
```
Expected: `OK` (≥140 each, some INTERNAL entries may be skipped).

**Step 5: Commit if any remaining changes**

```bash
git status
```

---

## Task 9: Rebuild MO2 zip

**Files:**
- Create: `CalamityAffixes_MO2_v1.2.3.zip`

**Step 1: Package the MO2 zip**

```bash
cd /tmp && rm -rf CalamityAffixes && mkdir -p CalamityAffixes/SKSE/Plugins/CalamityAffixes
mkdir -p CalamityAffixes/SKSE/Plugins/InventoryInjector
mkdir -p CalamityAffixes/MCM/Config/CalamityAffixes
mkdir -p CalamityAffixes/PrismaUI/views/CalamityAffixes
mkdir -p CalamityAffixes/Scripts/Source

PROJECT="/home/kdw73/projects/Calamity - Reactive Loot & Affixes"

cp "$PROJECT/Data/CalamityAffixes.esp" CalamityAffixes/
cp "$PROJECT/Data/CalamityAffixes_DISTR.ini" CalamityAffixes/
cp "$PROJECT/Data/CalamityAffixes_KID.ini" CalamityAffixes/
cp "$PROJECT/skse/CalamityAffixes/build.linux-clangcl-rel/Release/CalamityAffixes.dll" CalamityAffixes/SKSE/Plugins/
cp "$PROJECT/Data/SKSE/Plugins/CalamityAffixes/affixes.json" CalamityAffixes/SKSE/Plugins/CalamityAffixes/
cp "$PROJECT/Data/SKSE/Plugins/InventoryInjector/CalamityAffixes.json" CalamityAffixes/SKSE/Plugins/InventoryInjector/
cp "$PROJECT/Data/MCM/Config/CalamityAffixes/"* CalamityAffixes/MCM/Config/CalamityAffixes/
cp "$PROJECT/Data/PrismaUI/views/CalamityAffixes/index.html" CalamityAffixes/PrismaUI/views/CalamityAffixes/
cp "$PROJECT/Data/Scripts/"*.pex CalamityAffixes/Scripts/ 2>/dev/null || true
cp "$PROJECT/Data/Scripts/Source/"*.psc CalamityAffixes/Scripts/Source/ 2>/dev/null || true

cd /tmp && zip -r "$PROJECT/CalamityAffixes_MO2_v1.2.3.zip" CalamityAffixes/
```

**Step 2: Verify zip contents**

```bash
unzip -l "$PROJECT/CalamityAffixes_MO2_v1.2.3.zip" | head -30
```
Expected: All critical files present including .dll, .esp, affixes.json.

**Step 3: Commit zip**

```bash
git add CalamityAffixes_MO2_v1.2.3.zip
git commit -m "release: v1.2.3 MO2 zip with bilingual i18n support"
```

---

## Summary of changes by file

| File | Change |
|------|--------|
| `affixes/affixes.json` | +`nameEn`, +`nameKo` fields on 143 entries; bilingual ESP record names |
| `Data/SKSE/Plugins/CalamityAffixes/affixes.json` | Same as above (runtime copy) |
| `Data/CalamityAffixes.esp` | Regenerated with bilingual spell names |
| `EventBridge.h:337-338` | +`displayNameEn`, +`displayNameKo` in AffixRuntime |
| `EventBridge.h:112-114` | +`int a_uiLanguageMode = 2` param |
| `EventBridge.Config.cpp:383-390` | Parse `nameEn`/`nameKo` from JSON |
| `EventBridge.Loot.Runtime.cpp:157-159` | Updated function signature |
| `EventBridge.Loot.Runtime.cpp:280-318` | Language-aware name resolution in `formatTooltip` |
| `PrismaTooltip.cpp:1164` | Pass `g_uiLanguageMode.load()` to tooltip API |
| `AffixSpec.cs:91` | +`NameEn`, +`NameKo` optional properties |
