#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes
{
	void EventBridge::IndexAffixLookupKeys(
		const AffixRuntime& a_affix,
		std::size_t a_index,
		bool a_useSynthesizedDuplicateLogFormat,
		bool a_warnOnDuplicate)
	{
		if (!a_affix.id.empty()) {
			const auto [it, inserted] = _affixIndexById.emplace(a_affix.id, a_index);
			if (!inserted) {
				if (a_useSynthesizedDuplicateLogFormat) {
					if (a_warnOnDuplicate) {
						SKSE::log::warn(
							"CalamityAffixes: synthesized affix id duplicated (id='{}', firstIdx={}, dupIdx={}).",
							a_affix.id,
							it->second,
							a_index);
					}
				} else {
					SKSE::log::error(
						"CalamityAffixes: duplicate affix id '{}' in runtime config (firstIdx={}, dupIdx={}).",
						a_affix.id,
						it->second,
						a_index);
				}
			}
		}

		if (a_affix.token != 0u) {
			const auto [it, inserted] = _affixIndexByToken.emplace(a_affix.token, a_index);
			if (!inserted) {
				if (a_useSynthesizedDuplicateLogFormat) {
					if (a_warnOnDuplicate) {
						SKSE::log::warn(
							"CalamityAffixes: synthesized affix token duplicated (id='{}', firstIdx={}, dupIdx={}).",
							a_affix.id,
							it->second,
							a_index);
					}
				} else {
					SKSE::log::error(
						"CalamityAffixes: duplicate affix token in runtime config (id='{}', firstIdx={}, dupIdx={}).",
						a_affix.id,
						it->second,
						a_index);
				}
			}
		}
	}

	void EventBridge::IndexAffixLootPool(const AffixRuntime& a_affix, std::size_t a_index)
	{
		if (!a_affix.lootType) {
			return;
		}

		if (a_affix.slot == AffixSlot::kSuffix) {
			if (*a_affix.lootType == LootItemType::kWeapon) {
				_lootWeaponSuffixes.push_back(a_index);
			} else if (*a_affix.lootType == LootItemType::kArmor) {
				_lootArmorSuffixes.push_back(a_index);
			}
		} else {
			if (*a_affix.lootType == LootItemType::kWeapon) {
				_lootWeaponAffixes.push_back(a_index);
			} else if (*a_affix.lootType == LootItemType::kArmor) {
				_lootArmorAffixes.push_back(a_index);
			}
		}
	}

	void EventBridge::IndexAffixTriggerBucket(const AffixRuntime& a_affix, std::size_t a_index)
	{
		const bool isTriggerAction =
			a_affix.action.type == ActionType::kDebugNotify ||
			a_affix.action.type == ActionType::kCastSpell ||
			a_affix.action.type == ActionType::kCastSpellAdaptiveElement ||
			a_affix.action.type == ActionType::kSpawnTrap;
		if (!isTriggerAction) {
			return;
		}

		switch (a_affix.trigger) {
		case Trigger::kHit:
			_hitTriggerAffixIndices.push_back(a_index);
			break;
		case Trigger::kIncomingHit:
			_incomingHitTriggerAffixIndices.push_back(a_index);
			break;
		case Trigger::kDotApply:
			_dotApplyTriggerAffixIndices.push_back(a_index);
			break;
		case Trigger::kKill:
			_killTriggerAffixIndices.push_back(a_index);
			break;
		case Trigger::kLowHealth:
			_lowHealthTriggerAffixIndices.push_back(a_index);
			break;
		}
	}

	void EventBridge::IndexAffixSpecialActionBucket(const AffixRuntime& a_affix, std::size_t a_index)
	{
		if (a_affix.action.type == ActionType::kCastOnCrit) {
			_castOnCritAffixIndices.push_back(a_index);
		} else if (a_affix.action.type == ActionType::kConvertDamage) {
			_convertAffixIndices.push_back(a_index);
		} else if (a_affix.action.type == ActionType::kMindOverMatter) {
			_mindOverMatterAffixIndices.push_back(a_index);
		} else if (a_affix.action.type == ActionType::kArchmage) {
			_archmageAffixIndices.push_back(a_index);
		} else if (a_affix.action.type == ActionType::kCorpseExplosion) {
			_corpseExplosionAffixIndices.push_back(a_index);
		} else if (a_affix.action.type == ActionType::kSummonCorpseExplosion) {
			_summonCorpseExplosionAffixIndices.push_back(a_index);
		}
	}
}
