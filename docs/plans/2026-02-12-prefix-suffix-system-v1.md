# Prefix/Suffix System Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** D2-style Prefix/Suffix 분류를 도입하여 다중 어픽스 아이템의 체감 다양성을 극대화한다. 기존 26개 proc 어픽스는 Prefix, 신규 20개 패시브 스탯은 Suffix로 분류하고 롤링 규칙으로 P+S 조합을 강제한다.

**Architecture:** 기존 `AffixRuntime`에 `slot` (prefix/suffix) 및 `family` 필드를 추가한다. Suffix는 Ability-type 스펠을 장비 시 AddSpell / 해제 시 RemoveSpell로 적용한다. 롤링은 P/S 풀을 분리하여 2어픽스=1P+1S, 3어픽스=2P+1S or 1P+2S로 조합한다. 각 suffix는 3티어(Minor/Major/Grand)로 별도 affix 엔트리이며, 티어 가중치가 chance에 반영된다.

**Tech Stack:** C++23, CommonLibSSE-NG, nlohmann/json, SKSE64

---

## Phase 1: AffixRuntime 스키마 확장

### Task 1: AffixSlot enum 및 AffixRuntime 필드 추가

**Files:**
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h:173-204` (enum 영역)
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h:324-340` (AffixRuntime 구조체)

**Step 1: EventBridge.h에 AffixSlot enum 추가**

`LootItemType` enum 뒤 (line 204 이후)에 추가:

```cpp
enum class AffixSlot : std::uint8_t
{
	kNone,      // runeword-only or legacy
	kPrefix,    // active proc effects
	kSuffix,    // passive stat bonuses
};
```

**Step 2: AffixRuntime에 slot, family, passiveSpell 필드 추가**

`AffixRuntime` 구조체 (line 324-340) 끝에 추가:

```cpp
struct AffixRuntime
{
	// ... existing fields ...
	std::chrono::steady_clock::time_point nextAllowed{};

	// Prefix/Suffix classification
	AffixSlot slot{ AffixSlot::kNone };
	std::string family{};            // suffix family grouping (e.g., "max_health")
	RE::SpellItem* passiveSpell{ nullptr };  // suffix: Ability spell to add/remove on equip
};
```

**Step 3: 빌드 확인**

Run: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --config Release 2>&1 | tail -5`
Expected: BUILD SUCCESS (필드만 추가, 사용처 없음)

**Step 4: Commit**

```bash
git add include/CalamityAffixes/EventBridge.h
git commit -m "feat(prefix-suffix): add AffixSlot enum and slot/family/passiveSpell fields to AffixRuntime"
```

---

### Task 2: suffix 풀 벡터 및 passive 추적 멤버 추가

**Files:**
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h:455-458` (멤버 영역)

**Step 1: 멤버 필드 추가**

`_lootArmorAffixes` (line 456) 뒤에 추가:

```cpp
std::vector<std::size_t> _lootWeaponSuffixes;
std::vector<std::size_t> _lootArmorSuffixes;
std::unordered_set<RE::SpellItem*> _appliedPassiveSpells;  // currently active suffix spells
```

**Step 2: 빌드 확인**

Run: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --config Release 2>&1 | tail -5`
Expected: BUILD SUCCESS

**Step 3: Commit**

```bash
git add include/CalamityAffixes/EventBridge.h
git commit -m "feat(prefix-suffix): add suffix pool vectors and passive spell tracking member"
```

---

## Phase 2: JSON 파서 확장

### Task 3: slot, family, passiveSpell 필드 파싱

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.Config.cpp:356-423` (affix 파싱 루프)

**Step 1: parseAffixSlot 람다 추가**

`parseTrapSpawnAt` 람다 (line 317-322) 뒤에 추가:

```cpp
auto parseAffixSlot = [](std::string_view a_slot) -> AffixSlot {
	if (a_slot == "Prefix" || a_slot == "prefix") {
		return AffixSlot::kPrefix;
	}
	if (a_slot == "Suffix" || a_slot == "suffix") {
		return AffixSlot::kSuffix;
	}
	return AffixSlot::kNone;
};
```

**Step 2: affix 파싱 루프에서 slot, family 읽기**

`out.keyword` 검증 뒤 (line 382 뒤), `kid` 파싱 전에 추가:

