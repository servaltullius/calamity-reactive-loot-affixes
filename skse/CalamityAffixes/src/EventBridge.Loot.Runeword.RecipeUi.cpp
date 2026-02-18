#include "CalamityAffixes/EventBridge.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

	bool EventBridge::SelectRunewordRecipe(std::uint64_t a_recipeToken)
	{
		if (!_configLoaded || a_recipeToken == 0u || _runewordRecipes.empty()) {
			return false;
		}

		SanitizeRunewordState();
		const auto it = _runewordRecipeIndexByToken.find(a_recipeToken);
		if (it == _runewordRecipeIndexByToken.end() || it->second >= _runewordRecipes.size()) {
			return false;
		}

		const auto idx = it->second;
		_runewordRecipeCycleCursor = static_cast<std::uint32_t>(idx);
		const auto& recipe = _runewordRecipes[idx];

		if (_runewordSelectedBaseKey) {
			const auto selectedKey = *_runewordSelectedBaseKey;
			const bool completedBase = ResolveCompletedRunewordRecipe(selectedKey) != nullptr;

			if (completedBase) {
				_runewordInstanceStates.erase(selectedKey);
			} else {
				auto& state = _runewordInstanceStates[selectedKey];
				if (state.recipeToken != recipe.token) {
					state.recipeToken = recipe.token;
					state.insertedRunes = 0u;
				}
			}
		}

		std::string runes;
		for (std::size_t i = 0; i < recipe.runeIds.size(); ++i) {
			if (i > 0) {
				runes.push_back('-');
			}
			runes.append(recipe.runeIds[i]);
		}

		std::string note = "Runeword Recipe: " + recipe.displayName + " [" + runes + "]";
		if (_runewordSelectedBaseKey) {
			const auto baseType = ResolveInstanceLootType(*_runewordSelectedBaseKey);
			if (recipe.recommendedBaseType && baseType && *recipe.recommendedBaseType != *baseType) {
				note.append(" (Recommended ");
				note.append(DescribeLootItemType(*recipe.recommendedBaseType));
				note.append(", current ");
				note.append(DescribeLootItemType(*baseType));
				note.append(")");
			}
		}

		RE::DebugNotification(note.c_str());
		return true;
	}
}
