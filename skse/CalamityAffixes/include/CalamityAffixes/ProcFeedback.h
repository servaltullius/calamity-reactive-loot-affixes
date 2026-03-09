#pragma once

#include <array>
#include <string_view>

#include <RE/Skyrim.h>

#include "CalamityAffixes/PointerSafety.h"

namespace CalamityAffixes::ProcFeedback
{
	inline constexpr std::string_view kBloomSpellEditorIdPrefix = "CAFF_SPEL_DOT_BLOOM_";

	[[nodiscard]] inline RE::BGSArtObject* ResolveSpellFeedbackArtFromSpell(const RE::SpellItem* a_source) noexcept
	{
		if (!a_source) {
			return nullptr;
		}

		for (const auto* effect : a_source->effects) {
			if (!effect || !effect->baseEffect) {
				continue;
			}

			if (auto* art = effect->baseEffect->data.hitEffectArt) {
				return art;
			}
			if (auto* art = effect->baseEffect->data.enchantEffectArt) {
				return art;
			}
		}

		return nullptr;
	}

	[[nodiscard]] inline bool IsBloomProcSpell(const RE::SpellItem* a_spell) noexcept
	{
		if (!a_spell) {
			return false;
		}

		const auto editorId = SafeCStringView(a_spell->GetFormEditorID());
		return !editorId.empty() && editorId.starts_with(kBloomSpellEditorIdPrefix);
	}

	[[nodiscard]] inline std::string_view ResolveBloomProcDebugLabel(const RE::SpellItem* a_spell) noexcept
	{
		if (!a_spell) {
			return "Bloom";
		}

		const auto editorId = SafeCStringView(a_spell->GetFormEditorID());
		if (editorId.find("POISON") != std::string_view::npos) {
			return "Plague Spore";
		}
		if (editorId.find("TAR") != std::string_view::npos) {
			return "Tar Blight";
		}
		if (editorId.find("SIPHON") != std::string_view::npos) {
			return "Siphon Spore";
		}

		return "Bloom";
	}

	[[nodiscard]] inline RE::BGSArtObject* ResolveSpellFeedbackArt(const RE::SpellItem* a_spell) noexcept
	{
		if (auto* art = ResolveSpellFeedbackArtFromSpell(a_spell)) {
			return art;
		}

		static RE::BGSArtObject* fallback = []() noexcept -> RE::BGSArtObject* {
			constexpr std::array<RE::FormID, 2> kFallbackSpellFormIDs{
				0x00012FD0,
				0x0002B96C
			};
			for (const auto formID : kFallbackSpellFormIDs) {
				auto* spell = RE::TESForm::LookupByID<RE::SpellItem>(formID);
				if (auto* art = ResolveSpellFeedbackArtFromSpell(spell)) {
					return art;
				}
			}
			return nullptr;
		}();

		return fallback;
	}

	[[nodiscard]] inline RE::FormID ResolveSpellFeedbackSoundFormID(const RE::SpellItem* a_spell) noexcept
	{
		if (!a_spell) {
			return 0u;
		}

		const auto resolveSound = [a_spell](RE::MagicSystem::SoundID a_soundID) noexcept -> RE::FormID {
			for (const auto* effect : a_spell->effects) {
				if (!effect || !effect->baseEffect) {
					continue;
				}

				for (const auto& soundPair : effect->baseEffect->effectSounds) {
					if (soundPair.id == a_soundID && soundPair.sound) {
						return soundPair.sound->GetFormID();
					}
				}
			}

			return 0u;
		};

		if (const auto hitSound = resolveSound(RE::MagicSystem::SoundID::kHit); hitSound != 0u) {
			return hitSound;
		}

		return resolveSound(RE::MagicSystem::SoundID::kRelease);
	}

	inline void PlayBloomProcFeedback(
		RE::Actor* a_target,
		const RE::SpellItem* a_spell,
		float a_durationSeconds,
		bool a_playSound) noexcept
	{
		if (!a_target || !a_spell || !IsBloomProcSpell(a_spell)) {
			return;
		}

		if (auto* art = ResolveSpellFeedbackArt(a_spell)) {
			a_target->InstantiateHitArt(
				art,
				a_durationSeconds,
				nullptr,
				false,
				false,
				nullptr,
				false);
		}

		if (!a_playSound) {
			return;
		}

		auto* audioManager = RE::BSAudioManager::GetSingleton();
		if (!audioManager) {
			return;
		}

		if (const auto soundFormID = ResolveSpellFeedbackSoundFormID(a_spell); soundFormID != 0u) {
			audioManager->Play(soundFormID);
		}
	}
}
