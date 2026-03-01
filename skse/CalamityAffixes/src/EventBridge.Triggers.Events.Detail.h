#pragma once

#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootCurrencySourceHelpers.h"
#include "CalamityAffixes/PointerSafety.h"

#include <chrono>
#include <cstdint>
#include <limits>
#include <string_view>

namespace CalamityAffixes::TriggersDetail
{
	constexpr std::size_t kDotCooldownMaxEntries = 4096;
	constexpr auto kDotCooldownTtl = std::chrono::seconds(30);
	constexpr auto kDotCooldownPruneInterval = std::chrono::seconds(10);
	constexpr std::size_t kDotObservedMagicEffectsMaxEntries = 4096;
	constexpr std::string_view kRunewordFragmentEditorIdPrefix = "CAFF_RuneFrag_";
	constexpr std::string_view kReforgeOrbEditorId = "CAFF_Misc_ReforgeOrb";
	constexpr std::string_view kRunewordDropListEditorId = "CAFF_LItem_RunewordFragmentDrops";
	constexpr std::string_view kReforgeDropListEditorId = "CAFF_LItem_ReforgeOrbDrops";
	using LootCurrencySourceTier = detail::LootCurrencySourceTier;

	struct CorpseCurrencyInventorySnapshot
	{
		std::uint32_t runewordFragments{ 0u };
		std::uint32_t reforgeOrbs{ 0u };
		std::uint32_t runewordDropLists{ 0u };
		std::uint32_t reforgeDropLists{ 0u };
	};

	struct CorpseCurrencyDropProbe
	{
		bool runewordFragmentRecordFound{ false };
		bool reforgeOrbRecordFound{ false };
		bool runewordDropListFound{ false };
		bool reforgeDropListFound{ false };
		std::int32_t runewordChanceNone{ -1 };
		std::int32_t reforgeChanceNone{ -1 };
	};

	[[nodiscard]] inline std::uint32_t SaturatingAdd(std::uint32_t a_lhs, std::uint32_t a_rhs)
	{
		if (a_rhs > (std::numeric_limits<std::uint32_t>::max)() - a_lhs) {
			return (std::numeric_limits<std::uint32_t>::max)();
		}
		return a_lhs + a_rhs;
	}

	[[nodiscard]] inline CorpseCurrencyInventorySnapshot SnapshotCorpseCurrencyInventory(RE::TESObjectREFR* a_source)
	{
		CorpseCurrencyInventorySnapshot snapshot{};
		if (!a_source) {
			return snapshot;
		}

		auto inventory = a_source->GetInventory();
		for (const auto& [baseObject, entry] : inventory) {
			if (!baseObject) {
				continue;
			}

			const auto count = entry.first;
			if (count <= 0) {
				continue;
			}

			const auto editorId = SafeCStringView(baseObject->GetFormEditorID());
			if (editorId.empty()) {
				continue;
			}

			const auto itemCount = static_cast<std::uint32_t>(count);
			if (editorId == kRunewordDropListEditorId) {
				snapshot.runewordDropLists = SaturatingAdd(snapshot.runewordDropLists, itemCount);
				continue;
			}

			if (editorId == kReforgeDropListEditorId) {
				snapshot.reforgeDropLists = SaturatingAdd(snapshot.reforgeDropLists, itemCount);
				continue;
			}

			if (editorId == kReforgeOrbEditorId) {
				snapshot.reforgeOrbs = SaturatingAdd(snapshot.reforgeOrbs, itemCount);
				continue;
			}

			if (editorId.starts_with(kRunewordFragmentEditorIdPrefix)) {
				snapshot.runewordFragments = SaturatingAdd(snapshot.runewordFragments, itemCount);
			}
		}

		return snapshot;
	}

	[[nodiscard]] inline CorpseCurrencyDropProbe BuildCorpseCurrencyDropProbe(RE::Actor* a_actor)
	{
		CorpseCurrencyDropProbe probe{};
		(void)a_actor;
		probe.runewordFragmentRecordFound =
			RE::TESForm::LookupByEditorID<RE::TESObjectMISC>("CAFF_RuneFrag_El") != nullptr;
		probe.reforgeOrbRecordFound =
			RE::TESForm::LookupByEditorID<RE::TESObjectMISC>(kReforgeOrbEditorId.data()) != nullptr;

		auto* runewordDropList = RE::TESForm::LookupByEditorID<RE::TESLevItem>(kRunewordDropListEditorId.data());
		auto* reforgeDropList = RE::TESForm::LookupByEditorID<RE::TESLevItem>(kReforgeDropListEditorId.data());
		probe.runewordDropListFound = runewordDropList != nullptr;
		probe.reforgeDropListFound = reforgeDropList != nullptr;
		if (runewordDropList) {
			probe.runewordChanceNone = static_cast<std::int32_t>(runewordDropList->GetChanceNone());
		}
		if (reforgeDropList) {
			probe.reforgeChanceNone = static_cast<std::int32_t>(reforgeDropList->GetChanceNone());
		}

		return probe;
	}

	[[nodiscard]] inline bool IsInternalProcSpellSource(RE::FormID a_sourceFormID)
	{
		if (a_sourceFormID == 0u) {
			return false;
		}

		auto* sourceForm = RE::TESForm::LookupByID<RE::TESForm>(a_sourceFormID);
		auto* sourceSpell = sourceForm ? sourceForm->As<RE::SpellItem>() : nullptr;
		if (!sourceSpell) {
			return false;
		}

		const auto editorId = SafeCStringView(sourceSpell->GetFormEditorID());
		return !editorId.empty() && editorId.starts_with("CAFF_");
	}

	[[nodiscard]] inline RE::Actor* ResolveActorFromCombatRef(RE::TESObjectREFR* a_ref)
	{
		a_ref = SanitizeObjectPointer(a_ref);
		if (!a_ref) {
			return nullptr;
		}

		if (auto* actor = a_ref->As<RE::Actor>()) {
			return actor;
		}

		if (auto* proj = a_ref->As<RE::Projectile>()) {
			const auto& rt = proj->GetProjectileRuntimeData();
			if (auto shooterAny = rt.shooter.get(); shooterAny) {
				auto* shooterRef = SanitizeObjectPointer(shooterAny.get());
				if (auto* shooterActor = shooterRef ? shooterRef->As<RE::Actor>() : nullptr) {
					return shooterActor;
				}
			}

			if (rt.actorCause) {
				auto* actorCause = SanitizeObjectPointer(rt.actorCause.get());
				if (actorCause) {
					auto actorAny = actorCause->actor.get();
					auto* actorRef = actorAny ? SanitizeObjectPointer(actorAny.get()) : nullptr;
					if (actorRef) {
						return actorRef;
					}
				}
			}
		}

		return nullptr;
	}
}
