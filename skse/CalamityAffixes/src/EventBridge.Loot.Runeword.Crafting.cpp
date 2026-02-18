#include "CalamityAffixes/EventBridge.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
#include <string>

namespace CalamityAffixes
{
	using namespace RunewordDetail;

		void EventBridge::GrantNextRequiredRuneFragment(std::uint32_t a_amount)
		{
			if (!_configLoaded || a_amount == 0u) {
				return;
			}

			SanitizeRunewordState();
			const RunewordRecipe* recipe = nullptr;
			std::uint32_t nextIndex = 0u;
			if (_runewordSelectedBaseKey) {
				const auto stateIt = _runewordInstanceStates.find(*_runewordSelectedBaseKey);
				if (stateIt != _runewordInstanceStates.end()) {
					recipe = FindRunewordRecipeByToken(stateIt->second.recipeToken);
					nextIndex = stateIt->second.insertedRunes;
				}
			}

			if (!recipe) {
				recipe = GetCurrentRunewordRecipe();
			}
			if (!recipe || recipe->runeTokens.empty()) {
				return;
			}

				if (nextIndex >= recipe->runeTokens.size()) {
					nextIndex = 0u;
				}

				const auto runeToken = recipe->runeTokens[nextIndex];
				std::string runeName = "Rune";
				if (const auto nameIt = _runewordRuneNameByToken.find(runeToken); nameIt != _runewordRuneNameByToken.end()) {
					runeName = nameIt->second;
				}

				auto* player = RE::PlayerCharacter::GetSingleton();
				if (!player) {
					return;
				}

				const auto given = GrantRunewordFragments(player, _runewordRuneNameByToken, runeToken, a_amount);
				if (given == 0u) {
					std::string note = "Runeword fragment item missing: ";
					note.append(runeName);
					RE::DebugNotification(note.c_str());
					SKSE::log::error(
						"CalamityAffixes: runeword fragment item missing (runeToken={:016X}, runeName={}).",
						runeToken,
						runeName);
					return;
				}

				const auto owned = GetOwnedRunewordFragmentCount(player, _runewordRuneNameByToken, runeToken);
				const std::string note = "Runeword Fragment + " + runeName + " (" + std::to_string(owned) + ")";
				RE::DebugNotification(note.c_str());
			}

		void EventBridge::GrantCurrentRecipeRuneSet(std::uint32_t a_amount)
		{
			if (!_configLoaded || a_amount == 0u) {
				return;
			}

			const auto* recipe = GetCurrentRunewordRecipe();
				if (!recipe) {
					return;
				}

				auto* player = RE::PlayerCharacter::GetSingleton();
				if (!player) {
					return;
				}

				std::uint32_t totalGiven = 0u;
				for (const auto runeToken : recipe->runeTokens) {
					const auto given = GrantRunewordFragments(player, _runewordRuneNameByToken, runeToken, a_amount);
					const auto maxVal = std::numeric_limits<std::uint32_t>::max();
					totalGiven = (totalGiven > maxVal - given) ? maxVal : (totalGiven + given);
				}

				if (totalGiven == 0u) {
					RE::DebugNotification("Runeword fragments: missing rune fragment item records.");
					SKSE::log::error("CalamityAffixes: runeword fragment items missing (grant recipe set).");
					return;
				}

				const std::string note = "Runeword Fragments + Set: " + recipe->displayName;
				RE::DebugNotification(note.c_str());
			}

