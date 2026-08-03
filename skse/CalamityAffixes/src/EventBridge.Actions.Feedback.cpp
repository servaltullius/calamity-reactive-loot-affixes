#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes
{
	void EventBridge::PlayActionFeedback(
		const Action& a_action,
		RE::Actor* a_owner,
		RE::Actor* a_target,
		ActionFeedbackPlayOn a_playOn,
		RE::Actor* a_corpse) const noexcept
	{
		const auto& feedback = a_action.feedback;
		if (feedback.playOn != a_playOn) {
			return;
		}

		auto* recipient = feedback.target == ActionFeedbackTarget::kTarget ? a_target :
			feedback.target == ActionFeedbackTarget::kCorpse ? a_corpse : a_owner;
		if (!recipient) {
			return;
		}

		if (feedback.art && feedback.durationSeconds > 0.0f && recipient->Is3DLoaded()) {
			const auto* instantiated = recipient->InstantiateHitArt(
				feedback.art,
				feedback.durationSeconds,
				nullptr,
				false,
				false,
				nullptr,
				false);
			if (_loot.debugLog) {
				SKSE::log::debug(
					"CalamityAffixes: action feedback art (art=0x{:X}, recipient=0x{:X}, instantiated={}).",
					feedback.art->GetFormID(),
					recipient->GetFormID(),
					instantiated != nullptr);
			}
		} else if (feedback.art && _loot.debugLog) {
			SKSE::log::debug(
				"CalamityAffixes: action feedback art skipped (art=0x{:X}, recipient=0x{:X}, is3DLoaded={}).",
				feedback.art->GetFormID(),
				recipient->GetFormID(),
				recipient->Is3DLoaded());
		}

		if (!feedback.sound) {
			return;
		}

		if (feedback.spatialSound) {
			PlaySpatialSound(feedback.sound, recipient->GetPosition());
		} else if (auto* audioManager = RE::BSAudioManager::GetSingleton()) {
			audioManager->Play(feedback.sound->GetFormID());
		}
	}

	void EventBridge::PlaySpatialSound(
		RE::BGSSoundDescriptorForm* a_sound,
		const RE::NiPoint3& a_position) const noexcept
	{
		auto* audioManager = RE::BSAudioManager::GetSingleton();
		if (!a_sound || !audioManager) {
			return;
		}

		RE::BSSoundHandle handle{};
		if (!audioManager->BuildSoundDataFromDescriptor(handle, a_sound)) {
			return;
		}
		handle.SetPosition(a_position);
		handle.Play();
	}
}
