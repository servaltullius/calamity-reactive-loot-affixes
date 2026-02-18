#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootRollSelection.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CalamityAffixes
{
	namespace
	{
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
		std::uint8_t prefixTarget = targets.prefixTarget;
		std::uint8_t suffixTarget = targets.suffixTarget;
		if (_loot.stripTrackedSuffixSlots) {
			prefixTarget = targetCount;
			suffixTarget = 0u;
		}

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
}
