#include "CalamityAffixes/EventBridge.h"

#include <array>
#include <cstdint>

namespace CalamityAffixes
{
	void EventBridge::Save(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) {
			return;
		}

		const std::scoped_lock lock(_stateMutex);

		MaybeFlushRuntimeUserSettings(std::chrono::steady_clock::now(), true);
		PruneLootEvaluatedInstances();

		// --- IAXF v7: fixed 4-token slots ---
		const std::uint32_t count = static_cast<std::uint32_t>(_instanceAffixes.size());
		if (!a_intfc->OpenRecord(kSerializationRecordInstanceAffixes, kSerializationVersion)) {
			return;
		}
		if (!a_intfc->WriteRecordData(count)) {
			return;
		}

		for (const auto& [key, slots] : _instanceAffixes) {
			const auto baseID = static_cast<RE::FormID>(key >> 16);
			const auto uniqueID = static_cast<std::uint16_t>(key & 0xFFFFu);

			if (!a_intfc->WriteRecordData(baseID)) {
				return;
			}
			if (!a_intfc->WriteRecordData(uniqueID)) {
				return;
			}
			if (!a_intfc->WriteRecordData(slots.count)) {
				return;
			}
			for (std::size_t s = 0; s < kMaxAffixesPerItem; ++s) {
				if (!a_intfc->WriteRecordData(slots.tokens[s])) {
					return;
				}
			}
		}

		// --- IRST ---
		if (!a_intfc->OpenRecord(
				kSerializationRecordInstanceRuntimeStates,
				kInstanceRuntimeStateSerializationVersion)) {
			return;
		}

		const std::uint32_t runtimeStateCount = static_cast<std::uint32_t>(_instanceStates.size());
		if (!a_intfc->WriteRecordData(runtimeStateCount)) {
			return;
		}
		for (const auto& [stateKey, state] : _instanceStates) {
			const auto baseID = static_cast<RE::FormID>(stateKey.instanceKey >> 16);
			const auto uniqueID = static_cast<std::uint16_t>(stateKey.instanceKey & 0xFFFFu);
			if (!a_intfc->WriteRecordData(baseID)) {
				return;
			}
			if (!a_intfc->WriteRecordData(uniqueID)) {
				return;
			}
			if (!a_intfc->WriteRecordData(stateKey.affixToken)) {
				return;
			}
			if (!a_intfc->WriteRecordData(state.evolutionXp)) {
				return;
			}
			if (!a_intfc->WriteRecordData(state.modeCycleCounter)) {
				return;
			}
			if (!a_intfc->WriteRecordData(state.modeIndex)) {
				return;
			}
		}

		// --- RWRD ---
		if (!a_intfc->OpenRecord(kSerializationRecordRunewordState, kRunewordSerializationVersion)) {
			return;
		}

		RE::FormID selectedBaseID = 0;
		std::uint16_t selectedUniqueID = 0;
		if (_runewordState.selectedBaseKey) {
			selectedBaseID = static_cast<RE::FormID>(*_runewordState.selectedBaseKey >> 16);
			selectedUniqueID = static_cast<std::uint16_t>(*_runewordState.selectedBaseKey & 0xFFFFu);
		}

		if (!a_intfc->WriteRecordData(selectedBaseID)) {
			return;
		}
		if (!a_intfc->WriteRecordData(selectedUniqueID)) {
			return;
		}
		if (!a_intfc->WriteRecordData(_runewordState.recipeCycleCursor)) {
			return;
		}
		if (!a_intfc->WriteRecordData(_runewordState.baseCycleCursor)) {
			return;
		}

		const std::uint32_t fragmentCount = static_cast<std::uint32_t>(_runewordState.runeFragments.size());
		if (!a_intfc->WriteRecordData(fragmentCount)) {
			return;
		}
		for (const auto& [runeToken, amount] : _runewordState.runeFragments) {
			if (!a_intfc->WriteRecordData(runeToken)) {
				return;
			}
			if (!a_intfc->WriteRecordData(amount)) {
				return;
			}
		}

