#if !defined(CALAMITYAFFIXES_EVENTBRIDGE_STATE_GROUPS_INL_CONTEXT)
#error "Do not include EventBridge.StateGroups.inl directly; include CalamityAffixes/EventBridge.h"
#endif

		struct AffixRuntimeCacheState
		{
			std::vector<AffixRuntime> affixes{};
			std::vector<std::uint32_t> activeCounts{};
			float activeCritDamageBonusPct{ 0.0f };
			AffixRegistryState affixRegistry{};
			std::vector<std::size_t> activeHitTriggerAffixIndices{};
			std::vector<std::size_t> activeIncomingHitTriggerAffixIndices{};
			std::vector<std::size_t> activeDotApplyTriggerAffixIndices{};
			std::vector<std::size_t> activeKillTriggerAffixIndices{};
			std::vector<std::size_t> activeLowHealthTriggerAffixIndices{};
		};

		struct InstanceTrackingState
		{
			std::unordered_set<RE::SpellItem*> appliedPassiveSpells{};
			std::unordered_map<std::uint64_t, InstanceAffixSlots> instanceAffixes{};
			std::unordered_map<InstanceStateKey, InstanceRuntimeState, InstanceStateKeyHash> instanceStates{};
			std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> equippedInstanceKeysByToken{};
			bool equippedTokenCacheReady{ false };
		};
