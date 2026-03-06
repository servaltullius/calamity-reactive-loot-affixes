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
		std::string effectSummaryKey{};
		std::string effectSummaryText{};
		std::string effectDetailText{};
		std::string recommendedBaseKey{};
		bool selected{ false };
	};

	struct RunewordRuneRequirement
	{
		std::string runeName{};
		std::uint32_t required{ 0 };
		std::uint32_t owned{ 0 };
	};

	struct RunewordPanelState
	{
		bool hasBase{ false };
		bool hasRecipe{ false };
		bool isComplete{ false };
		std::string recipeName{};
		std::uint32_t insertedRunes{ 0 };
		std::uint32_t totalRunes{ 0 };
		std::string nextRuneName{};
		std::uint32_t nextRuneOwned{ 0 };
		bool canInsert{ false };
		std::string missingSummary{};
		std::vector<RunewordRuneRequirement> requiredRunes{};
	};

	struct OperationResult
	{
		bool success{ false };
		std::string message{};
	};
}