		const std::uint32_t runewordStateCount = static_cast<std::uint32_t>(_runewordState.instanceStates.size());
		if (!a_intfc->WriteRecordData(runewordStateCount)) {
			return;
		}
		for (const auto& [instanceKey, state] : _runewordState.instanceStates) {
			const auto baseID = static_cast<RE::FormID>(instanceKey >> 16);
			const auto uniqueID = static_cast<std::uint16_t>(instanceKey & 0xFFFFu);
			if (!a_intfc->WriteRecordData(baseID)) {
				return;
			}
			if (!a_intfc->WriteRecordData(uniqueID)) {
				return;
			}
			if (!a_intfc->WriteRecordData(state.recipeToken)) {
				return;
			}
			if (!a_intfc->WriteRecordData(state.insertedRunes)) {
				return;
			}
		}

		// --- LRLD ---
		if (!a_intfc->OpenRecord(kSerializationRecordLootEvaluated, kLootEvaluatedSerializationVersion)) {
			return;
		}

		const std::uint32_t lootEvaluatedCount = static_cast<std::uint32_t>(_lootState.evaluatedInstances.size());
		if (!a_intfc->WriteRecordData(lootEvaluatedCount)) {
			return;
		}
		for (const auto key : _lootState.evaluatedInstances) {
			const auto baseID = static_cast<RE::FormID>(key >> 16);
			const auto uniqueID = static_cast<std::uint16_t>(key & 0xFFFFu);
			if (!a_intfc->WriteRecordData(baseID)) {
				return;
			}
			if (!a_intfc->WriteRecordData(uniqueID)) {
				return;
			}
		}

		// --- LCLD ---
		if (!a_intfc->OpenRecord(
				kSerializationRecordLootCurrencyLedger,
				kLootCurrencyLedgerSerializationVersion)) {
			return;
		}

		const std::uint32_t lootCurrencyLedgerCount = static_cast<std::uint32_t>(_lootState.currencyRollLedger.size());
		if (!a_intfc->WriteRecordData(lootCurrencyLedgerCount)) {
			return;
		}
		for (const auto& [key, dayStamp] : _lootState.currencyRollLedger) {
			if (!a_intfc->WriteRecordData(key)) {
				return;
			}
			if (!a_intfc->WriteRecordData(dayStamp)) {
				return;
			}
		}

		// --- LSBG ---
		if (!a_intfc->OpenRecord(kSerializationRecordLootShuffleBags, kLootShuffleBagSerializationVersion)) {
			return;
		}

		auto writeBag = [&](std::uint8_t a_id, const LootShuffleBagState& a_bag) -> bool {
			const auto bagSize = static_cast<std::uint32_t>(a_bag.order.size());
			const auto bagCursor = static_cast<std::uint32_t>(std::min<std::size_t>(a_bag.cursor, a_bag.order.size()));
			if (!a_intfc->WriteRecordData(a_id)) {
				return false;
			}
			if (!a_intfc->WriteRecordData(bagCursor)) {
				return false;
			}
			if (!a_intfc->WriteRecordData(bagSize)) {
				return false;
			}
			for (const auto idx : a_bag.order) {
				const auto raw = static_cast<std::uint32_t>(idx);
				if (!a_intfc->WriteRecordData(raw)) {
					return false;
				}
			}
			return true;
		};

		const std::array<std::pair<std::uint8_t, const LootShuffleBagState*>, 6> kBags{ {
			{ 0u, &_lootState.prefixSharedBag },
			{ 1u, &_lootState.prefixWeaponBag },
			{ 2u, &_lootState.prefixArmorBag },
			{ 3u, &_lootState.suffixSharedBag },
			{ 4u, &_lootState.suffixWeaponBag },
			{ 5u, &_lootState.suffixArmorBag },
		} };
		const auto bagCount = static_cast<std::uint8_t>(kBags.size());
		if (!a_intfc->WriteRecordData(bagCount)) {
			return;
		}
		for (const auto& [id, bag] : kBags) {
			if (!bag || !writeBag(id, *bag)) {
				return;
			}
		}

		// --- MFLG ---
		if (!a_intfc->OpenRecord(kSerializationRecordMigrationFlags, kMigrationFlagsVersion)) {
			return;
		}
		const std::uint8_t migrationFlags = (_miscCurrencyMigrated ? 1u : 0u)
			| (_miscCurrencyRecovered ? 2u : 0u);
		if (!a_intfc->WriteRecordData(migrationFlags)) {
			return;
		}
	}
}