```cpp
out.slot = parseAffixSlot(a.value("slot", std::string{}));
out.family = a.value("family", std::string{});
```

**Step 3: suffix용 passiveSpell 파싱**

`runtime` 오브젝트 파싱 영역에서 — suffix인 경우 trigger 검증을 건너뛰고 passiveSpell 파싱:

기존 trigger 검증 코드 (line 401-423) 앞에 suffix 분기 추가:

```cpp
if (out.slot == AffixSlot::kSuffix) {
	// Suffixes don't use trigger/proc — they apply passive spells on equip.
	out.trigger = Trigger::kHit;  // placeholder, unused
	out.procChancePct = 0.0f;

	const auto& action = runtime.value("action", nlohmann::json::object());
	const auto passiveSpellId = action.value("passiveSpellEditorId", std::string{});
	if (!passiveSpellId.empty()) {
		out.passiveSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>(passiveSpellId);
		if (!out.passiveSpell) {
			SKSE::log::error(
				"CalamityAffixes: suffix passive spell not found (affixId={}, spellEditorId={}).",
				out.id, passiveSpellId);
		}
	}

	// Use kid.chance as loot weight for suffixes (same mechanic as prefixes)
	out.procChancePct = static_cast<float>(kid.value("chance", 1.0));
} else {
	// existing trigger/procChance parsing continues here...
```

기존 trigger 파싱 코드 (`const auto triggerStr = runtime.value(...)` 부터)를 else 블록 안으로 이동.

**Step 4: suffix 풀 등록**

`registerRuntimeAffix` 람다 (line 798-854) 안의 loot pool 등록 영역에 suffix 풀 추가.

기존 코드 (line 847-853):
```cpp
if (affix.lootType) {
	if (*affix.lootType == LootItemType::kWeapon) {
		_lootWeaponAffixes.push_back(idx);
	} else if (*affix.lootType == LootItemType::kArmor) {
		_lootArmorAffixes.push_back(idx);
	}
}
```

변경:
```cpp
if (affix.lootType) {
	if (affix.slot == AffixSlot::kSuffix) {
		if (*affix.lootType == LootItemType::kWeapon) {
			_lootWeaponSuffixes.push_back(idx);
		} else if (*affix.lootType == LootItemType::kArmor) {
			_lootArmorSuffixes.push_back(idx);
		}
	} else {
		if (*affix.lootType == LootItemType::kWeapon) {
			_lootWeaponAffixes.push_back(idx);
		} else if (*affix.lootType == LootItemType::kArmor) {
			_lootArmorAffixes.push_back(idx);
		}
	}
}
```

동일 변경을 메인 루프의 풀 등록 (line 789-795)에도 적용.

**Step 5: suffix 풀 clear를 LoadConfig 상단에 추가**

line 150-151 (`_lootWeaponAffixes.clear(); _lootArmorAffixes.clear();`) 뒤에:
```cpp
_lootWeaponSuffixes.clear();
_lootArmorSuffixes.clear();
_appliedPassiveSpells.clear();
```

**Step 6: 로그에 suffix 풀 크기 추가**

line 1948-1955의 로그 메시지에 suffix 풀 크기 추가:
```cpp
SKSE::log::info(
	"CalamityAffixes: runtime config loaded (affixes={}, prefixWeapon={}, prefixArmor={}, suffixWeapon={}, suffixArmor={}, lootChance={}%).",
	_affixes.size(),
	_lootWeaponAffixes.size(), _lootArmorAffixes.size(),
	_lootWeaponSuffixes.size(), _lootArmorSuffixes.size(),
	_loot.chancePercent);
```

**Step 7: 빌드 확인**

Run: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --config Release 2>&1 | tail -20`
Expected: BUILD SUCCESS

**Step 8: Commit**

```bash
git add src/EventBridge.Config.cpp
git commit -m "feat(prefix-suffix): parse slot/family/passiveSpell fields, build separate suffix pools"
```

---

## Phase 3: 롤링 로직 변경

### Task 4: RollSuffixIndex 함수 추가

**Files:**
- Modify: `skse/CalamityAffixes/include/CalamityAffixes/EventBridge.h` (선언 추가)
- Modify: `skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp:54-112` (구현)

**Step 1: 헤더에 선언 추가**

`RollLootAffixIndex` 선언 근처에 추가:

```cpp
[[nodiscard]] std::optional<std::size_t> RollSuffixIndex(
	LootItemType a_itemType,
	const std::vector<std::string>* a_excludeFamilies = nullptr);
