#include "CalamityAffixes/EventBridge.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <limits>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

	bool EventBridge::ConsumeRunewordFragmentRequirement(
		RE::PlayerCharacter* a_player,
		std::uint64_t a_runeToken,
		std::uint32_t a_requiredCount,
		std::vector<ConsumedRunewordFragment>& a_inOutConsumedRunes,
		std::string* a_outRuneName,
		std::string* a_outFailureReason)
	{
		if (a_outRuneName) {
			a_outRuneName->clear();
		}
		if (a_outFailureReason) {
			a_outFailureReason->clear();
		}
		if (!a_player || a_runeToken == 0u || a_requiredCount == 0u) {
			return false;
		}

		std::string runeName = "Rune";
		if (const auto nameIt = _runewordState.runeNameByToken.find(a_runeToken);
			nameIt != _runewordState.runeNameByToken.end()) {
			runeName = nameIt->second;
		}
		if (a_outRuneName) {
			*a_outRuneName = runeName;
		}

		auto* fragmentItem = LookupRunewordFragmentItem(_runewordState.runeNameByToken, a_runeToken);
		if (!fragmentItem) {
			if (a_outFailureReason) {
				*a_outFailureReason = "fragment-item-missing";
			}
			return false;
		}

		const auto ownedBeforeSigned = std::max(0, a_player->GetItemCount(fragmentItem));
		const auto ownedBefore = static_cast<std::uint32_t>(ownedBeforeSigned);
		if (ownedBefore < a_requiredCount) {
			if (a_outFailureReason) {
				*a_outFailureReason = "consume-precheck-owned-too-low";
			}
			return false;
		}

		const auto maxRemoveChunk = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
		std::uint32_t remaining = a_requiredCount;
		while (remaining > 0u) {
			const auto removeChunk = std::min(remaining, maxRemoveChunk);
			a_player->RemoveItem(fragmentItem, static_cast<std::int32_t>(removeChunk), RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			remaining -= removeChunk;
		}

		const auto ownedAfterSigned = std::max(0, a_player->GetItemCount(fragmentItem));
		const auto ownedAfter = static_cast<std::uint32_t>(ownedAfterSigned);
		const auto consumedCount = (ownedBefore > ownedAfter) ? (ownedBefore - ownedAfter) : 0u;
		if (consumedCount > 0u) {
			a_inOutConsumedRunes.push_back(ConsumedRunewordFragment{
				.token = a_runeToken,
				.count = consumedCount,
				.runeName = runeName
			});
		}
		if (consumedCount != a_requiredCount) {
			if (a_outFailureReason) {
				*a_outFailureReason = "consume-postcheck-partial";
			}
			return false;
		}

		return true;
	}

	bool EventBridge::RollbackConsumedRunewordFragments(
		RE::PlayerCharacter* a_player,
		const std::vector<ConsumedRunewordFragment>& a_consumedRunes,
		std::string_view a_reasonTag)
	{
		if (!a_player) {
			return false;
		}

		bool rollbackFullySucceeded = true;
		for (const auto& consumed : a_consumedRunes) {
			if (consumed.count == 0u) {
				continue;
			}

			auto restored = GrantRunewordFragments(
				a_player,
				_runewordState.runeNameByToken,
				consumed.token,
				consumed.count);
			if (restored < consumed.count) {
				restored += GrantRunewordFragments(
					a_player,
					_runewordState.runeNameByToken,
					consumed.token,
					consumed.count - restored);
			}
			if (restored == consumed.count) {
				continue;
			}

			rollbackFullySucceeded = false;
			const auto missing = consumed.count - std::min(consumed.count, restored);
			if (missing > 0u) {
				if (auto* fragmentItem = LookupRunewordFragmentItem(_runewordState.runeNameByToken, consumed.token)) {
					for (std::uint32_t i = 0; i < missing; ++i) {
						if (!a_player->PlaceObjectAtMe(fragmentItem, false)) {
							SKSE::log::error(
								"CalamityAffixes: runeword rollback world placement failed (reason={}, runeToken={:016X}, runeName={}).",
								a_reasonTag,
								consumed.token,
								consumed.runeName);
							break;
						}
					}
				}
			}
			SKSE::log::error(
				"CalamityAffixes: runeword rollback partial (reason={}, runeToken={:016X}, runeName={}, restored={}, expected={}).",
				a_reasonTag,
				consumed.token,
				consumed.runeName,
				restored,
				consumed.count);
		}

		return rollbackFullySucceeded;
	}
}
