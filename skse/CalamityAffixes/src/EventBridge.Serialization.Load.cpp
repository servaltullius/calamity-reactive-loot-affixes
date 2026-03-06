#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/LootRollSelection.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
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
			// Per-record read tracker for partial-failure recovery.
			// On read failure, drain remaining bytes and continue to next record
			// instead of abandoning the entire Load.
			std::uint32_t bytesRead = 0;
			bool recordOk = true;
			auto rd = [&](auto& val) -> bool {
				if (!recordOk) return false;
				auto sz = a_intfc->ReadRecordData(val);
				bytesRead += sz;
				if (sz != sizeof(val)) { recordOk = false; return false; }
				return true;
			};
			auto rdBuf = [&](void* buf, std::uint32_t len) -> bool {
				if (!recordOk) return false;
				auto sz = a_intfc->ReadRecordData(buf, len);
				bytesRead += sz;
				if (sz != len) { recordOk = false; return false; }
				return true;
			};
			auto drainRemaining = [&]() {
				if (bytesRead < length) {
					const auto remaining = length - bytesRead;
					DrainRecordBytes(a_intfc, remaining, "partial-record-recovery");
				}
			};

			if (type == kSerializationRecordInstanceAffixes) {
				if (version != kSerializationVersion &&
					version != kSerializationVersionV6 &&
					version != kSerializationVersionV5 &&
					version != kSerializationVersionV4 &&
					version != kSerializationVersionV3 &&
					version != kSerializationVersionV2 &&
					version != kSerializationVersionV1) {
						DrainRecordBytes(a_intfc, length, "unsupported-iaxf-version");
						continue;
					}

					if (version == kSerializationVersion) {
						// --- v7 load: fixed 4-token slots ---
						std::uint32_t count = 0;
						if (!rd(count)) {
							SKSE::log::warn("CalamityAffixes: truncated IAXF v7 record header; skipping.");
							drainRemaining();
							continue;
						}

						SKSE::log::info("CalamityAffixes: IAXF v7 — co-save contains {} instance entries.", count);
						for (std::uint32_t i = 0; i < count; ++i) {
							RE::FormID baseID = 0;
							std::uint16_t uniqueID = 0;
							std::uint8_t affixCount = 0;
							std::array<std::uint64_t, kMaxAffixesPerItem> tokens{};

							if (!rd(baseID) || !rd(uniqueID) || !rd(affixCount)) {
								break;
							}
							bool tokensOk = true;
							for (std::size_t s = 0; s < kMaxAffixesPerItem; ++s) {
								if (!rd(tokens[s])) { tokensOk = false; break; }
							}
							if (!tokensOk) break;

							RE::FormID resolvedBaseID = 0;
							if (!a_intfc->ResolveFormID(baseID, resolvedBaseID)) {
								SKSE::log::warn("CalamityAffixes: IAXF v7 — ResolveFormID failed for baseID {:08X}, skipping entry.", baseID);
								continue;
							}

							const auto key = MakeInstanceKey(resolvedBaseID, uniqueID);
							InstanceAffixSlots slots;
							slots.count = std::min<std::uint8_t>(affixCount, static_cast<std::uint8_t>(kMaxAffixesPerItem));
							slots.tokens = tokens;
							_instanceAffixes.emplace(key, slots);
						}
						if (!recordOk) {
							SKSE::log::warn("CalamityAffixes: truncated IAXF v7 record; recovered {} entries.", _instanceAffixes.size());
							drainRemaining();
						}

						SKSE::log::info("CalamityAffixes: IAXF v7 — loaded {} instance entries from co-save.", _instanceAffixes.size());
						continue;
					}

					if (version == kSerializationVersionV6) {
						// --- v6 load: fixed 3-token slots ---
						std::uint32_t count = 0;
						if (!rd(count)) {
							SKSE::log::warn("CalamityAffixes: truncated IAXF v6 record header; skipping.");
							drainRemaining();
							continue;
						}

						for (std::uint32_t i = 0; i < count; ++i) {
							RE::FormID baseID = 0;
							std::uint16_t uniqueID = 0;
							std::uint8_t affixCount = 0;
							std::array<std::uint64_t, 3> legacyTokens{};

							if (!rd(baseID) || !rd(uniqueID) || !rd(affixCount)) {
								break;
							}
							bool tokensOk = true;
							for (std::size_t s = 0; s < legacyTokens.size(); ++s) {
								if (!rd(legacyTokens[s])) { tokensOk = false; break; }
							}
							if (!tokensOk) break;

							RE::FormID resolvedBaseID = 0;
							if (!a_intfc->ResolveFormID(baseID, resolvedBaseID)) {
								SKSE::log::warn("CalamityAffixes: IAXF v6 — ResolveFormID failed for baseID {:08X}, skipping entry.", baseID);
								continue;
							}

							const auto key = MakeInstanceKey(resolvedBaseID, uniqueID);
							InstanceAffixSlots slots;
							slots.count = std::min<std::uint8_t>(affixCount, static_cast<std::uint8_t>(legacyTokens.size()));
							for (std::uint8_t s = 0; s < slots.count; ++s) {
								slots.tokens[s] = legacyTokens[s];
							}
							_instanceAffixes.emplace(key, slots);
						}
						if (!recordOk) {
							SKSE::log::warn("CalamityAffixes: truncated IAXF v6 record; recovered {} entries.", _instanceAffixes.size());
							drainRemaining();
						}

						continue;
					}

					// --- v1~v5 legacy load ---
					std::uint32_t count = 0;
					if (!rd(count)) {
						SKSE::log::warn("CalamityAffixes: truncated IAXF v1-v5 record header; skipping.");
						drainRemaining();
						continue;
					}

					for (std::uint32_t i = 0; i < count; i++) {
						RE::FormID baseID = 0;
						std::uint16_t uniqueID = 0;

						if (!rd(baseID) || !rd(uniqueID)) {
							break;
						}

							std::uint64_t token = 0;
							std::uint64_t supplementalToken = 0;
							InstanceRuntimeState state{};
						if (version == kSerializationVersionV1) {
							std::uint32_t len = 0;
							if (!rd(len)) break;

							std::string affixId;
							if (len > kMaxV1AffixIdLength) {
								SKSE::log::error("CalamityAffixes: corrupt v1 save — affixId length {} exceeds limit.", len);
								break;
							}
							affixId.resize(len);
							if (len > 0 && !rdBuf(affixId.data(), len)) {
								break;
							}

							token = affixId.empty() ? 0u : MakeAffixToken(affixId);
							} else {
								if (!rd(token)) break;
								if (version == kSerializationVersionV5 || version == kSerializationVersionV4) {
									if (!rd(supplementalToken)) break;
								}

								if (version == kSerializationVersionV5 ||
									version == kSerializationVersionV4 ||
									version == kSerializationVersionV3) {
									if (!rd(state.evolutionXp)) break;
								if (!rd(state.modeCycleCounter)) break;
								if (!rd(state.modeIndex)) break;
							}
						}

						RE::FormID resolvedBaseID = 0;
						if (!a_intfc->ResolveFormID(baseID, resolvedBaseID)) {
							SKSE::log::warn("CalamityAffixes: IAXF v1-v5 — ResolveFormID failed for baseID {:08X}, skipping entry.", baseID);
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
					if (!recordOk) {
						SKSE::log::warn("CalamityAffixes: truncated IAXF v1-v5 record; recovered {} entries.", _instanceAffixes.size());
						drainRemaining();
					}

					continue;
				}

				if (type == kSerializationRecordInstanceRuntimeStates) {
					if (version != kInstanceRuntimeStateSerializationVersion) {
						DrainRecordBytes(a_intfc, length, "unsupported-irst-version");
						continue;
					}

					std::uint32_t count = 0;
					if (!rd(count)) {
						SKSE::log::warn("CalamityAffixes: truncated IRST record header; skipping.");
						drainRemaining();
						continue;
					}

					for (std::uint32_t i = 0; i < count; ++i) {
						RE::FormID baseID = 0;
						std::uint16_t uniqueID = 0;
						std::uint64_t affixToken = 0;
						InstanceRuntimeState state{};
						if (!rd(baseID) || !rd(uniqueID) || !rd(affixToken)) {
							break;
						}
						if (!rd(state.evolutionXp) || !rd(state.modeCycleCounter) || !rd(state.modeIndex)) {
							break;
						}

						RE::FormID resolvedBaseID = 0;
						if (!a_intfc->ResolveFormID(baseID, resolvedBaseID)) {
							SKSE::log::warn("CalamityAffixes: IRST — ResolveFormID failed for baseID {:08X}, skipping entry.", baseID);
							continue;
						}
						if (affixToken == 0u) {
							continue;
						}
						const auto key = MakeInstanceKey(resolvedBaseID, uniqueID);
						_instanceStates[MakeInstanceStateKey(key, affixToken)] = state;
					}
					if (!recordOk) {
						SKSE::log::warn("CalamityAffixes: truncated IRST record; recovered {} entries.", _instanceStates.size());
						drainRemaining();
					}

					continue;
				}

				if (type == kSerializationRecordRunewordState) {
					if (version != kRunewordSerializationVersion) {
						DrainRecordBytes(a_intfc, length, "unsupported-rwrd-version");
						continue;
					}

					RE::FormID selectedBaseID = 0;
					std::uint16_t selectedUniqueID = 0;
					if (!rd(selectedBaseID) || !rd(selectedUniqueID) ||
						!rd(_runewordState.recipeCycleCursor) || !rd(_runewordState.baseCycleCursor)) {
						SKSE::log::warn("CalamityAffixes: truncated RWRD record header; skipping.");
						drainRemaining();
						continue;
					}

					std::uint32_t fragmentCount = 0;
					if (!rd(fragmentCount)) {
						SKSE::log::warn("CalamityAffixes: truncated RWRD fragment count; skipping remainder.");
						drainRemaining();
						continue;
					}
					for (std::uint32_t i = 0; i < fragmentCount; ++i) {
						std::uint64_t runeToken = 0;
						std::uint32_t amount = 0;
						if (!rd(runeToken) || !rd(amount)) {
							break;
						}
						if (runeToken != 0u && amount > 0u) {
							_runewordState.runeFragments[runeToken] = amount;
						}
					}
					if (!recordOk) {
						SKSE::log::warn("CalamityAffixes: truncated RWRD fragments; recovered {} entries.", _runewordState.runeFragments.size());
						drainRemaining();
						continue;
					}

					std::uint32_t runewordStateCount = 0;
					if (!rd(runewordStateCount)) {
						SKSE::log::warn("CalamityAffixes: truncated RWRD state count; skipping remainder.");
						drainRemaining();
						continue;
					}
					for (std::uint32_t i = 0; i < runewordStateCount; ++i) {
						RE::FormID baseID = 0;
						std::uint16_t uniqueID = 0;
						RunewordInstanceState state{};
						if (!rd(baseID) || !rd(uniqueID) ||
							!rd(state.recipeToken) || !rd(state.insertedRunes)) {
							break;
						}

						RE::FormID resolvedBaseID = 0;
						if (!a_intfc->ResolveFormID(baseID, resolvedBaseID)) {
							SKSE::log::warn("CalamityAffixes: RWRD — ResolveFormID failed for baseID {:08X}, skipping entry.", baseID);
							continue;
						}

						const auto key = MakeInstanceKey(resolvedBaseID, uniqueID);
						_runewordState.instanceStates[key] = state;
					}
					if (!recordOk) {
						SKSE::log::warn("CalamityAffixes: truncated RWRD states; recovered {} entries.", _runewordState.instanceStates.size());
						drainRemaining();
						// Fall through to resolve selectedBase with whatever was read.
					}

					if (selectedBaseID != 0 && selectedUniqueID != 0) {
						RE::FormID resolvedSelectedBaseID = 0;
						if (a_intfc->ResolveFormID(selectedBaseID, resolvedSelectedBaseID) && resolvedSelectedBaseID != 0) {
							_runewordState.selectedBaseKey = MakeInstanceKey(resolvedSelectedBaseID, selectedUniqueID);
						}
					}

					continue;
				}

				if (type == kSerializationRecordLootEvaluated) {
					if (version != kLootEvaluatedSerializationVersion) {
						DrainRecordBytes(a_intfc, length, "unsupported-lrld-version");
						continue;
					}

					std::uint32_t count = 0;
					if (!rd(count)) {
						SKSE::log::warn("CalamityAffixes: truncated LRLD record header; skipping.");
						drainRemaining();
						continue;
					}

					for (std::uint32_t i = 0; i < count; ++i) {
						RE::FormID baseID = 0;
						std::uint16_t uniqueID = 0;
						if (!rd(baseID) || !rd(uniqueID)) {
							break;
						}

						RE::FormID resolvedBaseID = 0;
						if (!a_intfc->ResolveFormID(baseID, resolvedBaseID)) {
							SKSE::log::warn("CalamityAffixes: LRLD — ResolveFormID failed for baseID {:08X}, skipping entry.", baseID);
							continue;
						}

						const auto key = MakeInstanceKey(resolvedBaseID, uniqueID);
						if (key != 0) {
							_lootState.evaluatedInstances.insert(key);
						}
					}
					if (!recordOk) {
						SKSE::log::warn(
							"CalamityAffixes: truncated LRLD record; recovered {} entries.",
							_lootState.evaluatedInstances.size());
						drainRemaining();
					}

					continue;
				}

				if (type == kSerializationRecordLootCurrencyLedger) {
					if (version != kLootCurrencyLedgerSerializationVersion &&
						version != kLootCurrencyLedgerSerializationVersionV1) {
						DrainRecordBytes(a_intfc, length, "unsupported-lcld-version");
						continue;
					}

					std::uint32_t count = 0;
					if (!rd(count)) {
						SKSE::log::warn("CalamityAffixes: truncated LCLD record header; skipping.");
						drainRemaining();
						continue;
					}

					for (std::uint32_t i = 0; i < count; ++i) {
						std::uint64_t key = 0;
						if (!rd(key)) break;
						std::uint32_t dayStamp = 0u;
						if (version == kLootCurrencyLedgerSerializationVersion) {
							if (!rd(dayStamp)) break;
						}
						if (key != 0u) {
							_lootState.currencyRollLedger.emplace(key, dayStamp);
						}
					}
					if (!recordOk) {
						SKSE::log::warn(
							"CalamityAffixes: truncated LCLD record; recovered {} entries.",
							_lootState.currencyRollLedger.size());
						drainRemaining();
					}

					continue;
				}

				if (type == kSerializationRecordLootShuffleBags) {
					if (version != kLootShuffleBagSerializationVersion) {
						DrainRecordBytes(a_intfc, length, "unsupported-lsbg-version");
						continue;
					}

					std::uint8_t bagCount = 0;
					if (!rd(bagCount)) {
						SKSE::log::warn("CalamityAffixes: truncated LSBG record header; skipping.");
						drainRemaining();
						continue;
					}

					auto resolveBag = [&](std::uint8_t a_id) -> LootShuffleBagState* {
						switch (a_id) {
						case 0u:
							return &_lootState.prefixSharedBag;
						case 1u:
							return &_lootState.prefixWeaponBag;
						case 2u:
							return &_lootState.prefixArmorBag;
						case 3u:
							return &_lootState.suffixSharedBag;
						case 4u:
							return &_lootState.suffixWeaponBag;
						case 5u:
							return &_lootState.suffixArmorBag;
						default:
							return nullptr;
						}
					};

					for (std::uint8_t i = 0; i < bagCount; ++i) {
						std::uint8_t id = 0;
						std::uint32_t cursor = 0;
						std::uint32_t size = 0;
						if (!rd(id) || !rd(cursor) || !rd(size)) {
							break;
						}

						std::vector<std::size_t> order;
						if (size > kMaxShuffleBagSize) {
							SKSE::log::error("CalamityAffixes: corrupt save — shuffle bag size {} exceeds limit.", size);
							recordOk = false;
							break;
						}
						order.reserve(size);
						bool orderOk = true;
						for (std::uint32_t n = 0; n < size; ++n) {
							std::uint32_t rawIdx = 0;
							if (!rd(rawIdx)) { orderOk = false; break; }
							order.push_back(static_cast<std::size_t>(rawIdx));
						}
						if (!orderOk) break;

						if (auto* bag = resolveBag(id)) {
							bag->order = std::move(order);
							bag->cursor = std::min<std::size_t>(cursor, bag->order.size());
						}
					}
					if (!recordOk) {
						SKSE::log::warn("CalamityAffixes: truncated LSBG record; recovered partial shuffle bags.");
						drainRemaining();
					}

					continue;
				}

				if (type == kSerializationRecordMigrationFlags) {
					if (version != kMigrationFlagsVersion) {
						DrainRecordBytes(a_intfc, length, "unsupported-mflg-version");
						continue;
					}

					std::uint8_t flags = 0;
					if (!rd(flags)) {
						SKSE::log::warn("CalamityAffixes: truncated MFLG record; skipping.");
						drainRemaining();
						continue;
					}
					_miscCurrencyMigrated = (flags & 1u) != 0;
					_miscCurrencyRecovered = (flags & 2u) != 0;
					continue;
				}

				{
					// Drain unknown record.
					DrainRecordBytes(a_intfc, length, "unknown-record");
				}
			}

			SKSE::log::info("CalamityAffixes: Load() — deserialized {} instance entries, {} runtime states.", _instanceAffixes.size(), _instanceStates.size());
			if (!_affixRegistry.affixIndexByToken.empty() && !_affixes.empty()) {
				SanitizeAllTrackedLootInstancesForCurrentLootRules("Serialization.Load");
			}

			for (const auto& [key, _] : _instanceAffixes) {
				_lootState.evaluatedInstances.insert(key);
			}

			_lootState.evaluatedRecent.clear();
			for (const auto key : _lootState.evaluatedInstances) {
				if (key != 0) {
					_lootState.evaluatedRecent.push_back(key);
				}
			}
			const std::size_t maxRecentEntries = kLootEvaluatedRecentKeep * 2;
			while (_lootState.evaluatedRecent.size() > maxRecentEntries) {
				_lootState.evaluatedRecent.pop_front();
			}
			_lootState.evaluatedInsertionsSincePrune = 0;

			_lootState.currencyRollLedgerRecent.clear();
			for (const auto& [key, _] : _lootState.currencyRollLedger) {
				if (key != 0u) {
					_lootState.currencyRollLedgerRecent.push_back(key);
				}
			}
			while (_lootState.currencyRollLedgerRecent.size() > kLootCurrencyLedgerMaxEntries) {
				const auto oldest = _lootState.currencyRollLedgerRecent.front();
				_lootState.currencyRollLedgerRecent.pop_front();
				_lootState.currencyRollLedger.erase(oldest);
			}

			detail::SanitizeLootShuffleBagOrder(
				_affixRegistry.lootSharedAffixes,
				_lootState.prefixSharedBag.order,
				_lootState.prefixSharedBag.cursor);
			detail::SanitizeLootShuffleBagOrder(
				_affixRegistry.lootWeaponAffixes,
				_lootState.prefixWeaponBag.order,
				_lootState.prefixWeaponBag.cursor);
			detail::SanitizeLootShuffleBagOrder(
				_affixRegistry.lootArmorAffixes,
				_lootState.prefixArmorBag.order,
				_lootState.prefixArmorBag.cursor);
			detail::SanitizeLootShuffleBagOrder(
				_affixRegistry.lootSharedSuffixes,
				_lootState.suffixSharedBag.order,
				_lootState.suffixSharedBag.cursor);
			detail::SanitizeLootShuffleBagOrder(
				_affixRegistry.lootWeaponSuffixes,
				_lootState.suffixWeaponBag.order,
				_lootState.suffixWeaponBag.cursor);
			detail::SanitizeLootShuffleBagOrder(
				_affixRegistry.lootArmorSuffixes,
				_lootState.suffixArmorBag.order,
				_lootState.suffixArmorBag.cursor);

			SanitizeRunewordState();

			// Rebuild active affix counts to restore passive suffix spells
			// that Revert() cleared before this Load() ran.
			RebuildActiveCounts();

			// NOTE: MaybeMigrateMiscCurrency() is intentionally NOT called here.
			// During the SKSE Load callback the game engine has not yet restored the
			// player's inventory, so GetItemCount() returns 0 for all items.
			// The migration check is deferred to OnPostLoadGame() (kPostLoadGame message),
			// where the game state is fully loaded.
		}
}
