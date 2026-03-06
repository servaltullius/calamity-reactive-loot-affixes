#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootCurrencyLedger.h"
#include "CalamityAffixes/LootCurrencySourceHelpers.h"
#include "CalamityAffixes/LootEligibility.h"
#include "CalamityAffixes/LootRollSelection.h"
#include "EventBridge.Config.Shared.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace CalamityAffixes
{
	namespace
	{
		static constexpr std::size_t kArmorTemplateScanDepth = 8;
		using LootCurrencySourceTier = detail::LootCurrencySourceTier;
		static_assert(!RuntimePolicy::kAllowPickupRandomAffixAssignment);
		static_assert(!RuntimePolicy::kAllowLegacyPickupAffixRollBranch);

		[[nodiscard]] bool IsCalamityResourceCurrencyItem(const RE::TESBoundObject* a_object) noexcept
		{
			if (!a_object || !a_object->As<RE::TESObjectMISC>()) {
				return false;
			}

			const char* editorIdRaw = a_object->GetFormEditorID();
			if (!editorIdRaw || editorIdRaw[0] == '\0') {
				return false;
			}

			const std::string_view editorId(editorIdRaw);
			return editorId == "CAFF_Misc_ReforgeOrb" || editorId.starts_with("CAFF_RuneFrag_");
		}
	}

	bool EventBridge::IsRuntimeCurrencyDropRollEnabled(std::string_view a_contextTag) const
	{
		if (_loot.runtimeCurrencyDropsEnabled) {
			return true;
		}

		if (_loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: {} currency roll skipped (runtime currency gate disabled).",
				a_contextTag);
		}
		return false;
	}

	float EventBridge::ResolveLootCurrencySourceChanceMultiplier(detail::LootCurrencySourceTier a_sourceTier) const noexcept
	{
		float sourceChanceMultiplier = _loot.lootSourceChanceMultContainer;
		switch (a_sourceTier) {
		case detail::LootCurrencySourceTier::kCorpse:
			sourceChanceMultiplier = _loot.lootSourceChanceMultCorpse;
			break;
		case detail::LootCurrencySourceTier::kContainer:
			sourceChanceMultiplier = _loot.lootSourceChanceMultContainer;
			break;
		case detail::LootCurrencySourceTier::kBossContainer:
			sourceChanceMultiplier = _loot.lootSourceChanceMultBossContainer;
			break;
		case detail::LootCurrencySourceTier::kWorld:
			sourceChanceMultiplier = _loot.lootSourceChanceMultWorld;
			break;
		default:
			sourceChanceMultiplier = _loot.lootSourceChanceMultContainer;
			break;
		}
		return std::clamp(sourceChanceMultiplier, 0.0f, 5.0f);
	}

	bool EventBridge::TryBeginLootCurrencyLedgerRoll(std::uint64_t a_ledgerKey, std::uint32_t a_dayStamp)
	{
		if (a_ledgerKey == 0u) {
			return true;
		}

		if (auto ledgerIt = _lootState.currencyRollLedger.find(a_ledgerKey);
			ledgerIt != _lootState.currencyRollLedger.end()) {
			if (detail::IsLootCurrencyLedgerExpired(
					ledgerIt->second,
					a_dayStamp,
					kLootCurrencyLedgerTtlDays)) {
				_lootState.currencyRollLedger.erase(ledgerIt);
			} else {
				return false;
			}
		}

		return true;
	}

	void EventBridge::FinalizeLootCurrencyLedgerRoll(std::uint64_t a_ledgerKey, std::uint32_t a_dayStamp)
	{
		if (a_ledgerKey == 0u) {
			return;
		}

		const auto [it, inserted] = _lootState.currencyRollLedger.emplace(a_ledgerKey, a_dayStamp);
		if (!inserted) {
			it->second = a_dayStamp;
		}
		if (inserted) {
			_lootState.currencyRollLedgerRecent.push_back(a_ledgerKey);
			while (_lootState.currencyRollLedgerRecent.size() > kLootCurrencyLedgerMaxEntries) {
				const auto oldest = _lootState.currencyRollLedgerRecent.front();
				_lootState.currencyRollLedgerRecent.pop_front();
				_lootState.currencyRollLedger.erase(oldest);
			}
		}
	}

	std::uint64_t EventBridge::MakeInstanceKey(RE::FormID a_baseID, std::uint16_t a_uniqueID) noexcept
	{
		return (static_cast<std::uint64_t>(a_baseID) << 16) | static_cast<std::uint64_t>(a_uniqueID);
	}

	bool EventBridge::IsLootArmorEligibleForAffixes(const RE::TESObjectARMO* a_armor) const
	{
		if (!a_armor) {
			return false;
		}

		bool hasSlotMask = false;
		bool hasArmorAddons = false;

		const RE::TESObjectARMO* cursor = a_armor;
		for (std::size_t depth = 0; cursor && depth < kArmorTemplateScanDepth; ++depth) {
			hasSlotMask = hasSlotMask || (cursor->GetSlotMask() != RE::BIPED_MODEL::BipedObjectSlot::kNone);
			hasArmorAddons = hasArmorAddons || !cursor->armorAddons.empty();
			if (!cursor->templateArmor) {
				break;
			}
			if (cursor->templateArmor == cursor) {
				break;
			}
			cursor = cursor->templateArmor;
		}

		const char* editorIdRaw = a_armor->GetFormEditorID();
		const std::string_view editorId = editorIdRaw ? std::string_view(editorIdRaw) : std::string_view{};
		const bool editorIdDenied =
			detail::MatchesAnyCaseInsensitiveMarker(editorId, _loot.armorEditorIdDenyContains);

		const detail::LootArmorEligibilityInput input{
			.playable = a_armor->GetPlayable(),
			.hasSlotMask = hasSlotMask,
			.hasArmorAddons = hasArmorAddons,
			.editorIdDenied = editorIdDenied
		};
		return detail::IsLootArmorEligible(input);
	}

	bool EventBridge::IsLootObjectEligibleForAffixes(const RE::TESBoundObject* a_object) const
	{
		if (!a_object) {
			return false;
		}
		if (a_object->As<RE::TESObjectWEAP>()) {
			return true;
		}
		if (const auto* armo = a_object->As<RE::TESObjectARMO>()) {
			return IsLootArmorEligibleForAffixes(armo);
		}
		return false;
	}

	std::optional<EventBridge::LootItemType> EventBridge::ParseLootItemType(std::string_view a_kidType) const
	{
		std::string type{ a_kidType };
		std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (type.find("weapon") != std::string::npos) {
			return LootItemType::kWeapon;
		}
		if (type.find("armor") != std::string::npos) {
			return LootItemType::kArmor;
		}

		return std::nullopt;
	}

	std::string EventBridge::FormatLootName(std::string_view a_baseName, std::string_view a_affixName) const
	{
		std::string out = _loot.nameFormat;

		ConfigShared::ReplaceAll(out, "{base}", a_baseName);
		ConfigShared::ReplaceAll(out, "{affix}", a_affixName);
		return out;
	}

	std::optional<std::size_t> EventBridge::RollLootAffixIndex(LootItemType a_itemType, const std::vector<std::size_t>* a_exclude, bool a_skipChanceCheck)
	{
		if (!a_skipChanceCheck && !RollLootChanceGateForEligibleInstance()) {
			return std::nullopt;
		}

		const std::vector<std::size_t>* sourcePool = nullptr;
		if (_loot.sharedPool) {
			sourcePool = &_affixRegistry.lootSharedAffixes;
		} else {
			sourcePool = (a_itemType == LootItemType::kWeapon) ? &_affixRegistry.lootWeaponAffixes : &_affixRegistry.lootArmorAffixes;
		}

		LootShuffleBagState& bagState = _loot.sharedPool ?
			_lootState.prefixSharedBag :
			((a_itemType == LootItemType::kWeapon) ? _lootState.prefixWeaponBag : _lootState.prefixArmorBag);
		const auto picked = detail::SelectWeightedEligibleLootIndexWithShuffleBag(
			_rng,
			*sourcePool,
			bagState.order,
			bagState.cursor,
			[&](std::size_t a_idx) {
				if (a_idx >= _affixes.size()) {
					return false;
				}
				if (_affixes[a_idx].slot == AffixSlot::kSuffix) {
					return false;
				}
				if (a_exclude && std::find(a_exclude->begin(), a_exclude->end(), a_idx) != a_exclude->end()) {
					return false;
				}
				return _affixes[a_idx].EffectiveLootWeight() > 0.0f;
			},
			[&](std::size_t a_idx) {
				if (a_idx >= _affixes.size()) {
					return 0.0f;
				}
				return _affixes[a_idx].EffectiveLootWeight();
			});
		return picked;
	}

	std::optional<std::size_t> EventBridge::RollSuffixIndex(
		LootItemType a_itemType,
		const std::vector<std::string>* a_excludeFamilies)
	{
		const std::vector<std::size_t>* sourcePool = nullptr;
		if (_loot.sharedPool) {
			sourcePool = &_affixRegistry.lootSharedSuffixes;
		} else {
			sourcePool = (a_itemType == LootItemType::kWeapon) ? &_affixRegistry.lootWeaponSuffixes : &_affixRegistry.lootArmorSuffixes;
		}

		LootShuffleBagState& bagState = _loot.sharedPool ?
			_lootState.suffixSharedBag :
			((a_itemType == LootItemType::kWeapon) ? _lootState.suffixWeaponBag : _lootState.suffixArmorBag);
		const auto picked = detail::SelectWeightedEligibleLootIndexWithShuffleBag(
			_rng,
			*sourcePool,
			bagState.order,
			bagState.cursor,
			[&](std::size_t a_idx) {
				if (a_idx >= _affixes.size()) {
					return false;
				}
				const auto& affix = _affixes[a_idx];
				if (affix.slot != AffixSlot::kSuffix || affix.EffectiveLootWeight() <= 0.0f) {
					return false;
				}
				if (a_excludeFamilies && !affix.family.empty()) {
					if (std::find(a_excludeFamilies->begin(), a_excludeFamilies->end(), affix.family) != a_excludeFamilies->end()) {
						return false;
					}
				}
				return true;
			},
			[&](std::size_t a_idx) {
				if (a_idx >= _affixes.size()) {
					return 0.0f;
				}
				return _affixes[a_idx].EffectiveLootWeight();
			});
		return picked;
	}

	std::uint8_t EventBridge::RollAffixCount()
	{
		std::array<double, kMaxRegularAffixesPerItem> weights{};
		bool hasPositiveWeight = false;
		for (std::size_t i = 0; i < kAffixCountWeights.size(); ++i) {
			const double weight = std::max(0.0, static_cast<double>(kAffixCountWeights[i]));
			weights[i] = weight;
			hasPositiveWeight = hasPositiveWeight || (weight > 0.0);
		}
		if (!hasPositiveWeight) {
			return 1u;
		}

		std::discrete_distribution<unsigned int> dist(weights.begin(), weights.end());
		return static_cast<std::uint8_t>(dist(_rng) + 1);
	}

	bool EventBridge::RollLootChanceGateForEligibleInstance()
	{
		const float chancePct = std::clamp(_loot.chancePercent, 0.0f, 100.0f);
		if (chancePct <= 0.0f) {
			_lootState.lootChanceEligibleFailStreak = 0;
			return false;
		}

		if (_lootState.lootChanceEligibleFailStreak >= kLootChancePityFailThreshold) {
			_lootState.lootChanceEligibleFailStreak = 0;
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: loot chance pity triggered (threshold={} failures).",
					kLootChancePityFailThreshold);
			}
			return true;
		}

		if (chancePct >= 100.0f) {
			_lootState.lootChanceEligibleFailStreak = 0;
			return true;
		}

		std::uniform_real_distribution<float> dist(0.0f, 100.0f);
		if (dist(_rng) < chancePct) {
			_lootState.lootChanceEligibleFailStreak = 0;
			return true;
		}

		if (_lootState.lootChanceEligibleFailStreak < std::numeric_limits<std::uint32_t>::max()) {
			++_lootState.lootChanceEligibleFailStreak;
		}
		return false;
	}

	void EventBridge::ProcessLootAcquired(
		RE::FormID a_baseObj,
		std::int32_t a_count,
		std::uint16_t a_uniqueID,
		RE::FormID a_oldContainer,
		bool a_allowRunewordFragmentRoll)
	{
		if (!_configLoaded || !_runtimeSettings.enabled) {
			return;
		}

		if (!a_allowRunewordFragmentRoll || a_count <= 0) {
			return;
		}
		if (!IsRuntimeCurrencyDropRollEnabled("pickup")) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: pickup currency roll context (baseObj={:08X}, uniqueID={}, oldContainer={:08X}).",
					a_baseObj,
					a_uniqueID,
					a_oldContainer);
			}
			return;
		}

		auto* baseForm = RE::TESForm::LookupByID<RE::TESForm>(a_baseObj);
		auto* baseObj = baseForm ? baseForm->As<RE::TESBoundObject>() : nullptr;
		if (!baseObj) {
			return;
		}

		if (IsCalamityResourceCurrencyItem(baseObj)) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: pickup loot roll skipped for resource currency item (baseObj={:08X}, uniqueID={}, oldContainer={:08X}).",
					a_baseObj,
					a_uniqueID,
					a_oldContainer);
			}
			return;
		}

		const auto sourceTier = detail::ResolveLootCurrencySourceTier(
			a_oldContainer,
			_loot.bossContainerEditorIdAllowContains,
			_loot.bossContainerEditorIdDenyContains,
			LootRerollGuard::kWorldContainer);

		if (sourceTier == LootCurrencySourceTier::kWorld &&
			!RuntimePolicy::kAllowWorldPickupCurrencyRoll) {
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: pickup currency roll skipped (world pickup disabled) (baseObj={:08X}, uniqueID={}, oldContainer={:08X}).",
					a_baseObj,
					a_uniqueID,
					a_oldContainer);
			}
			return;
		}

		if (!RuntimePolicy::kAllowPickupCurrencyRollFromContainerSources) {
			if (_loot.debugLog && sourceTier != LootCurrencySourceTier::kUnknown) {
				SKSE::log::debug(
					"CalamityAffixes: pickup currency roll skipped (activation-only source policy) (baseObj={:08X}, sourceTier={}, uniqueID={}, oldContainer={:08X}).",
					a_baseObj,
					detail::LootCurrencySourceTierLabel(sourceTier),
					a_uniqueID,
					a_oldContainer);
			}
			return;
		}
	}

	EventBridge::CurrencyRollExecutionResult EventBridge::ExecuteCurrencyDropRolls(
		float a_sourceChanceMultiplier,
		RE::TESObjectREFR* a_sourceContainerRef,
		bool a_forceWorldPlacement,
		std::uint32_t a_rollCount,
		bool a_allowRunewordRoll,
		bool a_allowReforgeRoll)
	{
		CurrencyRollExecutionResult result{};
		constexpr auto kDropNotificationCooldown = std::chrono::milliseconds(2500);
		static auto s_lastRunewordDropNotificationAt = std::chrono::steady_clock::time_point{};
		static auto s_lastReforgeDropNotificationAt = std::chrono::steady_clock::time_point{};
		const auto now = std::chrono::steady_clock::now();
		const auto shouldEmitDropNotification =
			[&](std::chrono::steady_clock::time_point& a_lastNotificationAt) {
				if (a_lastNotificationAt.time_since_epoch().count() == 0) {
					a_lastNotificationAt = now;
					return true;
				}
				if ((now - a_lastNotificationAt) < kDropNotificationCooldown) {
					return false;
				}
				a_lastNotificationAt = now;
				return true;
			};

		const auto rollCount = std::max<std::uint32_t>(1u, a_rollCount);
		for (std::uint32_t i = 0; i < rollCount; ++i) {
			if (a_allowRunewordRoll) {
				bool runewordPityTriggered = false;
				std::uint64_t runeToken = 0u;
				if (TryRollRunewordFragmentToken(a_sourceChanceMultiplier, runeToken, runewordPityTriggered)) {
					auto* fragmentItem = RunewordDetail::LookupRunewordFragmentItem(
						_runewordState.runeNameByToken,
						runeToken);
					if (!fragmentItem) {
						SKSE::log::error(
							"CalamityAffixes: runeword fragment item missing (runeToken={:016X}).",
							runeToken);
					} else if (TryPlaceLootCurrencyItem(fragmentItem, a_sourceContainerRef, a_forceWorldPlacement)) {
						CommitRunewordFragmentGrant(true);
						result.runewordDropGranted = true;
						std::string runeName = "Rune";
						if (const auto nameIt = _runewordState.runeNameByToken.find(runeToken);
							nameIt != _runewordState.runeNameByToken.end()) {
							runeName = nameIt->second;
						}
						std::string note = "Runeword Fragment Drop: ";
						note.append(runeName);
						if (runewordPityTriggered) {
							note.append(" [Pity]");
						}
						if (shouldEmitDropNotification(s_lastRunewordDropNotificationAt)) {
							RE::DebugNotification(note.c_str());
						}
					}
				}
			}

			if (a_allowReforgeRoll) {
				bool reforgePityTriggered = false;
				if (TryRollReforgeOrbGrant(a_sourceChanceMultiplier, reforgePityTriggered)) {
					auto* orb = RE::TESForm::LookupByEditorID<RE::TESObjectMISC>("CAFF_Misc_ReforgeOrb");
					if (!orb) {
						SKSE::log::error(
							"CalamityAffixes: reforge orb item missing (editorId=CAFF_Misc_ReforgeOrb).");
					} else if (TryPlaceLootCurrencyItem(orb, a_sourceContainerRef, a_forceWorldPlacement)) {
						CommitReforgeOrbGrant(true);
						result.reforgeDropGranted = true;
						if (shouldEmitDropNotification(s_lastReforgeDropNotificationAt)) {
							RE::DebugNotification("Reforge Orb Drop");
						}
						if (reforgePityTriggered && _loot.debugLog) {
							SKSE::log::debug("CalamityAffixes: reforge orb pity triggered.");
						}
					}
				}
			}
		}

		return result;
	}
}
