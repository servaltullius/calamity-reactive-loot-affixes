#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootRollSelection.h"
#include "CalamityAffixes/SerializationLoadState.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace CalamityAffixes
{
	namespace
	{
		constexpr std::uint32_t kMaxDrainBytes = 10'000'000u;
		constexpr std::uint32_t kMaxV1AffixIdLength = 1024u;
		constexpr std::uint32_t kMaxShuffleBagSize = 100'000u;

		bool DrainRecordBytes(
			SKSE::SerializationInterface* a_intfc,
			std::uint32_t a_length,
			const char* a_context)
		{
			if (!a_intfc || a_length == 0u) {
				return true;
			}

			if (a_length > kMaxDrainBytes) {
				SKSE::log::warn(
					"CalamityAffixes: draining unusually large serialization record segment (context={}, bytes={}).",
					a_context,
					a_length);
			}

			std::array<std::uint8_t, 4096> sink{};
			std::uint32_t remaining = a_length;
			while (remaining > 0u) {
				const auto chunk = std::min<std::uint32_t>(remaining, static_cast<std::uint32_t>(sink.size()));
				const auto read = a_intfc->ReadRecordData(sink.data(), chunk);
				if (read != chunk) {
					SKSE::log::warn(
						"CalamityAffixes: truncated serialization drain (context={}, requested={}, read={}).",
						a_context,
						chunk,
						read);
					return false;
				}
				remaining -= chunk;
			}

			return true;
		}
	}

#include "EventBridge.Serialization.Load.Records.inl"

	void EventBridge::Load(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) {
			return;
		}

		const std::scoped_lock lock(_stateMutex);
		SKSE::log::info("CalamityAffixes: Load() — deserializing co-save records.");

		_instanceAffixes.clear();
		_equippedInstanceKeysByToken.clear();
		_equippedTokenCacheReady = false;
		_lootState.ResetForLoadOrRevert();
		_instanceStates.clear();
		_activeCounts.clear();
		_activeCritDamageBonusPct = 0.0f;
		_activeHitTriggerAffixIndices.clear();
		_activeIncomingHitTriggerAffixIndices.clear();
		_activeDotApplyTriggerAffixIndices.clear();
		_activeKillTriggerAffixIndices.clear();
		_activeLowHealthTriggerAffixIndices.clear();
		_combatState.ResetTransientState();
		_trapState.Reset();
		_runewordState.ResetSelectionAndProgress();
		_corpseExplosionSeenCorpses.clear();
		_corpseExplosionState = {};
		_summonCorpseExplosionSeenCorpses.clear();
		_summonCorpseExplosionState = {};

		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;

		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			switch (type) {
			case kSerializationRecordInstanceAffixes:
				LoadInstanceAffixesRecord(a_intfc, version, length);
				break;
			case kSerializationRecordInstanceRuntimeStates:
				LoadInstanceRuntimeStatesRecord(a_intfc, version, length);
				break;
			case kSerializationRecordRunewordState:
				LoadRunewordStateRecord(a_intfc, version, length);
				break;
			case kSerializationRecordLootEvaluated:
				LoadLootEvaluatedRecord(a_intfc, version, length);
				break;
			case kSerializationRecordLootCurrencyLedger:
				LoadLootCurrencyLedgerRecord(a_intfc, version, length);
				break;
			case kSerializationRecordLootShuffleBags:
				LoadLootShuffleBagsRecord(a_intfc, version, length);
				break;
			case kSerializationRecordMigrationFlags:
				LoadMigrationFlagsRecord(a_intfc, version, length);
				break;
			default:
				DrainRecordBytes(a_intfc, length, "unknown-record");
				break;
			}
		}

		FinalizeLoadedSerializationState();

		// NOTE: MaybeMigrateMiscCurrency() is intentionally NOT called here.
		// During the SKSE Load callback the game engine has not yet restored the
		// player's inventory, so GetItemCount() returns 0 for all items.
		// The migration check is deferred to OnPostLoadGame() (kPostLoadGame message),
		// where the game state is fully loaded.
	}
}