```

**Step 2: RollSuffixIndex 구현**

`EventBridge.Loot.Assign.cpp`에서 `RollLootAffixIndex` 함수 뒤 (line 112 이후)에 추가:

```cpp
std::optional<std::size_t> EventBridge::RollSuffixIndex(
	LootItemType a_itemType,
	const std::vector<std::string>* a_excludeFamilies)
{
	std::vector<std::size_t> eligible;
	std::vector<double> weights;

	auto collectFromPool = [&](const std::vector<std::size_t>& a_pool) {
		for (const auto idx : a_pool) {
			if (idx >= _affixes.size()) {
				continue;
			}
			const auto& affix = _affixes[idx];
			if (affix.slot != AffixSlot::kSuffix) {
				continue;
			}
			// Exclude already-chosen suffix families
			if (a_excludeFamilies && !affix.family.empty()) {
				if (std::find(a_excludeFamilies->begin(), a_excludeFamilies->end(), affix.family) != a_excludeFamilies->end()) {
					continue;
				}
			}
			// Use procChancePct as loot weight (set from kid.chance during config parsing)
			const float weight = affix.procChancePct;
			if (weight <= 0.0f) {
				continue;
			}
			eligible.push_back(idx);
			weights.push_back(static_cast<double>(weight));
		}
	};

	if (_loot.sharedPool) {
		collectFromPool(_lootWeaponSuffixes);
		collectFromPool(_lootArmorSuffixes);
	} else {
		const auto& typePool = (a_itemType == LootItemType::kWeapon) ? _lootWeaponSuffixes : _lootArmorSuffixes;
		collectFromPool(typePool);
	}

	if (eligible.empty()) {
		return std::nullopt;
	}

	if (eligible.size() == 1) {
		return eligible[0];
	}

	std::discrete_distribution<std::size_t> dist(weights.begin(), weights.end());
	return eligible[dist(_rng)];
}
```

**Step 3: 빌드 확인**

Run: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --config Release 2>&1 | tail -5`
Expected: BUILD SUCCESS

**Step 4: Commit**

```bash
git add include/CalamityAffixes/EventBridge.h src/EventBridge.Loot.Assign.cpp
git commit -m "feat(prefix-suffix): add RollSuffixIndex with family-based dedup"
```

---

### Task 5: ApplyMultipleAffixes에 P/S 슬롯 분배 로직 적용

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp:124-210` (ApplyMultipleAffixes)

**Step 1: P/S 분배 로직으로 교체**

`ApplyMultipleAffixes`의 롤링 루프 (line 156-185)를 다음으로 교체:

```cpp
// Roll how many affixes this item gets (1-3)
const std::uint8_t targetCount = RollAffixCount();

// Determine prefix/suffix composition
std::uint8_t prefixTarget = 0;
std::uint8_t suffixTarget = 0;

if (targetCount == 1) {
	// 50/50 prefix or suffix
	std::uniform_int_distribution<int> coinFlip(0, 1);
	if (coinFlip(_rng) == 0) {
		prefixTarget = 1;
	} else {
		suffixTarget = 1;
	}
} else if (targetCount == 2) {
	// Always 1P + 1S
	prefixTarget = 1;
	suffixTarget = 1;
} else {
	// 3 affixes: 50% chance 2P+1S or 1P+2S
	std::uniform_int_distribution<int> coinFlip(0, 1);
	if (coinFlip(_rng) == 0) {
		prefixTarget = 2;
		suffixTarget = 1;
	} else {
		prefixTarget = 1;
		suffixTarget = 2;
	}
}

InstanceAffixSlots slots;
std::vector<std::size_t> chosenIndices;
chosenIndices.reserve(targetCount);

