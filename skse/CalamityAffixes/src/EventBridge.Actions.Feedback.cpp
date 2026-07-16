#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes
{
	void EventBridge::PlayActionFeedback(
		const Action& a_action,
		RE::Actor* a_owner,
		RE::Actor* a_target,
		ActionFeedbackPlayOn a_playOn) const noexcept
	{
		const auto& feedback = a_action.feedback;
		if (feedback.playOn != a_playOn) {
			return;
		}

		auto* recipient = feedback.target == ActionFeedbackTarget::kTarget ? a_target : a_owner;
		if (!recipient) {
			return;
		}

		if (feedback.art && feedback.durationSeconds > 0.0f) {
			recipient->InstantiateHitArt(
				feedback.art,
				feedback.durationSeconds,
				nullptr,
				false,
				false,
				nullptr,
				false);
		}

		if (!feedback.sound) {
			return;
		}

		if (auto* audioManager = RE::BSAudioManager::GetSingleton()) {
			audioManager->Play(feedback.sound->GetFormID());
		}
	}
}
