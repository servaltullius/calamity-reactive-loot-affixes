#pragma once

#include <cstdint>
#include <string_view>

namespace CalamityAffixes::detail
{
	enum class EquippedBuildTriggerKind : std::uint8_t
	{
		kHit,
		kIncomingHit,
		kDotApply,
		kKill,
		kLowHealth,
	};

	enum class EquippedBuildSlotKind : std::uint8_t
	{
		kPrefix,
		kSuffix,
		kRuneword,
	};

	enum class EquippedBuildProcLane : std::uint8_t
	{
		kNone,
		kStandard,
		kSpecial,
	};

	enum class EquippedBuildGroup : std::uint8_t
	{
		kOffense,
		kDefense,
		kKill,
		kPassive,
	};

	enum class EquippedBuildTriggerKey : std::uint8_t
	{
		kHit,
		kIncomingHit,
		kDotApply,
		kKill,
		kLowHealth,
		kPassive,
	};

	enum class EquippedBuildSuffixState : std::uint8_t
	{
		kNone,
		kStacking,
		kHighest,
		kSuppressed,
	};

	struct EquippedBuildPolicyInput
	{
		EquippedBuildTriggerKind trigger{ EquippedBuildTriggerKind::kHit };
		EquippedBuildSlotKind slot{ EquippedBuildSlotKind::kRuneword };
		EquippedBuildProcLane procLane{ EquippedBuildProcLane::kNone };
		bool hasPassiveContribution{ false };
		bool isDebugNotify{ false };
		float configuredProcChancePct{ 0.0f };
		float luckyHitChancePct{ 0.0f };
	};

	struct EquippedBuildPassiveContributionInput
	{
		bool hasPassiveSpell{ false };
		bool passiveSpellsDisabled{ false };
		bool hasCritContribution{ false };
		bool hasScrollContribution{ false };
		bool suffixFamilySuppressed{ false };
	};

	struct EquippedBuildPassiveContributionState
	{
		bool hasPassiveContribution{ false };
		bool passiveContributionActive{ false };
		bool passiveSpellDisabled{ false };
	};

	[[nodiscard]] constexpr EquippedBuildPassiveContributionState ResolveEquippedBuildPassiveContributionState(
		EquippedBuildPassiveContributionInput a_input) noexcept
	{
		return {
			.hasPassiveContribution =
				a_input.hasPassiveSpell ||
				a_input.hasCritContribution ||
				a_input.hasScrollContribution,
			.passiveContributionActive =
				(a_input.hasPassiveSpell &&
					!a_input.passiveSpellsDisabled &&
					!a_input.suffixFamilySuppressed) ||
				(a_input.hasCritContribution && !a_input.suffixFamilySuppressed) ||
				a_input.hasScrollContribution,
			.passiveSpellDisabled =
				a_input.hasPassiveSpell && a_input.passiveSpellsDisabled,
		};
	}

	[[nodiscard]] constexpr bool ResolveEquippedBuildSummaryReady(
		bool a_configLoaded,
		bool a_runtimeEnabled,
		bool a_equippedCacheReady,
		bool a_activeCountShapeMatches) noexcept
	{
		return a_configLoaded &&
			(!a_runtimeEnabled || (a_equippedCacheReady && a_activeCountShapeMatches));
	}

	[[nodiscard]] constexpr bool HasEquippedBuildProcRoll(EquippedBuildPolicyInput a_input) noexcept
	{
		return a_input.procLane != EquippedBuildProcLane::kNone &&
			a_input.configuredProcChancePct > 0.0f;
	}

	[[nodiscard]] constexpr bool ShouldShowEquippedBuildEntry(EquippedBuildPolicyInput a_input) noexcept
	{
		// DebugNotify-only helpers are internal wiring. A helper that also owns a
		// real passive/stat contribution remains player-visible.
		return !a_input.isDebugNotify || a_input.hasPassiveContribution;
	}

	[[nodiscard]] constexpr bool IsEquippedBuildPassive(EquippedBuildPolicyInput a_input) noexcept
	{
		return a_input.slot == EquippedBuildSlotKind::kSuffix ||
			(a_input.hasPassiveContribution && !HasEquippedBuildProcRoll(a_input));
	}