// Roll prefixes
std::vector<std::size_t> chosenPrefixIndices;
for (std::uint8_t p = 0; p < prefixTarget; ++p) {
	static constexpr std::uint8_t kMaxRetries = 3;
	bool found = false;
	for (std::uint8_t retry = 0; retry < kMaxRetries; ++retry) {
		const auto idx = RollLootAffixIndex(a_itemType, &chosenPrefixIndices, /*a_skipChanceCheck=*/p > 0 || !chosenIndices.empty());
		if (!idx) {
			break;
		}
		if (std::find(chosenPrefixIndices.begin(), chosenPrefixIndices.end(), *idx) != chosenPrefixIndices.end()) {
			continue;
		}
		if (*idx >= _affixes.size() || _affixes[*idx].id.empty()) {
			continue;
		}
		chosenPrefixIndices.push_back(*idx);
		chosenIndices.push_back(*idx);
		slots.AddToken(_affixes[*idx].token);
		found = true;
		break;
	}
	if (!found) {
		break;
	}
}

// Roll suffixes
std::vector<std::string> chosenFamilies;
for (std::uint8_t s = 0; s < suffixTarget; ++s) {
	const auto idx = RollSuffixIndex(a_itemType, &chosenFamilies);
	if (!idx) {
		break;
	}
	if (*idx >= _affixes.size() || _affixes[*idx].id.empty()) {
		continue;
	}
	if (!_affixes[*idx].family.empty()) {
		chosenFamilies.push_back(_affixes[*idx].family);
	}
	chosenIndices.push_back(*idx);
	slots.AddToken(_affixes[*idx].token);
}
```

나머지 코드 (line 187-210: slots.count == 0 체크, _instanceAffixes.emplace, 로그, display name)는 그대로 유지. 단, 로그 인덱싱을 `chosenIndices`로 변경.

**Step 2: chance 체크를 첫 번째 prefix 롤에만 적용하도록 확인**

기존 `RollLootAffixIndex`의 `a_skipChanceCheck` 로직은 유지. 첫 번째 prefix가 아이템 전체의 chance gate 역할.

**Step 3: suffix-only 아이템 (suffixTarget > 0, prefixTarget == 0)의 경우**

chance 체크를 suffix 롤 전에도 수행해야 함. `prefixTarget == 0`인 경우:

```cpp
// If no prefixes targeted, apply chance check before suffix rolling
if (prefixTarget == 0 && suffixTarget > 0) {
	if (_loot.chancePercent <= 0.0f) {
		return;
	}
	if (_loot.chancePercent < 100.0f) {
		std::uniform_real_distribution<float> dist(0.0f, 100.0f);
		if (dist(_rng) >= _loot.chancePercent) {
			return;
		}
	}
}
```

이 코드를 P/S 분배 결정 직후, 실제 롤링 전에 배치.

**Step 4: 빌드 확인**

Run: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --config Release 2>&1 | tail -5`
Expected: BUILD SUCCESS

**Step 5: Commit**

```bash
git add src/EventBridge.Loot.Assign.cpp
git commit -m "feat(prefix-suffix): P/S slot distribution in ApplyMultipleAffixes"
```

---

## Phase 4: Suffix 패시브 스펠 적용/해제

