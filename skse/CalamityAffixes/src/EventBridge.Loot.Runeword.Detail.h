#pragma once

#include "CalamityAffixes/EventBridge.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace CalamityAffixes::RunewordDetail
{
	[[nodiscard]] std::string ToLowerAscii(std::string_view a_text);
	[[nodiscard]] bool IsWeaponOrArmor(const RE::TESBoundObject* a_object);
	[[nodiscard]] std::string ResolveInventoryDisplayName(
		const RE::InventoryEntryData* a_entry,
		RE::ExtraDataList* a_xList);
	[[nodiscard]] double ResolveRunewordFragmentWeight(std::string_view a_runeName);
	[[nodiscard]] RE::TESObjectMISC* LookupRunewordFragmentItem(
		const std::unordered_map<std::uint64_t, std::string>& a_runeNameByToken,
		std::uint64_t a_runeToken);
	[[nodiscard]] std::uint32_t GetOwnedRunewordFragmentCount(
		RE::PlayerCharacter* a_player,
		const std::unordered_map<std::uint64_t, std::string>& a_runeNameByToken,
		std::uint64_t a_runeToken);
	[[nodiscard]] std::uint32_t GrantRunewordFragments(
		RE::PlayerCharacter* a_player,
		const std::unordered_map<std::uint64_t, std::string>& a_runeNameByToken,
		std::uint64_t a_runeToken,
		std::uint32_t a_amount);
	[[nodiscard]] bool TryConsumeRunewordFragment(
		RE::PlayerCharacter* a_player,
		const std::unordered_map<std::uint64_t, std::string>& a_runeNameByToken,
		std::uint64_t a_runeToken);
}
