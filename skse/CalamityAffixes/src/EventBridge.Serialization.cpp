#include "CalamityAffixes/EventBridge.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace CalamityAffixes
{
	void EventBridge::Save(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) {
			return;
		}

		PruneLootEvaluatedInstances();

		// --- IAXF v6: fixed 3-token slots ---
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
		if (_runewordSelectedBaseKey) {
			selectedBaseID = static_cast<RE::FormID>(*_runewordSelectedBaseKey >> 16);
			selectedUniqueID = static_cast<std::uint16_t>(*_runewordSelectedBaseKey & 0xFFFFu);
		}

		if (!a_intfc->WriteRecordData(selectedBaseID)) {
			return;
		}
		if (!a_intfc->WriteRecordData(selectedUniqueID)) {
			return;
		}
		if (!a_intfc->WriteRecordData(_runewordRecipeCycleCursor)) {
			return;
		}
		if (!a_intfc->WriteRecordData(_runewordBaseCycleCursor)) {
			return;
		}

		const std::uint32_t fragmentCount = static_cast<std::uint32_t>(_runewordRuneFragments.size());
		if (!a_intfc->WriteRecordData(fragmentCount)) {
			return;
		}
		for (const auto& [runeToken, amount] : _runewordRuneFragments) {
			if (!a_intfc->WriteRecordData(runeToken)) {
				return;
			}
			if (!a_intfc->WriteRecordData(amount)) {
				return;
			}
		}

		const std::uint32_t runewordStateCount = static_cast<std::uint32_t>(_runewordInstanceStates.size());
		if (!a_intfc->WriteRecordData(runewordStateCount)) {
			return;
		}
		for (const auto& [instanceKey, state] : _runewordInstanceStates) {
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

		const std::uint32_t lootEvaluatedCount = static_cast<std::uint32_t>(_lootEvaluatedInstances.size());
		if (!a_intfc->WriteRecordData(lootEvaluatedCount)) {
			return;
		}
		for (const auto key : _lootEvaluatedInstances) {
			const auto baseID = static_cast<RE::FormID>(key >> 16);
			const auto uniqueID = static_cast<std::uint16_t>(key & 0xFFFFu);
			if (!a_intfc->WriteRecordData(baseID)) {
				return;
			}
			if (!a_intfc->WriteRecordData(uniqueID)) {
				return;
			}
		}
	}

		void EventBridge::Load(SKSE::SerializationInterface* a_intfc)
		{
			if (!a_intfc) {
				return;
			}

			_instanceAffixes.clear();
			_equippedInstanceKeysByToken.clear();
			_equippedTokenCacheReady = false;
			_lootEvaluatedInstances.clear();
			_lootEvaluatedRecent.clear();
			_lootEvaluatedInsertionsSincePrune = 0;
			_instanceStates.clear();
			_dotObservedMagicEffects.clear();
			_dotTagSafetyWarned = false;
			_dotTagSafetySuppressed = false;
			_runewordRuneFragments.clear();
			_runewordInstanceStates.clear();
			_runewordSelectedBaseKey.reset();
			_runewordBaseCycleCursor = 0;
			_runewordRecipeCycleCursor = 0;

			std::uint32_t type = 0;
			std::uint32_t version = 0;
			std::uint32_t length = 0;

			while (a_intfc->GetNextRecordInfo(type, version, length)) {
				if (type == kSerializationRecordInstanceAffixes) {
					if (version != kSerializationVersion &&
						version != kSerializationVersionV5 &&
						version != kSerializationVersionV4 &&
						version != kSerializationVersionV3 &&
						version != kSerializationVersionV2 &&
						version != kSerializationVersionV1) {
						// Drain unsupported version.
						std::vector<std::uint8_t> sink(length);
						if (length > 0) {
							a_intfc->ReadRecordData(sink.data(), length);
						}
						continue;
					}

					if (version == kSerializationVersion) {
						// --- v6 load: fixed 3-token slots ---
						std::uint32_t count = 0;
						if (a_intfc->ReadRecordData(count) != sizeof(count)) {
							return;
						}

						for (std::uint32_t i = 0; i < count; ++i) {
							RE::FormID baseID = 0;
							std::uint16_t uniqueID = 0;
							std::uint8_t affixCount = 0;
							std::array<std::uint64_t, kMaxAffixesPerItem> tokens{};

							if (a_intfc->ReadRecordData(baseID) != sizeof(baseID)) {
								return;
							}
							if (a_intfc->ReadRecordData(uniqueID) != sizeof(uniqueID)) {
								return;
							}
							if (a_intfc->ReadRecordData(affixCount) != sizeof(affixCount)) {
								return;
							}
							for (std::size_t s = 0; s < kMaxAffixesPerItem; ++s) {
								if (a_intfc->ReadRecordData(tokens[s]) != sizeof(tokens[s])) {
									return;
								}
							}

							RE::FormID resolvedBaseID = 0;
							if (!a_intfc->ResolveFormID(baseID, resolvedBaseID)) {
								continue;
							}

							const auto key = MakeInstanceKey(resolvedBaseID, uniqueID);
							InstanceAffixSlots slots;
							slots.count = std::min<std::uint8_t>(affixCount, static_cast<std::uint8_t>(kMaxAffixesPerItem));
							slots.tokens = tokens;
							_instanceAffixes.emplace(key, slots);
						}

						continue;
					}

					// --- v1~v5 legacy load ---
					std::uint32_t count = 0;
					if (a_intfc->ReadRecordData(count) != sizeof(count)) {
						return;
					}

					for (std::uint32_t i = 0; i < count; i++) {
						RE::FormID baseID = 0;
						std::uint16_t uniqueID = 0;

						if (a_intfc->ReadRecordData(baseID) != sizeof(baseID)) {
							return;
						}
						if (a_intfc->ReadRecordData(uniqueID) != sizeof(uniqueID)) {
							return;
						}

							std::uint64_t token = 0;
							std::uint64_t supplementalToken = 0;
							InstanceRuntimeState state{};
						if (version == kSerializationVersionV1) {
							std::uint32_t len = 0;
							if (a_intfc->ReadRecordData(len) != sizeof(len)) {
								return;
							}

							std::string affixId;
							affixId.resize(len);
							if (len > 0 && a_intfc->ReadRecordData(affixId.data(), len) != len) {
								return;
							}

							token = affixId.empty() ? 0u : MakeAffixToken(affixId);
							} else {
								if (a_intfc->ReadRecordData(token) != sizeof(token)) {
									return;
								}
								if (version == kSerializationVersionV5 || version == kSerializationVersionV4) {
									if (a_intfc->ReadRecordData(supplementalToken) != sizeof(supplementalToken)) {
										return;
									}
								}

								if (version == kSerializationVersionV5 ||
									version == kSerializationVersionV4 ||
									version == kSerializationVersionV3) {
									if (a_intfc->ReadRecordData(state.evolutionXp) != sizeof(state.evolutionXp)) {
										return;
									}
								if (a_intfc->ReadRecordData(state.modeCycleCounter) != sizeof(state.modeCycleCounter)) {
									return;
								}
								if (a_intfc->ReadRecordData(state.modeIndex) != sizeof(state.modeIndex)) {
									return;
								}
							}
						}

						RE::FormID resolvedBaseID = 0;
						if (!a_intfc->ResolveFormID(baseID, resolvedBaseID)) {
							continue;
						}

							// v5 migration: convert to InstanceAffixSlots
							const auto key = MakeInstanceKey(resolvedBaseID, uniqueID);
							InstanceAffixSlots slots;
							if (token != 0u) {
								slots.AddToken(token);
							}
							if (supplementalToken != 0u) {
								slots.AddToken(supplementalToken);
							}
							_instanceAffixes.emplace(key, slots);
							if (version == kSerializationVersionV5 ||
								version == kSerializationVersionV4 ||
								version == kSerializationVersionV3) {
								const auto stateToken = (token != 0u) ? token : supplementalToken;
								if (stateToken != 0u) {
									_instanceStates[MakeInstanceStateKey(key, stateToken)] = state;
								}
							}
						}

					continue;
				}

				if (type == kSerializationRecordInstanceRuntimeStates) {
					if (version != kInstanceRuntimeStateSerializationVersion) {
						std::vector<std::uint8_t> sink(length);
						if (length > 0) {
							a_intfc->ReadRecordData(sink.data(), length);
						}
						continue;
					}

					std::uint32_t count = 0;
					if (a_intfc->ReadRecordData(count) != sizeof(count)) {
						return;
					}

					for (std::uint32_t i = 0; i < count; ++i) {
						RE::FormID baseID = 0;
						std::uint16_t uniqueID = 0;
						std::uint64_t affixToken = 0;
						InstanceRuntimeState state{};
						if (a_intfc->ReadRecordData(baseID) != sizeof(baseID)) {
							return;
						}
						if (a_intfc->ReadRecordData(uniqueID) != sizeof(uniqueID)) {
							return;
						}
						if (a_intfc->ReadRecordData(affixToken) != sizeof(affixToken)) {
							return;
						}
						if (a_intfc->ReadRecordData(state.evolutionXp) != sizeof(state.evolutionXp)) {
							return;
						}
						if (a_intfc->ReadRecordData(state.modeCycleCounter) != sizeof(state.modeCycleCounter)) {
							return;
						}
						if (a_intfc->ReadRecordData(state.modeIndex) != sizeof(state.modeIndex)) {
							return;
						}

						RE::FormID resolvedBaseID = 0;
						if (!a_intfc->ResolveFormID(baseID, resolvedBaseID) || affixToken == 0u) {
							continue;
						}

						const auto key = MakeInstanceKey(resolvedBaseID, uniqueID);
						_instanceStates[MakeInstanceStateKey(key, affixToken)] = state;
					}

					continue;
				}

				if (type == kSerializationRecordRunewordState) {
					if (version != kRunewordSerializationVersion) {
						std::vector<std::uint8_t> sink(length);
						if (length > 0) {
							a_intfc->ReadRecordData(sink.data(), length);
						}
						continue;
					}

					RE::FormID selectedBaseID = 0;
					std::uint16_t selectedUniqueID = 0;
					if (a_intfc->ReadRecordData(selectedBaseID) != sizeof(selectedBaseID)) {
						return;
					}
					if (a_intfc->ReadRecordData(selectedUniqueID) != sizeof(selectedUniqueID)) {
						return;
					}
					if (a_intfc->ReadRecordData(_runewordRecipeCycleCursor) != sizeof(_runewordRecipeCycleCursor)) {
						return;
					}
					if (a_intfc->ReadRecordData(_runewordBaseCycleCursor) != sizeof(_runewordBaseCycleCursor)) {
						return;
					}

					std::uint32_t fragmentCount = 0;
					if (a_intfc->ReadRecordData(fragmentCount) != sizeof(fragmentCount)) {
						return;
					}
					for (std::uint32_t i = 0; i < fragmentCount; ++i) {
						std::uint64_t runeToken = 0;
						std::uint32_t amount = 0;
						if (a_intfc->ReadRecordData(runeToken) != sizeof(runeToken)) {
							return;
						}
						if (a_intfc->ReadRecordData(amount) != sizeof(amount)) {
							return;
						}
						if (runeToken != 0u && amount > 0u) {
							_runewordRuneFragments[runeToken] = amount;
						}
					}

					std::uint32_t runewordStateCount = 0;
					if (a_intfc->ReadRecordData(runewordStateCount) != sizeof(runewordStateCount)) {
						return;
					}
					for (std::uint32_t i = 0; i < runewordStateCount; ++i) {
						RE::FormID baseID = 0;
						std::uint16_t uniqueID = 0;
						RunewordInstanceState state{};
						if (a_intfc->ReadRecordData(baseID) != sizeof(baseID)) {
							return;
						}
						if (a_intfc->ReadRecordData(uniqueID) != sizeof(uniqueID)) {
							return;
						}
						if (a_intfc->ReadRecordData(state.recipeToken) != sizeof(state.recipeToken)) {
							return;
						}
						if (a_intfc->ReadRecordData(state.insertedRunes) != sizeof(state.insertedRunes)) {
							return;
						}

						RE::FormID resolvedBaseID = 0;
						if (!a_intfc->ResolveFormID(baseID, resolvedBaseID)) {
							continue;
						}

						const auto key = MakeInstanceKey(resolvedBaseID, uniqueID);
						_runewordInstanceStates[key] = state;
					}

					if (selectedBaseID != 0 && selectedUniqueID != 0) {
						RE::FormID resolvedSelectedBaseID = 0;
						if (a_intfc->ResolveFormID(selectedBaseID, resolvedSelectedBaseID)) {
							_runewordSelectedBaseKey = MakeInstanceKey(resolvedSelectedBaseID, selectedUniqueID);
						}
					}

					continue;
				}

				if (type == kSerializationRecordLootEvaluated) {
					if (version != kLootEvaluatedSerializationVersion) {
						std::vector<std::uint8_t> sink(length);
						if (length > 0) {
							a_intfc->ReadRecordData(sink.data(), length);
						}
						continue;
					}

					std::uint32_t count = 0;
					if (a_intfc->ReadRecordData(count) != sizeof(count)) {
						return;
					}

					for (std::uint32_t i = 0; i < count; ++i) {
						RE::FormID baseID = 0;
						std::uint16_t uniqueID = 0;
						if (a_intfc->ReadRecordData(baseID) != sizeof(baseID)) {
							return;
						}
						if (a_intfc->ReadRecordData(uniqueID) != sizeof(uniqueID)) {
							return;
						}

						RE::FormID resolvedBaseID = 0;
						if (!a_intfc->ResolveFormID(baseID, resolvedBaseID)) {
							continue;
						}

						const auto key = MakeInstanceKey(resolvedBaseID, uniqueID);
						if (key != 0) {
							_lootEvaluatedInstances.insert(key);
						}
					}

					continue;
				}

				{
					// Drain unknown record.
					std::vector<std::uint8_t> sink(length);
					if (length > 0) {
						a_intfc->ReadRecordData(sink.data(), length);
					}
				}
			}

			for (const auto& [key, _] : _instanceAffixes) {
				_lootEvaluatedInstances.insert(key);
			}

			_lootEvaluatedRecent.clear();
			for (const auto key : _lootEvaluatedInstances) {
				if (key != 0) {
					_lootEvaluatedRecent.push_back(key);
				}
			}
			const std::size_t maxRecentEntries = kLootEvaluatedRecentKeep * 2;
			while (_lootEvaluatedRecent.size() > maxRecentEntries) {
				_lootEvaluatedRecent.pop_front();
			}
			_lootEvaluatedInsertionsSincePrune = 0;

			SanitizeRunewordState();
		}

		void EventBridge::Revert(SKSE::SerializationInterface*)
		{
			// Remove any active passive suffix spells before clearing
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (player) {
				for (auto* spell : _appliedPassiveSpells) {
					player->RemoveSpell(spell);
				}
			}
			_appliedPassiveSpells.clear();
			_instanceAffixes.clear();
			_equippedInstanceKeysByToken.clear();
			_equippedTokenCacheReady = false;
			_lootEvaluatedInstances.clear();
			_lootEvaluatedRecent.clear();
			_lootEvaluatedInsertionsSincePrune = 0;
			_activeSlotPenalty.clear();
			_instanceStates.clear();
			_runewordRuneFragments.clear();
			_runewordInstanceStates.clear();
			_runewordSelectedBaseKey.reset();
			_runewordBaseCycleCursor = 0;
			_runewordRecipeCycleCursor = 0;
			_traps.clear();
			_hasActiveTraps.store(false, std::memory_order_relaxed);
			_dotCooldowns.clear();
			_dotCooldownsLastPruneAt = {};
			_dotObservedMagicEffects.clear();
			_dotTagSafetyWarned = false;
			_dotTagSafetySuppressed = false;
			_perTargetCooldownStore.Clear();
			_nonHostileFirstHitGate.Clear();
			_corpseExplosionSeenCorpses.clear();
		_lootRerollGuard.Reset();
		_corpseExplosionState = {};
		_procDepth = 0;
		_healthDamageHookSeen = false;
		_healthDamageHookLastAt = {};
		_castOnCritNextAllowed = {};
		_castOnCritCycleCursor = 0;
		_lastHitAt = {};
		_lastHit = {};
		_lastHealthDamageSignature = 0;
		_lastHealthDamageSignatureAt = {};

		for (auto& affix : _affixes) {
			affix.nextAllowed = {};
		}
	}

	void EventBridge::OnFormDelete(RE::VMHandle a_handle)
	{
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm) {
			return;
		}

		auto* policy = vm->GetObjectHandlePolicy();
		if (!policy) {
			return;
		}

		if (!policy->HandleIsType(RE::FormType::Reference, a_handle)) {
			return;
		}

		auto* form = policy->GetObjectForHandle(RE::FormType::Reference, a_handle);
		auto* ref = form ? form->As<RE::TESObjectREFR>() : nullptr;
		if (!ref) {
			return;
		}

		const auto refHandle = static_cast<LootRerollGuard::RefHandle>(ref->GetHandle().native_handle());
		if (refHandle == 0) {
			return;
		}

		// Defer pruning to the task queue so that pickup container-changed events can consume
		// the drop guard first (prevents accidental prune on immediate re-pickup).
		if (auto* task = SKSE::GetTaskInterface()) {
			task->AddTask([refHandle]() {
				EventBridge::GetSingleton()->ProcessDroppedRefDeleted(refHandle);
			});
		} else {
			ProcessDroppedRefDeleted(refHandle);
		}
	}

		void EventBridge::EraseInstanceRuntimeStates(std::uint64_t a_instanceKey)
		{
			std::erase_if(_instanceStates, [a_instanceKey](const auto& entry) {
				return entry.first.instanceKey == a_instanceKey;
			});
		}

		void EventBridge::ProcessDroppedRefDeleted(LootRerollGuard::RefHandle a_refHandle)
		{
			const auto keyOpt = _lootRerollGuard.ConsumeIfPlayerDropDeleted(a_refHandle);
			if (!keyOpt) {
				return;
			}

			const auto key = *keyOpt;
			const auto it = _instanceAffixes.find(key);
			const bool wasLootEvaluated = IsLootEvaluatedInstance(key);
			const bool hasRunewordState = _runewordInstanceStates.contains(key);
			const bool isSelectedRunewordBase = (_runewordSelectedBaseKey && *_runewordSelectedBaseKey == key);
			if (it == _instanceAffixes.end() &&
				!wasLootEvaluated &&
				!hasRunewordState &&
				!isSelectedRunewordBase) {
				return;
			}

		// Safety net: if the instance is already back in the player inventory, don't prune it.
		if (PlayerHasInstanceKey(key)) {
			return;
		}

			if (it != _instanceAffixes.end()) {
				_instanceAffixes.erase(it);
			}
			ForgetLootEvaluatedInstance(key);
			EraseInstanceRuntimeStates(key);
			_runewordInstanceStates.erase(key);
			if (_runewordSelectedBaseKey && *_runewordSelectedBaseKey == key) {
				_runewordSelectedBaseKey.reset();
			}
			if (_loot.debugLog) {
				SKSE::log::debug("CalamityAffixes: pruned instance affix (world ref deleted) (key={:016X}).", key);
			}
		}

	bool EventBridge::PlayerHasInstanceKey(std::uint64_t a_instanceKey) const
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return false;
		}

		auto* changes = player->GetInventoryChanges();
		if (!changes || !changes->entryList) {
			return false;
		}

		for (auto* entry : *changes->entryList) {
			if (!entry || !entry->extraLists) {
				continue;
			}

			for (auto* xList : *entry->extraLists) {
				if (!xList) {
					continue;
				}

				const auto* uid = xList->GetByType<RE::ExtraUniqueID>();
				if (!uid) {
					continue;
				}

				if (MakeInstanceKey(uid->baseID, uid->uniqueID) == a_instanceKey) {
					return true;
				}
			}
		}

		return false;
	}
}