	[[nodiscard]] constexpr EquippedBuildGroup ResolveEquippedBuildGroup(
		EquippedBuildPolicyInput a_input) noexcept
	{
		if (IsEquippedBuildPassive(a_input)) {
			return EquippedBuildGroup::kPassive;
		}

		switch (a_input.trigger) {
		case EquippedBuildTriggerKind::kIncomingHit:
		case EquippedBuildTriggerKind::kLowHealth:
			return EquippedBuildGroup::kDefense;
		case EquippedBuildTriggerKind::kKill:
			return EquippedBuildGroup::kKill;
		case EquippedBuildTriggerKind::kHit:
		case EquippedBuildTriggerKind::kDotApply:
		default:
			return EquippedBuildGroup::kOffense;
		}
	}

	[[nodiscard]] constexpr EquippedBuildTriggerKey ResolveEquippedBuildTriggerKey(
		EquippedBuildPolicyInput a_input) noexcept
	{
		if (IsEquippedBuildPassive(a_input)) {
			return EquippedBuildTriggerKey::kPassive;
		}

		switch (a_input.trigger) {
		case EquippedBuildTriggerKind::kIncomingHit:
			return EquippedBuildTriggerKey::kIncomingHit;
		case EquippedBuildTriggerKind::kDotApply:
			return EquippedBuildTriggerKey::kDotApply;
		case EquippedBuildTriggerKind::kKill:
			return EquippedBuildTriggerKey::kKill;
		case EquippedBuildTriggerKind::kLowHealth:
			return EquippedBuildTriggerKey::kLowHealth;
		case EquippedBuildTriggerKind::kHit:
		default:
			return EquippedBuildTriggerKey::kHit;
		}
	}

	[[nodiscard]] constexpr bool HasEquippedBuildLuckyHitGate(EquippedBuildPolicyInput a_input) noexcept
	{
		const bool triggerUsesHitData =
			a_input.trigger == EquippedBuildTriggerKind::kHit ||
			a_input.trigger == EquippedBuildTriggerKind::kIncomingHit;
		return triggerUsesHitData && a_input.luckyHitChancePct > 0.0f;
	}

	[[nodiscard]] constexpr EquippedBuildSuffixState ResolveEquippedBuildSuffixState(
		bool a_isSuffix,
		bool a_hasFamily,
		bool a_isFamilyWinner) noexcept
	{
		if (!a_isSuffix) {
			return EquippedBuildSuffixState::kNone;
		}
		if (!a_hasFamily) {
			return EquippedBuildSuffixState::kStacking;
		}
		return a_isFamilyWinner ? EquippedBuildSuffixState::kHighest : EquippedBuildSuffixState::kSuppressed;
	}

	[[nodiscard]] constexpr std::string_view DescribeEquippedBuildGroup(EquippedBuildGroup a_group) noexcept
	{
		switch (a_group) {
		case EquippedBuildGroup::kDefense:
			return "defense";
		case EquippedBuildGroup::kKill:
			return "kill";
		case EquippedBuildGroup::kPassive:
			return "passive";
		case EquippedBuildGroup::kOffense:
		default:
			return "offense";
		}
	}

	[[nodiscard]] constexpr std::string_view DescribeEquippedBuildTriggerKey(
		EquippedBuildTriggerKey a_trigger) noexcept
	{
		switch (a_trigger) {
		case EquippedBuildTriggerKey::kIncomingHit:
			return "incomingHit";
		case EquippedBuildTriggerKey::kDotApply:
			return "dotApply";
		case EquippedBuildTriggerKey::kKill:
			return "kill";
		case EquippedBuildTriggerKey::kLowHealth:
			return "lowHealth";
		case EquippedBuildTriggerKey::kPassive:
			return "passive";
		case EquippedBuildTriggerKey::kHit:
		default:
			return "hit";
		}
	}

	[[nodiscard]] constexpr std::string_view DescribeEquippedBuildSlotKind(
		EquippedBuildSlotKind a_slot) noexcept
	{
		switch (a_slot) {
		case EquippedBuildSlotKind::kPrefix:
			return "prefix";
		case EquippedBuildSlotKind::kSuffix:
			return "suffix";
		case EquippedBuildSlotKind::kRuneword:
		default:
			return "runeword";
		}
	}

	[[nodiscard]] constexpr std::string_view DescribeEquippedBuildSuffixState(
		EquippedBuildSuffixState a_state) noexcept
	{
		switch (a_state) {
		case EquippedBuildSuffixState::kStacking:
			return "stacking";
		case EquippedBuildSuffixState::kHighest:
			return "highest";
		case EquippedBuildSuffixState::kSuppressed:
			return "suppressed";
		case EquippedBuildSuffixState::kNone:
		default:
			return "none";
		}
	}
}
