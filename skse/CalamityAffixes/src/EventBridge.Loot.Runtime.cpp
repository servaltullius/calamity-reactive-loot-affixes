#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootRollSelection.h"
#include "CalamityAffixes/LootUiGuards.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	namespace
	{
		[[nodiscard]] std::uint64_t NowSteadyMilliseconds() noexcept
		{
			return static_cast<std::uint64_t>(
				std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now().time_since_epoch())
					.count());
		}

		static constexpr std::uint64_t kSelectedPreviewHintTtlMs = 120000u;
		static constexpr std::size_t kSelectedPreviewHintMaxPerBaseObj = 12u;

		[[nodiscard]] bool IsPreviewItemSource(std::string_view a_source) noexcept
		{
			return a_source == "barter" || a_source == "container" || a_source == "gift";
		}

		[[nodiscard]] std::uint64_t NextPreviewRand(std::uint64_t& a_state) noexcept
		{
			a_state += 0x9E3779B97F4A7C15ull;
			std::uint64_t z = a_state;
			z = (z ^ (z >> 30u)) * 0xBF58476D1CE4E5B9ull;
			z = (z ^ (z >> 27u)) * 0x94D049BB133111EBull;
			return z ^ (z >> 31u);
		}

		[[nodiscard]] double NextPreviewUnit(std::uint64_t& a_state) noexcept
		{
			// Keep only 53 bits so the value maps cleanly to IEEE-754 double mantissa.
			const auto value = NextPreviewRand(a_state) >> 11u;
			return static_cast<double>(value) * (1.0 / 9007199254740992.0);
		}
	}

		EventBridge::InstanceRuntimeState& EventBridge::EnsureInstanceRuntimeState(
			std::uint64_t a_instanceKey,
			std::uint64_t a_affixToken)
		{
			return _instanceStates[MakeInstanceStateKey(a_instanceKey, a_affixToken)];
		}

	const EventBridge::InstanceRuntimeState* EventBridge::FindInstanceRuntimeState(
		std::uint64_t a_instanceKey,
		std::uint64_t a_affixToken) const
	{
		if (a_affixToken == 0u) {
			return nullptr;
		}

		const auto it = _instanceStates.find(MakeInstanceStateKey(a_instanceKey, a_affixToken));
		return (it != _instanceStates.end()) ? std::addressof(it->second) : nullptr;
	}

	std::size_t EventBridge::ResolveEvolutionStageIndex(const Action& a_action, const InstanceRuntimeState* a_state) const
	{
		if (!a_action.evolutionEnabled || a_action.evolutionThresholds.empty()) {
			return 0;
		}

		const std::uint32_t xp = a_state ? a_state->evolutionXp : 0u;
		std::size_t stage = 0;
		for (std::size_t i = 0; i < a_action.evolutionThresholds.size(); ++i) {
			if (xp < a_action.evolutionThresholds[i]) {
				break;
			}
			stage = i;
		}
		return stage;
	}

	float EventBridge::ResolveEvolutionMultiplier(const Action& a_action, const InstanceRuntimeState* a_state) const
	{
		if (!a_action.evolutionEnabled || a_action.evolutionMultipliers.empty()) {
			return 1.0f;
		}

		std::size_t stage = ResolveEvolutionStageIndex(a_action, a_state);
		if (stage >= a_action.evolutionMultipliers.size()) {
			stage = a_action.evolutionMultipliers.size() - 1;
		}

		const float mult = a_action.evolutionMultipliers[stage];
		return (mult > 0.0f) ? mult : 1.0f;
	}

	void EventBridge::AdvanceRuntimeStateForAffixProc(const AffixRuntime& a_affix)
	{
		const auto& action = a_affix.action;
		const bool usesEvolution = action.evolutionEnabled && action.evolutionXpPerProc > 0u;
		const bool usesModeCycle = action.modeCycleEnabled && !action.modeCycleSpells.empty() && !action.modeCycleManualOnly;
		if (!usesEvolution && !usesModeCycle) {
			return;
		}

		const auto* cachedKeys = FindEquippedInstanceKeysForAffixTokenCached(a_affix.token);
		std::vector<std::uint64_t> fallbackKeys;
		if (!cachedKeys) {
			fallbackKeys = CollectEquippedInstanceKeysForAffixToken(a_affix.token);
			cachedKeys = std::addressof(fallbackKeys);
		}
		if (!cachedKeys || cachedKeys->empty()) {
			return;
		}

		const std::uint32_t xpPerProc = action.evolutionXpPerProc;
		const std::uint32_t switchEvery = std::max<std::uint32_t>(1u, action.modeCycleSwitchEveryProcs);
		const auto modeCount = static_cast<std::uint32_t>(action.modeCycleSpells.size());

		for (const auto key : *cachedKeys) {
			auto& state = EnsureInstanceRuntimeState(key, a_affix.token);

			if (usesEvolution) {
				const auto maxVal = std::numeric_limits<std::uint32_t>::max();
				if (state.evolutionXp > maxVal - xpPerProc) {
					state.evolutionXp = maxVal;
				} else {
					state.evolutionXp += xpPerProc;
				}
			}

			if (usesModeCycle && modeCount > 0u) {
				const auto maxVal = std::numeric_limits<std::uint32_t>::max();
				if (state.modeCycleCounter < maxVal) {
					state.modeCycleCounter += 1u;
				}
				if (state.modeCycleCounter >= switchEvery) {
					state.modeCycleCounter = 0u;
					state.modeIndex = (state.modeIndex + 1u) % modeCount;
				}
			}
		}
	}

	void EventBridge::CycleManualModeForEquippedAffixes(std::int32_t a_direction, std::string_view a_affixIdFilter)
	{
		if (!_configLoaded || a_direction == 0) {
			return;
		}

		std::uint32_t switched = 0u;
		for (const auto& affix : _affixes) {
			const auto& action = affix.action;
			if (action.type != ActionType::kCastSpell ||
				!action.modeCycleEnabled ||
				!action.modeCycleManualOnly ||
				action.modeCycleSpells.empty()) {
				continue;
			}

			if (!a_affixIdFilter.empty() && affix.id != a_affixIdFilter) {
				continue;
			}

			const auto modeCount = static_cast<std::int32_t>(action.modeCycleSpells.size());
			if (modeCount <= 0) {
				continue;
			}

			const auto keys = CollectEquippedInstanceKeysForAffixToken(affix.token);
			for (const auto key : keys) {
				auto& state = EnsureInstanceRuntimeState(key, affix.token);
				const std::int32_t current = static_cast<std::int32_t>(state.modeIndex % static_cast<std::uint32_t>(modeCount));
				std::int32_t next = (current + a_direction) % modeCount;
				if (next < 0) {
					next += modeCount;
				}

				state.modeIndex = static_cast<std::uint32_t>(next);
				state.modeCycleCounter = 0u;
				switched += 1u;
			}
		}

		if (_loot.debugLog && switched > 0u) {
			spdlog::debug(
				"CalamityAffixes: manual mode cycle switched (count={}, direction={}, filter='{}').",
				switched,
				a_direction,
				std::string(a_affixIdFilter));
		}
	}

	const InstanceAffixSlots* EventBridge::FindLootPreviewSlots(std::uint64_t a_instanceKey) const
	{
		if (a_instanceKey == 0u) {
			return nullptr;
		}

		const auto it = _lootPreviewAffixes.find(a_instanceKey);
		if (it == _lootPreviewAffixes.end()) {
			return nullptr;
		}

		return std::addressof(it->second);
	}

	std::uint64_t EventBridge::FindSelectedLootPreviewKey(RE::FormID a_baseObj, std::uint64_t a_nowMs)
	{
		if (a_baseObj == 0u) {
			return 0u;
		}

		const auto it = _lootPreviewSelectedByBaseObj.find(a_baseObj);
		if (it == _lootPreviewSelectedByBaseObj.end()) {
			return 0u;
		}

		auto& hints = it->second;
		for (auto hintIt = hints.begin(); hintIt != hints.end();) {
			const auto key = hintIt->instanceKey;
			const bool keyMatchesBaseObj = key != 0u && static_cast<RE::FormID>(key >> 16u) == a_baseObj;
			const bool hintFresh = IsSelectedLootPreviewHintFresh(a_nowMs, hintIt->recordedAtMs, kSelectedPreviewHintTtlMs);
			const bool previewTracked = keyMatchesBaseObj && FindLootPreviewSlots(key) != nullptr;
			if (!previewTracked || !hintFresh) {
				hintIt = hints.erase(hintIt);
			} else {
				++hintIt;
			}
		}

		if (hints.empty()) {
			_lootPreviewSelectedByBaseObj.erase(it);
			return 0u;
		}

		return hints.back().instanceKey;
	}

	void EventBridge::RememberSelectedLootPreviewKey(RE::FormID a_baseObj, std::uint64_t a_instanceKey, std::uint64_t a_nowMs)
	{
		if (a_baseObj == 0u || a_instanceKey == 0u) {
			return;
		}
		if (static_cast<RE::FormID>(a_instanceKey >> 16u) != a_baseObj) {
			return;
		}

		auto& hints = _lootPreviewSelectedByBaseObj[a_baseObj];
		std::erase_if(
			hints,
			[=](const SelectedLootPreviewHint& a_hint) {
				return a_hint.instanceKey == a_instanceKey;
			});
		hints.push_back(SelectedLootPreviewHint{
			.instanceKey = a_instanceKey,
			.recordedAtMs = a_nowMs
		});
		while (hints.size() > kSelectedPreviewHintMaxPerBaseObj) {
			hints.pop_front();
		}
	}

	void EventBridge::ForgetSelectedLootPreviewKeyForInstance(std::uint64_t a_instanceKey)
	{
		if (a_instanceKey == 0u || _lootPreviewSelectedByBaseObj.empty()) {
			return;
		}

		for (auto it = _lootPreviewSelectedByBaseObj.begin(); it != _lootPreviewSelectedByBaseObj.end();) {
			auto& hints = it->second;
			std::erase_if(
				hints,
				[=](const SelectedLootPreviewHint& a_hint) {
					return a_hint.instanceKey == a_instanceKey;
				});
			if (hints.empty()) {
				it = _lootPreviewSelectedByBaseObj.erase(it);
			} else {
				++it;
			}
		}
	}

	void EventBridge::RememberLootPreviewSlots(std::uint64_t a_instanceKey, const InstanceAffixSlots& a_slots)
	{
		if (a_instanceKey == 0u) {
			return;
		}

		const auto [_, inserted] = _lootPreviewAffixes.insert_or_assign(a_instanceKey, a_slots);
		if (!inserted) {
			return;
		}

		_lootPreviewRecent.push_back(a_instanceKey);
		while (_lootPreviewAffixes.size() > kLootPreviewCacheMax && !_lootPreviewRecent.empty()) {
			const auto oldest = _lootPreviewRecent.front();
			_lootPreviewRecent.pop_front();
			if (oldest != 0u) {
				_lootPreviewAffixes.erase(oldest);
			}
		}
	}

	void EventBridge::ForgetLootPreviewSlots(std::uint64_t a_instanceKey)
	{
		if (a_instanceKey == 0u) {
			return;
		}
		_lootPreviewAffixes.erase(a_instanceKey);
		std::erase(_lootPreviewRecent, a_instanceKey);
		ForgetSelectedLootPreviewKeyForInstance(a_instanceKey);
	}

	std::uint64_t EventBridge::HashLootPreviewSeed(std::uint64_t a_instanceKey, std::uint64_t a_salt)
	{
		std::uint64_t x = a_instanceKey ^ a_salt;
		x ^= x >> 33u;
		x *= 0xff51afd7ed558ccdull;
		x ^= x >> 33u;
		x *= 0xc4ceb9fe1a85ec53ull;
		x ^= x >> 33u;
		return x;
	}

	std::optional<InstanceAffixSlots> EventBridge::BuildLootPreviewAffixSlots(
		std::uint64_t a_instanceKey,
		LootItemType a_itemType) const
	{
		const std::vector<std::size_t>* prefixPool = nullptr;
		const std::vector<std::size_t>* suffixPool = nullptr;
		if (_loot.sharedPool) {
			prefixPool = std::addressof(_lootSharedAffixes);
			suffixPool = std::addressof(_lootSharedSuffixes);
		} else {
			prefixPool = (a_itemType == LootItemType::kWeapon) ? std::addressof(_lootWeaponAffixes) : std::addressof(_lootArmorAffixes);
			suffixPool = (a_itemType == LootItemType::kWeapon) ? std::addressof(_lootWeaponSuffixes) : std::addressof(_lootArmorSuffixes);
		}

		if (!prefixPool || prefixPool->empty()) {
			return std::nullopt;
		}

		InstanceAffixSlots slots{};

		const float chancePct = std::clamp(_loot.chancePercent, 0.0f, 100.0f);
		if (chancePct <= 0.0f) {
			return slots;
		}

		std::uint64_t rngState = HashLootPreviewSeed(
			a_instanceKey,
			(a_itemType == LootItemType::kWeapon) ? 0xA11CE5F17E5ull : 0xA11CE5F17A2ull);

		const double chanceRoll = NextPreviewUnit(rngState) * 100.0;
		if (chanceRoll >= static_cast<double>(chancePct)) {
			return slots;
		}

		const auto rollAffixCount = [&]() -> std::uint8_t {
			double totalWeight = 0.0;
			for (const auto weight : kAffixCountWeights) {
				totalWeight += std::max(0.0, static_cast<double>(weight));
			}
			if (totalWeight <= 0.0) {
				return 1u;
			}

			double roll = NextPreviewUnit(rngState) * totalWeight;
			for (std::size_t i = 0; i < kAffixCountWeights.size(); ++i) {
				const double weight = std::max(0.0, static_cast<double>(kAffixCountWeights[i]));
				if (weight <= 0.0) {
					continue;
				}
				if (roll < weight) {
					return static_cast<std::uint8_t>(i + 1u);
				}
				roll -= weight;
			}

			return static_cast<std::uint8_t>(kAffixCountWeights.size());
		};

		const auto pickWeightedIndex = [&](const std::vector<std::size_t>& a_pool, auto&& a_isEligible) -> std::optional<std::size_t> {
			double totalWeight = 0.0;
			for (const auto idx : a_pool) {
				if (idx >= _affixes.size()) {
					continue;
				}
				if (!a_isEligible(idx)) {
					continue;
				}
				const double weight = std::max(0.0, static_cast<double>(_affixes[idx].EffectiveLootWeight()));
				totalWeight += weight;
			}

			if (totalWeight <= 0.0) {
				return std::nullopt;
			}

			double roll = NextPreviewUnit(rngState) * totalWeight;
			std::optional<std::size_t> lastEligible;
			for (const auto idx : a_pool) {
				if (idx >= _affixes.size()) {
					continue;
				}
				if (!a_isEligible(idx)) {
					continue;
				}
				const double weight = std::max(0.0, static_cast<double>(_affixes[idx].EffectiveLootWeight()));
				if (weight <= 0.0) {
					continue;
				}
				lastEligible = idx;
				if (roll < weight) {
					return idx;
				}
				roll -= weight;
			}

			return lastEligible;
		};

		const std::uint8_t targetCount = rollAffixCount();
		const auto targets = detail::DetermineLootPrefixSuffixTargets(targetCount);
		const std::uint8_t prefixTarget = targets.prefixTarget;
		const std::uint8_t suffixTarget = targets.suffixTarget;

		std::vector<std::size_t> chosenPrefixIndices;
		chosenPrefixIndices.reserve(prefixTarget);
		for (std::uint8_t p = 0; p < prefixTarget; ++p) {
			const auto idx = pickWeightedIndex(*prefixPool, [&](std::size_t a_idx) {
				if (std::find(chosenPrefixIndices.begin(), chosenPrefixIndices.end(), a_idx) != chosenPrefixIndices.end()) {
					return false;
				}
				return _affixes[a_idx].slot != AffixSlot::kSuffix;
			});
			if (!idx) {
				break;
			}
			chosenPrefixIndices.push_back(*idx);
			slots.AddToken(_affixes[*idx].token);
		}

		std::vector<std::string> chosenFamilies;
		chosenFamilies.reserve(suffixTarget);
		for (std::uint8_t s = 0; s < suffixTarget; ++s) {
			if (!detail::ShouldConsumeSuffixRollForSingleAffixTarget(targetCount, slots.count)) {
				break;
			}

			const auto idx = pickWeightedIndex(*suffixPool, [&](std::size_t a_idx) {
				const auto& affix = _affixes[a_idx];
				if (affix.slot != AffixSlot::kSuffix) {
					return false;
				}
				if (!affix.family.empty() &&
					std::find(chosenFamilies.begin(), chosenFamilies.end(), affix.family) != chosenFamilies.end()) {
					return false;
				}
				return true;
			});
			if (!idx) {
				break;
			}

			const auto& affix = _affixes[*idx];
			if (!affix.family.empty()) {
				chosenFamilies.push_back(affix.family);
			}
			slots.AddToken(affix.token);
		}

		return slots;
	}

	std::optional<std::string> EventBridge::GetInstanceAffixTooltip(
		const RE::InventoryEntryData* a_item,
		std::string_view a_selectedDisplayName,
		int a_uiLanguageMode,
		std::string_view a_itemSource)
	{
		if (!_configLoaded) {
			return std::nullopt;
		}
		if (!a_item) {
			return std::nullopt;
		}

		if (!a_item->extraLists) {
			return std::nullopt;
		}

		std::optional<LootItemType> itemType{};
		if (a_item->object && a_item->object->As<RE::TESObjectWEAP>()) {
			itemType = LootItemType::kWeapon;
		} else if (a_item->object && a_item->object->As<RE::TESObjectARMO>()) {
			itemType = LootItemType::kArmor;
		}

		const bool allowPreview =
			itemType.has_value() &&
			IsPreviewItemSource(a_itemSource) &&
			IsLootObjectEligibleForAffixes(a_item->object);
		const RE::FormID itemBaseObj = (a_item->object ? a_item->object->GetFormID() : 0u);

		struct TooltipCandidate
		{
			std::uint64_t instanceKey{ 0u };
			std::uint64_t primaryToken{ 0u };
			std::string rowName{};
			InstanceAffixSlots slots{};
			bool preview{ false };
		};

		std::vector<TooltipCandidate> candidates;
		for (auto* xList : *a_item->extraLists) {
			if (!xList) {
				continue;
			}

			auto* uid = xList->GetByType<RE::ExtraUniqueID>();
			if (!uid && allowPreview) {
				auto* player = RE::PlayerCharacter::GetSingleton();
				auto* changes = player ? player->GetInventoryChanges() : nullptr;
				if (changes) {
					auto* created = RE::BSExtraData::Create<RE::ExtraUniqueID>();
					if (created) {
						created->baseID = a_item->object ? a_item->object->GetFormID() : 0u;
						created->uniqueID = changes->GetNextUniqueID();
						xList->Add(created);
						uid = created;
					}
				}
			}

			if (!uid) {
				continue;
			}

			const auto key = MakeInstanceKey(uid->baseID, uid->uniqueID);

			TooltipCandidate candidate{};
			candidate.instanceKey = key;
			candidate.preview = false;
			candidate.slots.Clear();

			if (const auto mappedIt = _instanceAffixes.find(key); mappedIt != _instanceAffixes.end()) {
				candidate.slots = mappedIt->second;
			}

			if (candidate.slots.count == 0 && allowPreview) {
				if (const auto* previewSlots = FindLootPreviewSlots(key)) {
					candidate.slots = *previewSlots;
					candidate.preview = true;
				} else if (const auto generatedPreview = BuildLootPreviewAffixSlots(key, *itemType); generatedPreview.has_value()) {
					RememberLootPreviewSlots(key, *generatedPreview);
					if (const auto* storedPreview = FindLootPreviewSlots(key)) {
						candidate.slots = *storedPreview;
						candidate.preview = true;
					}
				}
			}

			// For normal runtime candidates, skip items without mapped slots.
			// For preview mode, keep zero-slot candidates so users can see "no affix" before pickup.
			if (candidate.slots.count == 0 && !candidate.preview) {
				continue;
			}

			candidate.primaryToken = candidate.slots.GetPrimary();
			const char* rowNameRaw = xList->GetDisplayName(a_item->object);
			candidate.rowName = rowNameRaw ? rowNameRaw : "";
			candidates.push_back(std::move(candidate));
		}

		if (candidates.empty()) {
			return std::nullopt;
		}

		auto formatAffixDetail = [&](const AffixRuntime& a_affix, std::uint64_t a_instanceKey) -> std::string {
			std::string detail;
			const auto* state = FindInstanceRuntimeState(a_instanceKey, a_affix.token);
			const auto& action = a_affix.action;

			if (action.modeCycleEnabled && !action.modeCycleSpells.empty()) {
				const auto modeCount = static_cast<std::uint32_t>(action.modeCycleSpells.size());
				const std::uint32_t idx = modeCount > 0u ? ((state ? state->modeIndex : 0u) % modeCount) : 0u;

				detail.append("Mode ");
				if (idx < action.modeCycleLabels.size() && !action.modeCycleLabels[idx].empty()) {
					detail.append(action.modeCycleLabels[idx]);
				} else {
					detail.append(std::to_string(idx + 1));
					detail.push_back('/');
					detail.append(std::to_string(modeCount));
				}
				if (action.modeCycleManualOnly) {
					detail.append(" (Manual)");
				}
			}

			return detail;
		};

		auto resolveDisplayName = [&](const AffixRuntime& a_affix) -> std::string {
			switch (a_uiLanguageMode) {
			case 0:
				return a_affix.displayNameEn;
			case 1:
				return a_affix.displayNameKo;
			case 2:
			default:
				if (a_affix.displayNameKo == a_affix.displayNameEn || a_affix.displayNameEn.empty()) {
					return a_affix.displayNameKo;
				}
				if (a_affix.displayNameKo.empty()) {
					return a_affix.displayNameEn;
				}
				return a_affix.displayNameKo + " / " + a_affix.displayNameEn;
			}
		};

		auto formatTooltip = [&](const TooltipCandidate& a_candidate) -> std::string {
			if (a_candidate.slots.count == 0) {
				if (a_candidate.preview) {
					return "Preview: No affix";
				}
				return {};
			}

			std::string tooltip;
			if (a_candidate.preview) {
				tooltip.append("Preview\n");
			}

			for (std::uint8_t s = 0; s < a_candidate.slots.count; ++s) {
				const auto token = a_candidate.slots.tokens[s];
				const auto idxIt = _affixIndexByToken.find(token);
				if (idxIt == _affixIndexByToken.end() || idxIt->second >= _affixes.size()) {
					continue;
				}

				const auto& affix = _affixes[idxIt->second];
				auto resolvedName = resolveDisplayName(affix);
				if (resolvedName.empty()) {
					continue;
				}

				if (!tooltip.empty() && tooltip.back() != '\n') {
					tooltip.push_back('\n');
				}

				if (a_candidate.slots.count > 1) {
					tooltip.push_back('[');
					if (affix.slot == AffixSlot::kPrefix) {
						tooltip.push_back('P');
					} else if (affix.slot == AffixSlot::kSuffix) {
						tooltip.push_back('S');
					}
					tooltip.append(std::to_string(s + 1));
					tooltip.push_back('/');
					tooltip.append(std::to_string(a_candidate.slots.count));
					tooltip.append("] ");
				}

				tooltip.append(resolvedName);

				const auto detail = formatAffixDetail(affix, a_candidate.instanceKey);
				if (!detail.empty()) {
					tooltip.push_back('\n');
					tooltip.append(detail);
				}
			}

			return tooltip;
		};

		std::vector<TooltipResolutionCandidate> resolutionCandidates;
		resolutionCandidates.reserve(candidates.size());
		for (const auto& candidate : candidates) {
			resolutionCandidates.push_back(TooltipResolutionCandidate{
				.rowName = candidate.rowName,
				.affixToken = candidate.primaryToken,
				.instanceKey = candidate.instanceKey
			});
		}

		const auto resolution = ResolveTooltipCandidateSelection(resolutionCandidates, a_selectedDisplayName);
		if (resolution.matchedIndex &&
			*resolution.matchedIndex < candidates.size() &&
			!resolution.ambiguous) {
			const auto& matched = candidates[*resolution.matchedIndex];
			if (allowPreview && itemBaseObj != 0u) {
				if (matched.preview) {
					const auto nowMs = NowSteadyMilliseconds();
					RememberSelectedLootPreviewKey(itemBaseObj, matched.instanceKey, nowMs);
				} else {
					_lootPreviewSelectedByBaseObj.erase(itemBaseObj);
				}
			}
			const auto formatted = formatTooltip(matched);
			if (formatted.empty()) {
				return std::nullopt;
			}
			return formatted;
		}

		if (allowPreview && itemBaseObj != 0u) {
			std::vector<std::uint64_t> previewCandidateKeys;
			std::unordered_set<std::uint64_t> seenPreviewKeys;
			previewCandidateKeys.reserve(candidates.size());
			for (const auto& candidate : candidates) {
				if (!candidate.preview || candidate.instanceKey == 0u) {
					continue;
				}
				if (seenPreviewKeys.insert(candidate.instanceKey).second) {
					previewCandidateKeys.push_back(candidate.instanceKey);
				}
			}

			if (previewCandidateKeys.size() == 1u) {
				const auto nowMs = NowSteadyMilliseconds();
				RememberSelectedLootPreviewKey(itemBaseObj, previewCandidateKeys.front(), nowMs);
			} else if (previewCandidateKeys.size() > 1u) {
				_lootPreviewSelectedByBaseObj.erase(itemBaseObj);
			} else if (previewCandidateKeys.empty()) {
				_lootPreviewSelectedByBaseObj.erase(itemBaseObj);
			}
		}

		if (!a_selectedDisplayName.empty()) {
			return std::nullopt;
		}

		const auto fallback = formatTooltip(candidates.front());
		if (fallback.empty()) {
			return std::nullopt;
		}
		return fallback;
	}
}