		void EventBridge::MaybeGrantRandomRunewordFragment()
		{
			if (_runewordRuneTokenPool.empty()) {
				return;
			}

			const float chance = std::clamp(_loot.runewordFragmentChancePercent, 0.0f, 100.0f);
			if (chance <= 0.0f) {
				return;
			}

			std::uniform_real_distribution<float> chanceDist(0.0f, 100.0f);
			if (chanceDist(_rng) >= chance) {
				return;
			}

				std::uint64_t runeToken = 0u;
				if (!_runewordRuneTokenWeights.empty() && _runewordRuneTokenWeights.size() == _runewordRuneTokenPool.size()) {
					std::discrete_distribution<std::size_t> pick(_runewordRuneTokenWeights.begin(), _runewordRuneTokenWeights.end());
					runeToken = _runewordRuneTokenPool[pick(_rng)];
				} else {
					std::uniform_int_distribution<std::size_t> pick(0, _runewordRuneTokenPool.size() - 1u);
					runeToken = _runewordRuneTokenPool[pick(_rng)];
				}

				std::string runeName = "Rune";
				if (const auto nameIt = _runewordRuneNameByToken.find(runeToken); nameIt != _runewordRuneNameByToken.end()) {
					runeName = nameIt->second;
				}

				auto* player = RE::PlayerCharacter::GetSingleton();
				if (!player) {
					return;
				}

				const auto given = GrantRunewordFragments(player, _runewordRuneNameByToken, runeToken, 1u);
				if (given == 0u) {
					SKSE::log::error(
						"CalamityAffixes: runeword fragment item missing (runeToken={:016X}, runeName={}).",
						runeToken,
						runeName);
					return;
				}

				const auto owned = GetOwnedRunewordFragmentCount(player, _runewordRuneNameByToken, runeToken);

				std::string note = "Runeword Fragment: ";
				note.append(runeName);
				note.append(" (");
				note.append(std::to_string(owned));
				note.push_back(')');
				RE::DebugNotification(note.c_str());
			}

		void EventBridge::MaybeGrantRandomReforgeOrb()
		{
			const float chance = std::clamp(_loot.reforgeOrbChancePercent, 0.0f, 100.0f);
			if (chance <= 0.0f) {
				return;
			}

			std::uniform_real_distribution<float> chanceDist(0.0f, 100.0f);
			if (chanceDist(_rng) >= chance) {
				return;
			}

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return;
			}

			auto* orb = RE::TESForm::LookupByEditorID<RE::TESObjectMISC>("CAFF_Misc_ReforgeOrb");
			if (!orb) {
				SKSE::log::error("CalamityAffixes: reforge orb item missing (editorId=CAFF_Misc_ReforgeOrb).");
				return;
			}

			player->AddObjectToContainer(orb, nullptr, 1, nullptr);
			const auto owned = std::max(0, player->GetItemCount(orb));

			std::string note = "Reforge Orb (";
			note.append(std::to_string(owned));
			note.push_back(')');
			RE::DebugNotification(note.c_str());
		}

		void EventBridge::LogRunewordStatus() const
		{
			std::string note = "Runeword Status: ";
			if (const auto* recipe = GetCurrentRunewordRecipe()) {
				note.append(recipe->displayName);
			} else {
				note.append("No recipe");
			}

			if (_runewordSelectedBaseKey) {
				note.append(" | Base selected");
				const auto stateIt = _runewordInstanceStates.find(*_runewordSelectedBaseKey);
				if (stateIt != _runewordInstanceStates.end()) {
					const auto* recipe = FindRunewordRecipeByToken(stateIt->second.recipeToken);
					if (recipe) {
						note.append(" | ");
						note.append(std::to_string(stateIt->second.insertedRunes));
						note.push_back('/');
						note.append(std::to_string(recipe->runeTokens.size()));
					}
				}
			}

				if (const auto* recipe = GetCurrentRunewordRecipe()) {
					note.append(" | Runes:");
					auto* player = RE::PlayerCharacter::GetSingleton();
					for (const auto token : recipe->runeTokens) {
						std::string runeName = "Rune";
						if (const auto nameIt = _runewordRuneNameByToken.find(token); nameIt != _runewordRuneNameByToken.end()) {
							runeName = nameIt->second;
						}
						const auto owned = GetOwnedRunewordFragmentCount(player, _runewordRuneNameByToken, token);
						note.push_back(' ');
						note.append(runeName);
						note.push_back('=');
						note.append(std::to_string(owned));
					}
				}

			RE::DebugNotification(note.c_str());
			SKSE::log::info("CalamityAffixes: {}", note);
		}

}
