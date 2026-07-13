#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/RunewordUiPolicy.h"
#include "CalamityAffixes/RunewordUtil.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

	namespace
	{
		[[nodiscard]] detail::RunewordBaseKind ResolveRunewordBaseKind(RE::TESBoundObject* a_object) noexcept
		{
			if (!a_object) {
				return detail::RunewordBaseKind::kUnknown;
			}
			if (const auto* weapon = a_object->As<RE::TESObjectWEAP>()) {
				return detail::ResolveRunewordWeaponBaseKind(
					static_cast<std::uint8_t>(weapon->GetWeaponType()));
			}
			if (const auto* armor = a_object->As<RE::TESObjectARMO>()) {
				return detail::ResolveRunewordArmorBaseKind(
					armor->IsShield(),
					armor->IsHeavyArmor(),
					armor->IsLightArmor(),
					armor->IsClothing());
			}
			return detail::RunewordBaseKind::kUnknown;
		}

		void PopulateSpecializedBaseWarning(
			RunewordPanelState& a_panelState,
			std::string_view a_recipeId,
			RE::TESBoundObject* a_object)
		{
			const auto requirement = detail::ResolveSpecializedRunewordBase(a_recipeId);
			if (requirement == detail::SpecializedRunewordBase::kNone ||
				detail::IsSpecializedRunewordBaseCompatible(
					requirement,
					ResolveRunewordBaseKind(a_object))) {
				return;
			}

			a_panelState.baseCompatibilityWarning = true;
			a_panelState.baseCompatibilityMessageEn =
				"Base mismatch: recommended base is ";
			a_panelState.baseCompatibilityMessageEn.append(
				detail::DescribeSpecializedRunewordBaseEn(requirement));
			a_panelState.baseCompatibilityMessageEn.append(
				". The selected base is incompatible; transmute remains allowed.");

			a_panelState.baseCompatibilityMessageKo =
				"베이스 불일치: 이 룬워드는 ";
			a_panelState.baseCompatibilityMessageKo.append(
				detail::DescribeSpecializedRunewordBaseKo(requirement));
			a_panelState.baseCompatibilityMessageKo.append(
				"에 맞춰져 있습니다. 현재 베이스와 호환되지 않지만 변환은 계속할 수 있습니다.");
		}
	}

	EventBridge::RunewordPanelState EventBridge::GetRunewordPanelState()
	{
		const std::scoped_lock lock(_stateMutex);
		RunewordPanelState panelState{};
		if (!_configLoaded) {
			return panelState;
		}

		SanitizeRunewordState();
		if (!_runewordState.selectedBaseKey) {
			return panelState;
		}
		panelState.hasBase = true;

		// Re-transmutation falls through to the normal recipe selection flow.
		// The old runeword remains active until the replacement commits successfully.

		std::uint32_t inserted = 0u;

		const auto stateIt = _runewordState.instanceStates.find(*_runewordState.selectedBaseKey);
		if (stateIt != _runewordState.instanceStates.end()) {
			inserted = stateIt->second.insertedRunes;
		}

		const auto* recipe = ResolveSelectedRunewordRecipe(*_runewordState.selectedBaseKey);
		if (!recipe || recipe->runeTokens.empty()) {
			return panelState;
		}

		panelState.hasRecipe = true;
		panelState.recipeName = recipe->displayName;
		panelState.totalRunes = static_cast<std::uint32_t>(recipe->runeTokens.size());
		panelState.insertedRunes = std::min(inserted, panelState.totalRunes);

		RE::InventoryEntryData* selectedEntry = nullptr;
		RE::ExtraDataList* selectedXList = nullptr;
		if (ResolvePlayerInventoryInstance(
				*_runewordState.selectedBaseKey,
				selectedEntry,
				selectedXList) &&
			selectedEntry && selectedEntry->object) {
			PopulateSpecializedBaseWarning(panelState, recipe->id, selectedEntry->object);
		}

		const auto applyBlockReason = ResolveRunewordApplyBlockReason(*_runewordState.selectedBaseKey, *recipe);
		const bool canApplyResult = applyBlockReason == RunewordApplyBlockReason::kNone;

		if (panelState.insertedRunes >= panelState.totalRunes) {
			// Legacy: allow finalization only when result can actually be applied.
			panelState.canInsert = CanFinalizeRunewordFromPanel(
				panelState.insertedRunes,
				panelState.totalRunes,
				canApplyResult);
			if (!canApplyResult) {
				panelState.missingSummary = BuildRunewordApplyBlockMessage(applyBlockReason);
			}
			return panelState;
		}

		// Transmute-only UI: require all remaining rune fragments at once.
		const auto requiredCounts = BuildRuneTokenCounts<16>(
			std::span<const std::uint64_t>(recipe->runeTokens.data(), recipe->runeTokens.size()),
			panelState.insertedRunes);

		auto* player = RE::PlayerCharacter::GetSingleton();
		bool ready = true;
		bool firstMissingSet = false;
		std::string missingSummary;

		panelState.requiredRunes.reserve(requiredCounts.size);
		for (std::size_t i = 0; i < requiredCounts.size; ++i) {
			const auto token = requiredCounts.entries[i].token;
			const auto required = requiredCounts.entries[i].count;
			if (token == 0u || required == 0u) {
				continue;
			}

			std::string runeName = "Rune";
			if (const auto nameIt = _runewordState.runeNameByToken.find(token); nameIt != _runewordState.runeNameByToken.end()) {
				runeName = nameIt->second;
			}

			const auto owned = GetOwnedRunewordFragmentCount(player, _runewordState.runeNameByToken, token);
			panelState.requiredRunes.push_back(RunewordRuneRequirement{
				.runeName = runeName,
				.required = required,
				.owned = owned,
			});
			if (owned >= required) {
				continue;
			}

			ready = false;
			const auto missing = required - owned;
			if (!missingSummary.empty()) {
				missingSummary.append(", ");
			}
			missingSummary.append(runeName);
			missingSummary.append(" x");
			missingSummary.append(std::to_string(missing));

			if (!firstMissingSet) {
				firstMissingSet = true;
				panelState.nextRuneName = runeName;
				panelState.nextRuneOwned = owned;
			}
		}

		if (!missingSummary.empty()) {
			panelState.missingSummary = std::move(missingSummary);
		} else if (!canApplyResult) {
			panelState.missingSummary = BuildRunewordApplyBlockMessage(applyBlockReason);
		}

		panelState.canInsert = CanInsertRunewordFromPanel(ready, canApplyResult);
		return panelState;
	}

	std::optional<std::string> EventBridge::GetSelectedRunewordBaseAffixTooltip(int a_uiLanguageMode)
	{
		const std::scoped_lock lock(_stateMutex);
		if (!_configLoaded) {
			return std::nullopt;
		}

		SanitizeRunewordState();
		if (!_runewordState.selectedBaseKey) {
			return std::nullopt;
		}

		RE::InventoryEntryData* entry = nullptr;
		RE::ExtraDataList* xList = nullptr;
		if (!ResolvePlayerInventoryInstance(*_runewordState.selectedBaseKey, entry, xList) || !entry || !entry->object || !xList) {
			return std::nullopt;
		}

		const std::string selectedName = ResolveInventoryDisplayName(entry, xList);
		return GetInstanceAffixTooltip(
			entry,
			selectedName,
			a_uiLanguageMode,
			"inventory",
			0u,
			*_runewordState.selectedBaseKey);
	}

}
