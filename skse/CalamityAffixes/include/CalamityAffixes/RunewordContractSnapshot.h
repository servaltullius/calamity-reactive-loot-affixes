#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace CalamityAffixes
{
	struct RunewordContractRecipeRow
	{
		std::string id{};
		std::string displayName{};
		std::vector<std::string> runeNames{};
		std::string resultAffixId{};
		std::optional<bool> recommendedBaseIsWeapon{};
	};

	struct RunewordContractSnapshot
	{
		std::vector<RunewordContractRecipeRow> recipes{};
		std::unordered_map<std::string, double> runeWeightsByName{};
	};

	[[nodiscard]] bool LoadRunewordContractSnapshotFromRuntime(
		std::string_view a_relativePath,
		RunewordContractSnapshot& a_outSnapshot);
}