### Task 6: RebuildActiveCounts에서 패시브 스펠 관리

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.Triggers.Events.cpp:579-672` (RebuildActiveCounts)

**Step 1: 패시브 스펠 관리 로직 추가**

`RebuildActiveCounts` 함수의 장착 루프 안에서 suffix 패시브 스펠을 수집하고, 루프 후 diff를 적용.

기존 `_activeCounts.assign()` 뒤 (line 586 이후)에:

```cpp
std::unordered_set<RE::SpellItem*> desiredPassives;
```

기존 worn 체크 뒤의 slot 순회 (line 639-652) 안에 suffix 감지 추가:

```cpp
// Count each unique affix token as active, track best slot penalty
for (std::uint8_t slot = 0; slot < slots.count; ++slot) {
	const auto idxIt = _affixIndexByToken.find(slots.tokens[slot]);
	if (idxIt == _affixIndexByToken.end()) {
		continue;
	}
	const auto affixIdx = idxIt->second;

	if (affixIdx < _activeCounts.size()) {
		_activeCounts[affixIdx] += 1;
	}

	// Collect passive spells for equipped suffixes
	if (affixIdx < _affixes.size() && _affixes[affixIdx].slot == AffixSlot::kSuffix && _affixes[affixIdx].passiveSpell) {
		desiredPassives.insert(_affixes[affixIdx].passiveSpell);
	}

	// "Best Slot Wins" penalty (only for prefixes — suffixes don't proc)
	if (affixIdx < _affixes.size() && _affixes[affixIdx].slot != AffixSlot::kSuffix) {
		const float penalty = kMultiAffixProcPenalty[std::min<std::uint8_t>(slots.count, static_cast<std::uint8_t>(kMaxAffixesPerItem)) - 1];
		if (affixIdx < _activeSlotPenalty.size()) {
			_activeSlotPenalty[affixIdx] = std::max(_activeSlotPenalty[affixIdx], penalty);
		}
	}
}
```

**Step 2: 디버그 로그 뒤 (함수 끝, line 671 이전)에 diff 적용:**

```cpp
// Apply/remove passive suffix spells
auto* player = RE::PlayerCharacter::GetSingleton();
if (player) {
	// Remove spells no longer needed
	for (auto it = _appliedPassiveSpells.begin(); it != _appliedPassiveSpells.end(); ) {
		if (desiredPassives.find(*it) == desiredPassives.end()) {
			player->RemoveSpell(*it);
			SKSE::log::debug("CalamityAffixes: removed passive suffix spell {:08X}", (*it)->GetFormID());
			it = _appliedPassiveSpells.erase(it);
		} else {
			++it;
		}
	}
	// Add new spells
	for (auto* spell : desiredPassives) {
		if (_appliedPassiveSpells.find(spell) == _appliedPassiveSpells.end()) {
			player->AddSpell(spell);
			_appliedPassiveSpells.insert(spell);
			SKSE::log::debug("CalamityAffixes: applied passive suffix spell {:08X}", spell->GetFormID());
		}
	}
}
```

**Step 3: Revert에 _appliedPassiveSpells.clear() 추가**

`EventBridge.Serialization.cpp`의 `Revert()` 함수에서 기존 clear 라인 뒤에:
```cpp
_appliedPassiveSpells.clear();
```

**Step 4: 빌드 확인**

Run: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --config Release 2>&1 | tail -5`
Expected: BUILD SUCCESS

**Step 5: Commit**

```bash
git add src/EventBridge.Triggers.Events.cpp src/EventBridge.Serialization.cpp
git commit -m "feat(prefix-suffix): apply/remove passive suffix spells on equip change"
```

---

## Phase 5: 네이밍 및 툴팁 업데이트

### Task 7: EnsureMultiAffixDisplayName suffix 지원

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.Loot.Assign.cpp:212-345` (EnsureMultiAffixDisplayName)

**Step 1: primary affix 선택을 prefix 우선으로 변경**

기존 코드 (line 226-234):
```cpp
const auto primaryToken = a_slots.GetPrimary();
```

변경:
```cpp
// Use first prefix as primary label; fall back to first suffix if no prefix
std::uint64_t primaryToken = 0u;
for (std::uint8_t s = 0; s < a_slots.count; ++s) {
	const auto idxIt = _affixIndexByToken.find(a_slots.tokens[s]);
	if (idxIt != _affixIndexByToken.end() && idxIt->second < _affixes.size()) {
		if (_affixes[idxIt->second].slot != AffixSlot::kSuffix) {
			primaryToken = a_slots.tokens[s];
			break;
		}
	}
}
if (primaryToken == 0u) {
	primaryToken = a_slots.GetPrimary();  // fallback: first token regardless of slot
}
```

**Step 2: suffix-only 아이템의 네이밍 형식**

suffix-only 아이템(prefix 없음)의 경우, D2 스타일 "of XXX" 접미사:

star prefix 생성 (line 322-327) 뒤에:

```cpp
// Determine if this is a suffix-only or mixed item for naming
const bool hasPrefixAffix = [&]() {
	for (std::uint8_t s = 0; s < a_slots.count; ++s) {
		const auto idxIt = _affixIndexByToken.find(a_slots.tokens[s]);
		if (idxIt != _affixIndexByToken.end() && idxIt->second < _affixes.size()) {
			if (_affixes[idxIt->second].slot != AffixSlot::kSuffix) {
				return true;
			}
		}
	}
	return false;
}();
```

포맷 적용 (line 329-330):

```cpp
std::string formatted;
if (hasPrefixAffix) {
	formatted = FormatLootName(baseName, primaryAffix.label);
} else {
	// Suffix-only: "Iron Sword of Vitality"
	formatted = baseName + " " + primaryAffix.label;
}
const std::string newName = prefix + formatted;
```

**Step 3: 빌드 확인**

Run: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --config Release 2>&1 | tail -5`
Expected: BUILD SUCCESS

