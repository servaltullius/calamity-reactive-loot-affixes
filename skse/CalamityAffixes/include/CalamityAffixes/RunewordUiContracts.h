#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CalamityAffixes
{
	struct RunewordBaseInventoryEntry
	{
		std::uint64_t instanceKey{ 0 };
		std::string displayName{};
		bool selected{ false };
	};

	struct RunewordRecipeEntry
	{
		std::uint64_t recipeToken{ 0 };
		std::string displayName{};
		std::string runeSequence{};
		// Preserve recipe order and duplicates. The Prisma view uses this
		// authoritative token sequence to compare requirements with the dynamic
		// inventory snapshot without parsing display text.
		std::vector<std::uint64_t> runeTokens{};
		std::string effectSummaryKey{};
		std::string effectSummaryTextEn{};
		std::string effectSummaryTextKo{};
		std::string effectDetailTextEn{};
		std::string effectDetailTextKo{};
		std::string recommendedBaseKey{};
		bool selected{ false };
	};

	struct RunewordRuneRequirement
	{
		std::string runeName{};
		std::uint32_t required{ 0 };
		std::uint32_t owned{ 0 };
	};

	struct RunewordRuneInventoryEntry
	{
		std::uint64_t runeToken{ 0 };
		std::string runeName{};
		std::uint32_t owned{ 0 };
	};

	struct RunewordReforgeLockCandidate
	{
		std::uint64_t affixToken{ 0 };
		std::string displayNameEn{};
		std::string displayNameKo{};
		std::string slotKind{};
	};

	struct EquippedBuildEntry
	{
		std::uint64_t token{ 0 };
		std::string displayNameEn{};
		std::string displayNameKo{};
		std::string group{};
		std::string triggerKey{};
		std::string slotKind{};
		std::string suffixState{};
		std::uint32_t equippedCount{ 0 };
		bool hasPassiveContribution{ false };
		bool passiveContributionActive{ false };
		bool passiveSpellDisabled{ false };
		bool hasProcRoll{ false };
		float procRollChancePct{ 0.0f };
		bool hasLuckyHitGate{ false };
		float luckyHitGateChancePct{ 0.0f };
	};

	struct EquippedBuildSummary
	{
		bool ready{ false };
		bool runtimeEnabled{ false };
		std::uint32_t equippedAffixSlots{ 0 };
		std::vector<EquippedBuildEntry> entries{};
	};

	struct RunewordPanelState
	{
		bool hasBase{ false };
		bool hasRecipe{ false };
		bool isComplete{ false };
		std::string recipeName{};
		std::uint64_t recipeToken{ 0 };
		std::uint32_t insertedRunes{ 0 };
		std::uint32_t totalRunes{ 0 };
		std::string nextRuneName{};
		std::uint32_t nextRuneOwned{ 0 };
		bool canInsert{ false };
		std::string missingSummary{};
		bool baseCompatibilityWarning{ false };
		std::string baseCompatibilityMessageEn{};
		std::string baseCompatibilityMessageKo{};
		std::vector<RunewordRuneRequirement> requiredRunes{};
		// False means the snapshot is unavailable and consumers must not treat
		// the empty vector as an inventory containing zero fragments.
		bool runeInventoryKnown{ false };
		std::uint32_t runeInventoryExpectedCount{ 0 };
		std::vector<RunewordRuneInventoryEntry> runeInventory{};
		std::uint32_t regularAffixCount{ 0 };
		// Counts are authoritative only when their corresponding known flag is
		// true. This keeps an unavailable snapshot distinct from a real zero.
		bool reforgeOrbsKnown{ false };
		std::uint32_t reforgeOrbsOwned{ 0 };
		bool pityKnown{ false };
		// A streak equal to its threshold means the next eligible ordinary
		// currency evaluation is guaranteed. It is not a kill/drop countdown.
		std::uint32_t runewordFragmentFailStreak{ 0 };
		std::uint32_t runewordFragmentFailStreakThreshold{ 0 };
		std::uint32_t reforgeOrbFailStreak{ 0 };
		std::uint32_t reforgeOrbFailStreakThreshold{ 0 };
		std::uint32_t standardReforgeCost{ 0 };
		std::uint32_t lockedReforgeCost{ 0 };
		std::vector<RunewordReforgeLockCandidate> reforgeLockCandidates{};
		EquippedBuildSummary equippedBuild{};
		// Gates the panel's cheat-adjacent debug tools; true only while a debug
		// toggle (HUD notifications or verbose logging) is enabled.
		bool debugTools{ false };
	};

	struct OperationResult
	{
		bool success{ false };
		std::string message{};
	};
}
