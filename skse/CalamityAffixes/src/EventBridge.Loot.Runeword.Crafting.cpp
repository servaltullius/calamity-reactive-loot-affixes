#include "CalamityAffixes/EventBridge.h"
#include "CalamityAffixes/PluginEditorIds.h"
#include "CalamityAffixes/RunewordUtil.h"
#include "EventBridge.Loot.Runeword.Detail.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <mutex>
#include <random>
#include <span>
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
			// The recipe the panel shows for the selected base, not the global cursor.
			const RunewordRecipe* recipe = _runewordState.selectedBaseKey ?
				ResolveSelectedRunewordRecipe(*_runewordState.selectedBaseKey) :
				GetCurrentRunewordRecipe();
			if (!recipe || recipe->runeTokens.empty()) {
				return;
			}
			std::uint32_t insertedRunes = 0u;
			if (_runewordState.selectedBaseKey) {
				if (const auto stateIt = _runewordState.instanceStates.find(*_runewordState.selectedBaseKey);
					stateIt != _runewordState.instanceStates.end()) {
					insertedRunes = stateIt->second.insertedRunes;
				}
			}
			if (insertedRunes >= recipe->runeTokens.size()) {
				insertedRunes = 0u;
			}

				auto* player = RE::PlayerCharacter::GetSingleton();
				if (!player) {
					return;
				}

				// The first rune the player still has fewer of than the recipe needs
				// (the panel's "missing" list); with everything owned, the next one in
				// order.
				std::uint64_t runeToken = recipe->runeTokens[insertedRunes];
				const auto requiredCounts = BuildRuneTokenCounts<16>(
					std::span<const std::uint64_t>(recipe->runeTokens.data(), recipe->runeTokens.size()),
					insertedRunes);
				for (std::size_t i = 0; i < requiredCounts.size; ++i) {
					const auto token = requiredCounts.entries[i].token;
					if (token != 0u &&
						GetOwnedRunewordFragmentCount(player, _runewordState.runeNameByToken, token) < requiredCounts.entries[i].count) {
						runeToken = token;
						break;
					}
				}
				std::string runeName = "Rune";
				if (const auto nameIt = _runewordState.runeNameByToken.find(runeToken); nameIt != _runewordState.runeNameByToken.end()) {
					runeName = nameIt->second;
				}

				const auto given = GrantRunewordFragments(player, _runewordState.runeNameByToken, runeToken, a_amount);
				if (given == 0u) {
					std::string note = "Runeword fragment item missing: ";
					note.append(runeName);
					EmitHudNotification(note.c_str());
					SKSE::log::error(
						"CalamityAffixes: runeword fragment item missing (runeToken={:016X}, runeName={}).",
						runeToken,
						runeName);
					return;
				}

				const auto owned = GetOwnedRunewordFragmentCount(player, _runewordState.runeNameByToken, runeToken);
				const std::string note = "Runeword Fragment + " + runeName + " (" + std::to_string(owned) + ")";
				EmitHudNotification(note.c_str());
			}

		void EventBridge::GrantCurrentRecipeRuneSet(std::uint32_t a_amount)
		{
			if (!_configLoaded || a_amount == 0u) {
				return;
			}

			SanitizeRunewordState();
			const RunewordRecipe* recipe = _runewordState.selectedBaseKey ?
				ResolveSelectedRunewordRecipe(*_runewordState.selectedBaseKey) :
				GetCurrentRunewordRecipe();
				if (!recipe) {
					return;
				}

				auto* player = RE::PlayerCharacter::GetSingleton();
				if (!player) {
					return;
				}

				std::uint32_t totalGiven = 0u;
				for (const auto runeToken : recipe->runeTokens) {
					const auto given = GrantRunewordFragments(player, _runewordState.runeNameByToken, runeToken, a_amount);
					const auto maxVal = std::numeric_limits<std::uint32_t>::max();
					totalGiven = (totalGiven > maxVal - given) ? maxVal : (totalGiven + given);
				}

				if (totalGiven == 0u) {
					EmitHudNotification("Runeword fragments: missing rune fragment item records.");
					SKSE::log::error("CalamityAffixes: runeword fragment items missing (grant recipe set).");
					return;
				}

				const std::string note = "Runeword Fragments + Set: " + recipe->HudName();
				EmitHudNotification(note.c_str());
			}

		std::uint32_t EventBridge::GrantReforgeOrbs(std::uint32_t a_amount)
		{
			return GrantCraftingCurrency("CAFF_Misc_ReforgeOrb", "Reforge Orbs", a_amount);
		}

		std::uint32_t EventBridge::GrantCraftingCurrency(
			std::string_view a_editorId, std::string_view a_label, std::uint32_t a_amount)
		{
			if (!_configLoaded || a_amount == 0u) {
				return 0u;
			}

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return 0u;
			}

			const std::string editorId(a_editorId);
			auto* item = PluginEditorIds::Lookup<RE::TESObjectMISC>(editorId);
			if (!item) {
				SKSE::log::error("CalamityAffixes: crafting currency item missing (editorId={}).", editorId);
				return 0u;
			}

			const auto maxGive = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
			const auto give = (a_amount > maxGive) ? maxGive : a_amount;
			player->AddObjectToContainer(item, nullptr, static_cast<std::int32_t>(give), nullptr);

			const auto owned = std::max(0, player->GetItemCount(item));
			std::string note(a_label);
			note.append(" +");
			note.append(std::to_string(give));
			note.append(" (");
			note.append(std::to_string(owned));
			note.push_back(')');
			EmitHudNotification(note.c_str());
			return give;
		}

		std::uint64_t EventBridge::PickRunewordFragmentToken()
		{
			if (_runewordState.runeTokenPool.empty()) {
				return 0u;
			}

			std::uint64_t runeToken = 0u;
			if (!_runewordState.runeTokenWeights.empty() &&
				_runewordState.runeTokenWeights.size() == _runewordState.runeTokenPool.size()) {
				std::discrete_distribution<std::size_t> pick(
					_runewordState.runeTokenWeights.begin(),
					_runewordState.runeTokenWeights.end());
				std::lock_guard<std::mutex> rngLock(_rngMutex);
				runeToken = _runewordState.runeTokenPool[pick(_rng)];
			} else {
				std::uniform_int_distribution<std::size_t> pick(0, _runewordState.runeTokenPool.size() - 1u);
				std::lock_guard<std::mutex> rngLock(_rngMutex);
				runeToken = _runewordState.runeTokenPool[pick(_rng)];
			}

			return runeToken;
		}

		bool EventBridge::TryRollRunewordFragmentToken(
			float a_sourceChanceMultiplier,
			std::uint64_t& a_outRuneToken,
			bool& a_outPityTriggered)
		{
			a_outRuneToken = 0u;
			a_outPityTriggered = false;
			if (_runewordState.runeTokenPool.empty()) {
				return false;
			}

			const float chance = std::clamp(_loot.runewordFragmentChancePercent, 0.0f, 100.0f);
			if (chance <= 0.0f) {
				return false;
			}

			const float sourceChanceMultiplier = std::max(0.0f, a_sourceChanceMultiplier);
			const float effectiveChance = std::clamp(chance * sourceChanceMultiplier, 0.0f, 100.0f);
			if (effectiveChance <= 0.0f) {
				// Source has zero multiplier — skip without touching fail streak.
				// Intentional: the player wasn't "unlucky", this source simply doesn't drop fragments.
				return false;
			}

			a_outPityTriggered = (_lootState.runewordFragmentFailStreak >= kRunewordFragmentPityFailThreshold);
			bool grant = a_outPityTriggered;
			if (!grant) {
				std::uniform_real_distribution<float> chanceDist(0.0f, 100.0f);
				{
					std::lock_guard<std::mutex> rngLock(_rngMutex);
					grant = (chanceDist(_rng) < effectiveChance);
				}
			}

			if (!grant) {
				if (_lootState.runewordFragmentFailStreak < std::numeric_limits<std::uint32_t>::max()) {
					++_lootState.runewordFragmentFailStreak;
				}
				return false;
			}

			a_outRuneToken = PickRunewordFragmentToken();
			return a_outRuneToken != 0u;
		}

		void EventBridge::CommitRunewordFragmentGrant(bool a_success)
		{
			if (a_success) {
				_lootState.runewordFragmentFailStreak = 0;
			}
		}

		bool EventBridge::TryRollReforgeOrbGrant(
			float a_sourceChanceMultiplier,
			bool& a_outPityTriggered)
		{
			a_outPityTriggered = false;
			const float chance = std::clamp(_loot.reforgeOrbChancePercent, 0.0f, 100.0f);
			if (chance <= 0.0f) {
				return false;
			}

			const float sourceChanceMultiplier = std::max(0.0f, a_sourceChanceMultiplier);
			const float effectiveChance = std::clamp(chance * sourceChanceMultiplier, 0.0f, 100.0f);
			if (effectiveChance <= 0.0f) {
				// Source has zero multiplier — skip without touching fail streak.
				return false;
			}

			a_outPityTriggered = (_lootState.reforgeOrbFailStreak >= kReforgeOrbPityFailThreshold);
			bool grant = a_outPityTriggered;
			if (!grant) {
				std::uniform_real_distribution<float> chanceDist(0.0f, 100.0f);
				{
					std::lock_guard<std::mutex> rngLock(_rngMutex);
					grant = (chanceDist(_rng) < effectiveChance);
				}
			}

			if (!grant) {
				if (_lootState.reforgeOrbFailStreak < std::numeric_limits<std::uint32_t>::max()) {
					++_lootState.reforgeOrbFailStreak;
				}
				return false;
			}

			return true;
		}

		void EventBridge::CommitReforgeOrbGrant(bool a_success)
		{
			if (a_success) {
				_lootState.reforgeOrbFailStreak = 0;
			}
		}

		bool EventBridge::TryAddLootCurrencyToCorpseInventory(
			RE::TESBoundObject* a_dropItem,
			RE::Actor* a_corpse) const
		{
			static_assert(!RuntimePolicy::kAllowWorldCurrencyPlacement);

			if (!a_dropItem || !a_corpse || !a_corpse->IsDead()) {
				return false;
			}

			const auto countInCorpse = [a_corpse, a_dropItem]() {
				auto inventory = a_corpse->GetInventory();
				const auto it = inventory.find(a_dropItem);
				return it != inventory.end() ? std::max(0, it->second.first) : 0;
			};

			const auto before = countInCorpse();
			a_corpse->AddObjectToContainer(a_dropItem, nullptr, 1, nullptr);
			const auto after = countInCorpse();
			return after > before;
		}

		void EventBridge::LogRunewordStatus() const
		{
			std::string note = "Runeword Status: ";
			if (const auto* recipe = GetCurrentRunewordRecipe()) {
				note.append(recipe->HudName());
			} else {
				note.append("No recipe");
			}

			if (_runewordState.selectedBaseKey) {
				note.append(" | Base selected");
				const auto stateIt = _runewordState.instanceStates.find(*_runewordState.selectedBaseKey);
				if (stateIt != _runewordState.instanceStates.end()) {
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
						if (const auto nameIt = _runewordState.runeNameByToken.find(token); nameIt != _runewordState.runeNameByToken.end()) {
							runeName = nameIt->second;
						}
						const auto owned = GetOwnedRunewordFragmentCount(player, _runewordState.runeNameByToken, token);
						note.push_back(' ');
						note.append(runeName);
						note.push_back('=');
						note.append(std::to_string(owned));
					}
				}

			EmitHudNotification(note.c_str());
			SKSE::log::info("CalamityAffixes: {}", note);
		}

}
