		struct SerializationLoadCursor
		{
			SKSE::SerializationInterface* intfc{ nullptr };
			std::uint32_t length{ 0u };
			std::uint32_t bytesRead{ 0u };
			bool recordOk{ true };

			template <class T>
			bool Read(T& a_value)
			{
				if (!recordOk) {
					return false;
				}

				const auto size = intfc->ReadRecordData(a_value);
				bytesRead += size;
				if (size != sizeof(a_value)) {
					recordOk = false;
					return false;
				}
				return true;
			}

			bool ReadBuffer(void* a_buffer, std::uint32_t a_length)
			{
				if (!recordOk) {
					return false;
				}

				const auto size = intfc->ReadRecordData(a_buffer, a_length);
				bytesRead += size;
				if (size != a_length) {
					recordOk = false;
					return false;
				}
				return true;
			}

			void DrainRemaining(const char* a_context)
			{
				if (bytesRead < length) {
					DrainRecordBytes(intfc, length - bytesRead, a_context);
				}
			}
		};

		[[nodiscard]] bool IsSupportedInstanceAffixSerializationVersion(std::uint32_t a_version) noexcept
		{
			return a_version >= 1u && a_version <= 7u;
		}

	void EventBridge::LoadInstanceAffixesRecord(
		SKSE::SerializationInterface* a_intfc,
		std::uint32_t a_version,
		std::uint32_t a_length)
	{
		if (!IsSupportedInstanceAffixSerializationVersion(a_version)) {
			DrainRecordBytes(a_intfc, a_length, "unsupported-iaxf-version");
			return;
		}

		SerializationLoadCursor cursor{ .intfc = a_intfc, .length = a_length };

		if (a_version == kSerializationVersion) {
			std::uint32_t count = 0;
			if (!cursor.Read(count)) {
				SKSE::log::warn("CalamityAffixes: truncated IAXF v7 record header; skipping.");
				cursor.DrainRemaining("partial-record-recovery");
				return;
			}

			SKSE::log::info("CalamityAffixes: IAXF v7 — co-save contains {} instance entries.", count);
			for (std::uint32_t i = 0; i < count; ++i) {
				RE::FormID baseID = 0;
				std::uint16_t uniqueID = 0;
				std::uint8_t affixCount = 0;
				std::array<std::uint64_t, kMaxAffixesPerItem> tokens{};

				if (!cursor.Read(baseID) || !cursor.Read(uniqueID) || !cursor.Read(affixCount)) {
					break;
				}
				bool tokensOk = true;
				for (std::size_t s = 0; s < kMaxAffixesPerItem; ++s) {
					if (!cursor.Read(tokens[s])) {
						tokensOk = false;
						break;
					}
				}
				if (!tokensOk) {
					break;
				}

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
			if (!cursor.recordOk) {
				SKSE::log::warn("CalamityAffixes: truncated IAXF v7 record; recovered {} entries.", _instanceAffixes.size());
				cursor.DrainRemaining("partial-record-recovery");
			}

			SKSE::log::info("CalamityAffixes: IAXF v7 — loaded {} instance entries from co-save.", _instanceAffixes.size());
			return;
		}

		if (a_version == kSerializationVersionV6) {
			std::uint32_t count = 0;
			if (!cursor.Read(count)) {
				SKSE::log::warn("CalamityAffixes: truncated IAXF v6 record header; skipping.");
				cursor.DrainRemaining("partial-record-recovery");
				return;
			}

			for (std::uint32_t i = 0; i < count; ++i) {
				RE::FormID baseID = 0;
				std::uint16_t uniqueID = 0;
				std::uint8_t affixCount = 0;
				std::array<std::uint64_t, 3> legacyTokens{};

				if (!cursor.Read(baseID) || !cursor.Read(uniqueID) || !cursor.Read(affixCount)) {
					break;
				}
				bool tokensOk = true;
				for (std::size_t s = 0; s < legacyTokens.size(); ++s) {
					if (!cursor.Read(legacyTokens[s])) {
						tokensOk = false;
						break;
					}
				}
				if (!tokensOk) {
					break;
				}

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
			if (!cursor.recordOk) {
				SKSE::log::warn("CalamityAffixes: truncated IAXF v6 record; recovered {} entries.", _instanceAffixes.size());
				cursor.DrainRemaining("partial-record-recovery");
			}
			return;
		}

		std::uint32_t count = 0;
		if (!cursor.Read(count)) {
			SKSE::log::warn("CalamityAffixes: truncated IAXF v1-v5 record header; skipping.");
			cursor.DrainRemaining("partial-record-recovery");
			return;
		}

		for (std::uint32_t i = 0; i < count; i++) {
			RE::FormID baseID = 0;
			std::uint16_t uniqueID = 0;

			if (!cursor.Read(baseID) || !cursor.Read(uniqueID)) {
				break;
			}

			std::uint64_t token = 0;
			std::uint64_t supplementalToken = 0;
			InstanceRuntimeState state{};
			if (a_version == kSerializationVersionV1) {
				std::uint32_t len = 0;
				if (!cursor.Read(len)) {
					break;
				}

				std::string affixId;
				if (len > kMaxV1AffixIdLength) {
					SKSE::log::error("CalamityAffixes: corrupt v1 save — affixId length {} exceeds limit.", len);
					break;
				}
				affixId.resize(len);
				if (len > 0 && !cursor.ReadBuffer(affixId.data(), len)) {
					break;
				}

				token = affixId.empty() ? 0u : MakeAffixToken(affixId);
			} else {
				if (!cursor.Read(token)) {
					break;
				}
				if (a_version == kSerializationVersionV5 || a_version == kSerializationVersionV4) {
					if (!cursor.Read(supplementalToken)) {
						break;
					}
				}

				if (a_version == kSerializationVersionV5 ||
					a_version == kSerializationVersionV4 ||
					a_version == kSerializationVersionV3) {
					if (!cursor.Read(state.evolutionXp) ||
						!cursor.Read(state.modeCycleCounter) ||
						!cursor.Read(state.modeIndex)) {
						break;
					}
				}
			}

			RE::FormID resolvedBaseID = 0;
			if (!a_intfc->ResolveFormID(baseID, resolvedBaseID)) {
				SKSE::log::warn("CalamityAffixes: IAXF v1-v5 — ResolveFormID failed for baseID {:08X}, skipping entry.", baseID);
				continue;
			}

			const auto key = MakeInstanceKey(resolvedBaseID, uniqueID);
			InstanceAffixSlots slots;
			if (token != 0u) {
				slots.AddToken(token);
			}
			if (supplementalToken != 0u) {
				slots.AddToken(supplementalToken);
			}
			_instanceAffixes.emplace(key, slots);
			if (a_version == kSerializationVersionV5 ||
				a_version == kSerializationVersionV4 ||
				a_version == kSerializationVersionV3) {
				const auto stateToken = (token != 0u) ? token : supplementalToken;
				if (stateToken != 0u) {
					_instanceStates[MakeInstanceStateKey(key, stateToken)] = state;
				}
			}
		}

		if (!cursor.recordOk) {
			SKSE::log::warn("CalamityAffixes: truncated IAXF v1-v5 record; recovered {} entries.", _instanceAffixes.size());
			cursor.DrainRemaining("partial-record-recovery");
		}
	}

	void EventBridge::LoadInstanceRuntimeStatesRecord(
		SKSE::SerializationInterface* a_intfc,
		std::uint32_t a_version,
		std::uint32_t a_length)
	{
		if (a_version != kInstanceRuntimeStateSerializationVersion) {
			DrainRecordBytes(a_intfc, a_length, "unsupported-irst-version");
			return;
		}

		SerializationLoadCursor cursor{ .intfc = a_intfc, .length = a_length };
		std::uint32_t count = 0;
		if (!cursor.Read(count)) {
			SKSE::log::warn("CalamityAffixes: truncated IRST record header; skipping.");
			cursor.DrainRemaining("partial-record-recovery");
			return;
		}

		for (std::uint32_t i = 0; i < count; ++i) {
			RE::FormID baseID = 0;
			std::uint16_t uniqueID = 0;
			std::uint64_t affixToken = 0;
			InstanceRuntimeState state{};
			if (!cursor.Read(baseID) || !cursor.Read(uniqueID) || !cursor.Read(affixToken)) {
				break;
			}
			if (!cursor.Read(state.evolutionXp) || !cursor.Read(state.modeCycleCounter) || !cursor.Read(state.modeIndex)) {
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
		if (!cursor.recordOk) {
			SKSE::log::warn("CalamityAffixes: truncated IRST record; recovered {} entries.", _instanceStates.size());
			cursor.DrainRemaining("partial-record-recovery");
		}
	}

	void EventBridge::LoadRunewordStateRecord(
		SKSE::SerializationInterface* a_intfc,
		std::uint32_t a_version,
		std::uint32_t a_length)
	{
		if (a_version != kRunewordSerializationVersion) {
			DrainRecordBytes(a_intfc, a_length, "unsupported-rwrd-version");
			return;
		}

		SerializationLoadCursor cursor{ .intfc = a_intfc, .length = a_length };
		RE::FormID selectedBaseID = 0;
		std::uint16_t selectedUniqueID = 0;
		if (!cursor.Read(selectedBaseID) || !cursor.Read(selectedUniqueID) ||
			!cursor.Read(_runewordState.recipeCycleCursor) || !cursor.Read(_runewordState.baseCycleCursor)) {
			SKSE::log::warn("CalamityAffixes: truncated RWRD record header; skipping.");
			cursor.DrainRemaining("partial-record-recovery");
			return;
		}

		std::uint32_t fragmentCount = 0;
		if (!cursor.Read(fragmentCount)) {
			SKSE::log::warn("CalamityAffixes: truncated RWRD fragment count; skipping remainder.");
			cursor.DrainRemaining("partial-record-recovery");
			return;
		}
		for (std::uint32_t i = 0; i < fragmentCount; ++i) {
			std::uint64_t runeToken = 0;
			std::uint32_t amount = 0;
			if (!cursor.Read(runeToken) || !cursor.Read(amount)) {
				break;
			}
			if (runeToken != 0u && amount > 0u) {
				_runewordState.runeFragments[runeToken] = amount;
			}
		}
		if (!cursor.recordOk) {
			SKSE::log::warn("CalamityAffixes: truncated RWRD fragments; recovered {} entries.", _runewordState.runeFragments.size());
			cursor.DrainRemaining("partial-record-recovery");
			return;
		}

		std::uint32_t runewordStateCount = 0;
		if (!cursor.Read(runewordStateCount)) {
			SKSE::log::warn("CalamityAffixes: truncated RWRD state count; skipping remainder.");
			cursor.DrainRemaining("partial-record-recovery");
			return;
		}
		for (std::uint32_t i = 0; i < runewordStateCount; ++i) {
			RE::FormID baseID = 0;
			std::uint16_t uniqueID = 0;
			RunewordInstanceState state{};
			if (!cursor.Read(baseID) || !cursor.Read(uniqueID) ||
				!cursor.Read(state.recipeToken) || !cursor.Read(state.insertedRunes)) {
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
		if (!cursor.recordOk) {
			SKSE::log::warn("CalamityAffixes: truncated RWRD states; recovered {} entries.", _runewordState.instanceStates.size());
			cursor.DrainRemaining("partial-record-recovery");
		}

		if (selectedBaseID != 0 && selectedUniqueID != 0) {
			RE::FormID resolvedSelectedBaseID = 0;
			if (a_intfc->ResolveFormID(selectedBaseID, resolvedSelectedBaseID) && resolvedSelectedBaseID != 0) {
				_runewordState.selectedBaseKey = MakeInstanceKey(resolvedSelectedBaseID, selectedUniqueID);
			}
		}
	}

	void EventBridge::LoadLootEvaluatedRecord(
		SKSE::SerializationInterface* a_intfc,
		std::uint32_t a_version,
		std::uint32_t a_length)
	{
		if (a_version != kLootEvaluatedSerializationVersion) {
			DrainRecordBytes(a_intfc, a_length, "unsupported-lrld-version");
			return;
		}

		SerializationLoadCursor cursor{ .intfc = a_intfc, .length = a_length };
		std::uint32_t count = 0;
		if (!cursor.Read(count)) {
			SKSE::log::warn("CalamityAffixes: truncated LRLD record header; skipping.");
			cursor.DrainRemaining("partial-record-recovery");
			return;
		}

		for (std::uint32_t i = 0; i < count; ++i) {
			RE::FormID baseID = 0;
			std::uint16_t uniqueID = 0;
			if (!cursor.Read(baseID) || !cursor.Read(uniqueID)) {
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
		if (!cursor.recordOk) {
			SKSE::log::warn("CalamityAffixes: truncated LRLD record; recovered {} entries.", _lootState.evaluatedInstances.size());
			cursor.DrainRemaining("partial-record-recovery");
		}
	}

	void EventBridge::LoadLootCurrencyLedgerRecord(
		SKSE::SerializationInterface* a_intfc,
		std::uint32_t a_version,
		std::uint32_t a_length)
	{
		if (a_version != kLootCurrencyLedgerSerializationVersion &&
			a_version != kLootCurrencyLedgerSerializationVersionV1) {
			DrainRecordBytes(a_intfc, a_length, "unsupported-lcld-version");
			return;
		}

		SerializationLoadCursor cursor{ .intfc = a_intfc, .length = a_length };
		std::uint32_t count = 0;
		if (!cursor.Read(count)) {
			SKSE::log::warn("CalamityAffixes: truncated LCLD record header; skipping.");
			cursor.DrainRemaining("partial-record-recovery");
			return;
		}

		for (std::uint32_t i = 0; i < count; ++i) {
			std::uint64_t key = 0;
			if (!cursor.Read(key)) {
				break;
			}
			std::uint32_t dayStamp = 0u;
			if (a_version == kLootCurrencyLedgerSerializationVersion) {
				if (!cursor.Read(dayStamp)) {
					break;
				}
			}
			if (key != 0u) {
				_lootState.currencyRollLedger.emplace(key, dayStamp);
			}
		}
		if (!cursor.recordOk) {
			SKSE::log::warn("CalamityAffixes: truncated LCLD record; recovered {} entries.", _lootState.currencyRollLedger.size());
			cursor.DrainRemaining("partial-record-recovery");
		}
	}

	void EventBridge::LoadLootShuffleBagsRecord(
		SKSE::SerializationInterface* a_intfc,
		std::uint32_t a_version,
		std::uint32_t a_length)
	{
		if (a_version != kLootShuffleBagSerializationVersion) {
			DrainRecordBytes(a_intfc, a_length, "unsupported-lsbg-version");
			return;
		}

		SerializationLoadCursor cursor{ .intfc = a_intfc, .length = a_length };
		std::uint8_t bagCount = 0;
		if (!cursor.Read(bagCount)) {
			SKSE::log::warn("CalamityAffixes: truncated LSBG record header; skipping.");
			cursor.DrainRemaining("partial-record-recovery");
			return;
		}

		for (std::uint8_t i = 0; i < bagCount; ++i) {
			std::uint8_t id = 0;
			std::uint32_t cursorValue = 0;
			std::uint32_t size = 0;
			if (!cursor.Read(id) || !cursor.Read(cursorValue) || !cursor.Read(size)) {
				break;
			}

			std::vector<std::size_t> order;
			if (size > kMaxShuffleBagSize) {
				SKSE::log::error("CalamityAffixes: corrupt save — shuffle bag size {} exceeds limit.", size);
				cursor.recordOk = false;
				break;
			}
			order.reserve(size);
			bool orderOk = true;
			for (std::uint32_t n = 0; n < size; ++n) {
				std::uint32_t rawIdx = 0;
				if (!cursor.Read(rawIdx)) {
					orderOk = false;
					break;
				}
				order.push_back(static_cast<std::size_t>(rawIdx));
			}
			if (!orderOk) {
				break;
			}

			if (auto* bag = SerializationLoadState::ResolveShuffleBag(_lootState, id)) {
				bag->order = std::move(order);
				bag->cursor = std::min<std::size_t>(cursorValue, bag->order.size());
			}
		}
		if (!cursor.recordOk) {
			SKSE::log::warn("CalamityAffixes: truncated LSBG record; recovered partial shuffle bags.");
			cursor.DrainRemaining("partial-record-recovery");
		}
	}

	void EventBridge::LoadMigrationFlagsRecord(
		SKSE::SerializationInterface* a_intfc,
		std::uint32_t a_version,
		std::uint32_t a_length)
	{
		if (a_version != kMigrationFlagsVersion) {
			DrainRecordBytes(a_intfc, a_length, "unsupported-mflg-version");
			return;
		}

		SerializationLoadCursor cursor{ .intfc = a_intfc, .length = a_length };
		std::uint8_t flags = 0;
		if (!cursor.Read(flags)) {
			SKSE::log::warn("CalamityAffixes: truncated MFLG record; skipping.");
			cursor.DrainRemaining("partial-record-recovery");
			return;
		}
		_miscCurrencyMigrated = (flags & 1u) != 0;
		_miscCurrencyRecovered = (flags & 2u) != 0;
	}

	void EventBridge::FinalizeLoadedSerializationState()
	{
		SKSE::log::info("CalamityAffixes: Load() — deserialized {} instance entries, {} runtime states.", _instanceAffixes.size(), _instanceStates.size());
		if (!_affixRegistry.affixIndexByToken.empty() && !_affixes.empty()) {
			SanitizeAllTrackedLootInstancesForCurrentLootRules("Serialization.Load");
		}

		for (const auto& [key, _] : _instanceAffixes) {
			_lootState.evaluatedInstances.insert(key);
		}

		SerializationLoadState::RebuildEvaluatedRecent(_lootState, kLootEvaluatedRecentKeep * 2);
		SerializationLoadState::RebuildCurrencyLedgerRecent(_lootState, kLootCurrencyLedgerMaxEntries);
		SerializationLoadState::SanitizeShuffleBagOrders(_affixRegistry, _lootState);

		SanitizeRunewordState();
		RebuildActiveCounts();
	}