**Step 4: Commit**

```bash
git add src/EventBridge.Loot.Assign.cpp
git commit -m "feat(prefix-suffix): prefix-first naming, suffix-only 'of X' format"
```

---

### Task 8: GetInstanceAffixTooltip suffix 표시

**Files:**
- Modify: `skse/CalamityAffixes/src/EventBridge.Loot.Runtime.cpp:315-347` (formatTooltip 내부)

**Step 1: suffix 슬롯을 구분하여 표시**

기존 slot 라벨 `[1/3]` 뒤에 P/S 표시 추가:

```cpp
// Slot number prefix for multi-affix items
if (slots.count > 1) {
	tooltip.push_back('[');
	// Show P/S indicator
	if (affix.slot == AffixSlot::kPrefix) {
		tooltip.push_back('P');
	} else if (affix.slot == AffixSlot::kSuffix) {
		tooltip.push_back('S');
	}
	tooltip.append(std::to_string(s + 1));
	tooltip.push_back('/');
	tooltip.append(std::to_string(slots.count));
	tooltip.append("] ");
}
```

**Step 2: 빌드 확인**

Run: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --config Release 2>&1 | tail -5`
Expected: BUILD SUCCESS

**Step 3: Commit**

```bash
git add src/EventBridge.Loot.Runtime.cpp
git commit -m "feat(prefix-suffix): show P/S indicator in tooltip slot labels"
```

---

## Phase 6: Suffix JSON 엔트리 추가

### Task 9: affixes.json에 20개 suffix (60 티어) 추가

**Files:**
- Modify: `Data/SKSE/Plugins/CalamityAffixes/affixes.json`

**Step 1: suffix 엔트리 추가**

기존 affixes 배열 끝에 suffix 엔트리를 추가. 각 suffix family × 3 tiers = 3 JSON 엔트리.

**티어별 chance 가중치:**
- T1 (Minor): `"chance": 6.0` (60%)
- T2 (Major): `"chance": 3.0` (30%)
- T3 (Grand): `"chance": 1.0` (10%)

**엔트리 형식 (Max Health 예시):**

```json
{
  "id": "suffix_max_health_t1",
  "editorId": "CAFF_KYWD_SUFFIX_MAX_HEALTH_T1",
  "name": "of Minor Vitality: Max Health +25",
  "slot": "suffix",
  "family": "max_health",
  "kid": {
    "type": "Weapon",
    "strings": "NONE",
    "formFilters": "NONE",
    "traits": "-E",
    "chance": 6.0
  },
  "runtime": {
    "trigger": "Hit",
    "procChancePercent": 0,
    "action": {
      "type": "DebugNotify",
      "passiveSpellEditorId": "CAFF_SPEL_SUFFIX_MAX_HEALTH_T1"
    }
  }
},
{
  "id": "suffix_max_health_t2",
  "editorId": "CAFF_KYWD_SUFFIX_MAX_HEALTH_T2",
  "name": "of Vitality: Max Health +50",
  "slot": "suffix",
  "family": "max_health",
  "kid": {
    "type": "Weapon",
    "strings": "NONE",
    "formFilters": "NONE",
    "traits": "-E",
    "chance": 3.0
  },
  "runtime": {
    "trigger": "Hit",
    "procChancePercent": 0,
    "action": {
      "type": "DebugNotify",
      "passiveSpellEditorId": "CAFF_SPEL_SUFFIX_MAX_HEALTH_T2"
    }
  }
},
{
  "id": "suffix_max_health_t3",
  "editorId": "CAFF_KYWD_SUFFIX_MAX_HEALTH_T3",
  "name": "of Grand Vitality: Max Health +75",
  "slot": "suffix",
  "family": "max_health",
  "kid": {
    "type": "Weapon",
    "strings": "NONE",
    "formFilters": "NONE",
    "traits": "-E",
    "chance": 1.0
  },
  "runtime": {
    "trigger": "Hit",
    "procChancePercent": 0,
    "action": {
      "type": "DebugNotify",
      "passiveSpellEditorId": "CAFF_SPEL_SUFFIX_MAX_HEALTH_T3"
    }
  }
}
```

**전체 20개 suffix 가족 (각 3티어):**

| Family | Label Pattern | T1/T2/T3 Values |
|--------|--------------|-----------------|
| max_health | of (Minor/Grand) Vitality | +25/+50/+75 |
| max_stamina | of (Minor/Grand) Endurance | +20/+40/+60 |
| max_magicka | of (Minor/Grand) Sorcery | +20/+40/+60 |
| health_regen | of (Minor/Grand) Mending | +25%/+50%/+75% |
| stamina_regen | of (Minor/Grand) Recovery | +25%/+50%/+75% |
| magicka_regen | of (Minor/Grand) Insight | +25%/+50%/+75% |
| armor_rating | of (Minor/Grand) Warding | +25/+50/+80 |
| fire_resist | of (Minor/Grand) Flame Guard | +5%/+10%/+15% |
| frost_resist | of (Minor/Grand) Frost Guard | +5%/+10%/+15% |
| shock_resist | of (Minor/Grand) Shock Guard | +5%/+10%/+15% |
| magic_resist | of (Minor/Grand) Spell Guard | +3%/+8%/+12% |
| poison_resist | of (Minor/Grand) Antivenom | +5%/+10%/+15% |
| disease_resist | of (Minor/Grand) Purity | +25%/+50%/+100% |
| attack_speed | of (Minor/Grand) Alacrity | +5%/+10%/+15% |
| critical_damage | of (Minor/Grand) Precision | +10%/+20%/+30% |
| destruction_cost | of (Minor/Grand) Efficiency | -5%/-10%/-15% |
| move_speed | of (Minor/Grand) Swiftness | +4%/+7%/+10% |
| carry_weight | of (Minor/Grand) Burden | +20/+40/+60 |
| shout_cooldown | of (Minor/Grand) the Voice | -5%/-10%/-15% |
| experience_gain | of (Minor/Grand) Wisdom | +5%/+10%/+15% |

**Step 2: JSON 유효성 확인**

Run: `python3 -c "import json; json.load(open('Data/SKSE/Plugins/CalamityAffixes/affixes.json'))"`
Expected: 에러 없음

**Step 3: Commit**

```bash
git add Data/SKSE/Plugins/CalamityAffixes/affixes.json
git commit -m "feat(prefix-suffix): add 60 suffix entries (20 families x 3 tiers) to affixes.json"
```

---

### Task 10: 기존 prefix 어픽스에 slot 필드 추가

**Files:**
- Modify: `Data/SKSE/Plugins/CalamityAffixes/affixes.json`

**Step 1: 기존 26개 lootable 어픽스에 `"slot": "prefix"` 추가**

각 기존 affix 엔트리의 `"id"` 뒤에 `"slot": "prefix"` 추가. runeword-only 어픽스는 생략 (slot 없으면 kNone).

**Step 2: JSON 유효성 확인**

Run: `python3 -c "import json; json.load(open('Data/SKSE/Plugins/CalamityAffixes/affixes.json'))"`
Expected: 에러 없음

**Step 3: Commit**

```bash
git add Data/SKSE/Plugins/CalamityAffixes/affixes.json
git commit -m "feat(prefix-suffix): tag existing 26 lootable affixes as prefix"
```

---

## Phase 7: ESP 레코드 생성 (수동/xEdit)

### Task 11: xEdit 스크립트로 Keyword + MagicEffect + Spell 일괄 생성

**이 태스크는 C++ 코드가 아님 — xEdit Pascal 스크립트 또는 수동 CK 작업.**

**필요 레코드:**
- **60 Keywords**: `CAFF_KYWD_SUFFIX_MAX_HEALTH_T1` ~ `CAFF_KYWD_SUFFIX_EXPERIENCE_GAIN_T3`
- **60 MagicEffects**: `CAFF_MGEF_SUFFIX_MAX_HEALTH_T1` ~ `CAFF_MGEF_SUFFIX_EXPERIENCE_GAIN_T3`
  - ArchType: ValueModifier (ActorValue에 따라)
  - Casting: ConstantEffect
  - Delivery: Self
  - Magnitude: 티어별 값
- **60 Spells**: `CAFF_SPEL_SUFFIX_MAX_HEALTH_T1` ~ `CAFF_SPEL_SUFFIX_EXPERIENCE_GAIN_T3`
  - Type: Ability
  - Casting: ConstantEffect
  - Delivery: Self
  - 각 Spell은 대응하는 MagicEffect 하나를 포함

**ActorValue 매핑:**
| Family | ActorValue | ArchType |
|--------|-----------|----------|
| max_health | Health | ValueModifier |
| max_stamina | Stamina | ValueModifier |
| max_magicka | Magicka | ValueModifier |
| health_regen | HealRate | ValueModifier |
| stamina_regen | StaminaRate | ValueModifier |
| magicka_regen | MagickaRate | ValueModifier |
| armor_rating | DamageResist | ValueModifier |
| fire_resist | ResistFire | ValueModifier |
| frost_resist | ResistFrost | ValueModifier |
| shock_resist | ResistShock | ValueModifier |
| magic_resist | ResistMagic | ValueModifier |
| poison_resist | PoisonResist | ValueModifier |
| disease_resist | ResistDisease | ValueModifier |
| attack_speed | WeaponSpeedMult | ValueModifier |
| critical_damage | CriticalChance | ValueModifier |
| destruction_cost | DestructionModifier | ValueModifier |
| move_speed | SpeedMult | ValueModifier |
| carry_weight | CarryWeight | ValueModifier |
| shout_cooldown | ShoutRecoveryMult | ValueModifier |
| experience_gain | — (custom script effect) | Script |

**Note:** `experience_gain`과 `attack_speed`는 일반 ValueModifier로 구현 불가능할 수 있음 — 별도 검토 필요. `WeaponSpeedMult`는 Skyrim에서 공격 속도에 영향을 주는 AV. `ShoutRecoveryMult`도 마찬가지. 이들의 정확한 동작은 인게임 테스트로 확인.

**이 태스크의 산출물:**
- `CalamityAffixes.esp`에 180개 레코드 추가 (60 KYWD + 60 MGEF + 60 SPEL)
- 또는 xEdit Pascal 스크립트 제공

---

## Phase 8: 통합 테스트 및 빌드 검증

### Task 12: 전체 빌드 + static_assert 테스트

**Files:**
- Test: `skse/CalamityAffixes/tests/` (기존 테스트)

**Step 1: 빌드 확인**

Run: `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel --config Release 2>&1 | tail -20`
Expected: BUILD SUCCESS

**Step 2: 기존 테스트 통과 확인**

Run: `cd skse/CalamityAffixes/build.linux-clangcl-rel && ctest --output-on-failure`
Expected: ALL TESTS PASSED

**Step 3: 최종 Commit**

```bash
git add -A
git commit -m "feat(prefix-suffix): complete Prefix/Suffix system v1 implementation"
```

---

## 파일 변경 요약

| Phase | 파일 | 변경 |
|-------|------|------|
| 1 | `EventBridge.h` | AffixSlot enum, AffixRuntime 필드, suffix 풀 벡터 |
| 2 | `EventBridge.Config.cpp` | slot/family/passiveSpell 파싱, suffix 풀 빌드 |
| 3 | `EventBridge.Loot.Assign.cpp` | RollSuffixIndex, P/S 분배 로직 |
| 4 | `EventBridge.Triggers.Events.cpp` | 패시브 스펠 apply/remove |
| 4 | `EventBridge.Serialization.cpp` | Revert에 passive clear |
| 5 | `EventBridge.Loot.Assign.cpp` | prefix 우선 네이밍 |
| 5 | `EventBridge.Loot.Runtime.cpp` | P/S 툴팁 표시 |
| 6 | `affixes.json` | 60 suffix 엔트리 + 기존 prefix 태깅 |
| 7 | `CalamityAffixes.esp` | 180 레코드 (수동/xEdit) |

## 직렬화 영향
- **변경 없음** — suffix 토큰도 일반 affix 토큰과 동일하게 InstanceAffixSlots에 저장됨
- v6 포맷 호환, kSerializationVersion 변경 불필요
- 기존 세이브 파일 완전 호환

## 롤링 규칙 요약

| 어픽스 수 | 확률 | 조합 |
|-----------|------|------|
| 1 (70%) | 50/50 | 1P or 1S |
| 2 (25%) | 고정 | 1P + 1S |
| 3 (5%) | 50/50 | 2P+1S or 1P+2S |

## Suffix 티어 출현률

| Tier | chance 가중치 | 실질 확률 |
|------|-------------|----------|
| Minor (T1) | 6.0 | 60% |
| Major (T2) | 3.0 | 30% |
| Grand (T3) | 1.0 | 10% |
