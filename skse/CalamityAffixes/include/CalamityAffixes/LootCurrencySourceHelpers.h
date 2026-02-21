#pragma once

#include "CalamityAffixes/LootCurrencyLedger.h"
#include "CalamityAffixes/LootEligibility.h"

#include <RE/Skyrim.h>

#include <cmath>
#include <limits>
#include <string_view>
#include <vector>

namespace CalamityAffixes::detail
{
	[[nodiscard]] inline bool IsBossContainerEditorId(
		std::string_view a_editorId,
		const std::vector<std::string>& a_allowContains,
		const std::vector<std::string>& a_denyContains)
	{
		if (a_editorId.empty()) {
			return false;
		}

		if (MatchesAnyCaseInsensitiveMarker(a_editorId, a_denyContains)) {
			return false;
		}

		if (!a_allowContains.empty()) {
			return MatchesAnyCaseInsensitiveMarker(a_editorId, a_allowContains);
		}

		// Fallback heuristic when allow-list is intentionally empty.
		return ContainsCaseInsensitiveAscii(a_editorId, "boss");
	}

	[[nodiscard]] inline LootCurrencySourceTier ResolveLootCurrencySourceTier(
		const RE::TESObjectREFR* a_ref,
		const std::vector<std::string>& a_bossAllowContains,
		const std::vector<std::string>& a_bossDenyContains)
	{
		if (!a_ref) {
			return LootCurrencySourceTier::kUnknown;
		}

		if (const auto* actor = a_ref->As<RE::Actor>(); actor && actor->IsDead()) {
			return LootCurrencySourceTier::kCorpse;
		}

		const RE::TESForm* sourceForm = a_ref;
		const RE::TESForm* sourceBase = a_ref->GetBaseObject();
		if (!sourceBase || !sourceBase->As<RE::TESObjectCONT>()) {
			return LootCurrencySourceTier::kUnknown;
		}

		const char* sourceEditorIdRaw = sourceForm->GetFormEditorID();
		const char* baseEditorIdRaw = sourceBase->GetFormEditorID();
		if (IsBossContainerEditorId(
				sourceEditorIdRaw ? std::string_view(sourceEditorIdRaw) : std::string_view{},
				a_bossAllowContains,
				a_bossDenyContains) ||
			IsBossContainerEditorId(
				baseEditorIdRaw ? std::string_view(baseEditorIdRaw) : std::string_view{},
				a_bossAllowContains,
				a_bossDenyContains)) {
			return LootCurrencySourceTier::kBossContainer;
		}

		return LootCurrencySourceTier::kContainer;
	}

	[[nodiscard]] inline LootCurrencySourceTier ResolveLootCurrencySourceTier(
		RE::FormID a_oldContainer,
		const std::vector<std::string>& a_bossAllowContains,
		const std::vector<std::string>& a_bossDenyContains,
		RE::FormID a_worldContainerFormId)
	{
		if (a_oldContainer == 0u) {
			return LootCurrencySourceTier::kUnknown;
		}
		if (a_oldContainer == a_worldContainerFormId) {
			return LootCurrencySourceTier::kWorld;
		}

		auto* sourceForm = RE::TESForm::LookupByID<RE::TESForm>(a_oldContainer);
		if (!sourceForm) {
			return LootCurrencySourceTier::kUnknown;
		}

		if (auto* sourceActor = sourceForm->As<RE::Actor>(); sourceActor && sourceActor->IsDead()) {
			return LootCurrencySourceTier::kCorpse;
		}

		auto isBossContainer = [&](const RE::TESForm* a_form) {
			if (!a_form) {
				return false;
			}
			const char* editorIdRaw = a_form->GetFormEditorID();
			return editorIdRaw && IsBossContainerEditorId(editorIdRaw, a_bossAllowContains, a_bossDenyContains);
		};

		if (auto* sourceRef = sourceForm->As<RE::TESObjectREFR>()) {
			if (auto* sourceBase = sourceRef->GetBaseObject(); sourceBase && sourceBase->As<RE::TESObjectCONT>()) {
				if (isBossContainer(sourceRef) || isBossContainer(sourceBase)) {
					return LootCurrencySourceTier::kBossContainer;
				}
				return LootCurrencySourceTier::kContainer;
			}
		}

		if (sourceForm->As<RE::TESObjectCONT>()) {
			return isBossContainer(sourceForm) ? LootCurrencySourceTier::kBossContainer : LootCurrencySourceTier::kContainer;
		}

		return LootCurrencySourceTier::kUnknown;
	}

	[[nodiscard]] inline std::string_view LootCurrencySourceTierLabel(LootCurrencySourceTier a_tier)
	{
		switch (a_tier) {
		case LootCurrencySourceTier::kCorpse:
			return "corpse";
		case LootCurrencySourceTier::kContainer:
			return "container";
		case LootCurrencySourceTier::kBossContainer:
			return "boss_container";
		case LootCurrencySourceTier::kWorld:
			return "world";
		default:
			return "unknown";
		}
	}

	[[nodiscard]] inline std::uint32_t GetInGameDayStamp() noexcept
	{
		auto* calendar = RE::Calendar::GetSingleton();
		if (!calendar) {
			return 0u;
		}

		const float daysPassed = calendar->GetDaysPassed();
		if (!std::isfinite(daysPassed) || daysPassed <= 0.0f) {
			return 0u;
		}

		const auto dayFloor = std::floor(daysPassed);
		if (dayFloor >= static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
			return std::numeric_limits<std::uint32_t>::max();
		}

		return static_cast<std::uint32_t>(dayFloor);
	}
}
