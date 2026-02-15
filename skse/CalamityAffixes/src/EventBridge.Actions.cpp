#include "CalamityAffixes/EventBridge.h"

#include <string>

namespace CalamityAffixes
{
	void EventBridge::ExecuteDebugNotifyAction(const Action& a_action)
	{
		const std::string msg = a_action.text.empty() ? "CalamityAffixes proc" : ("CalamityAffixes proc: " + a_action.text);
		if (auto* console = RE::ConsoleLog::GetSingleton()) {
			console->Print(msg.c_str());
		}
	}

	void EventBridge::DispatchActionByType(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		const auto& a_action = a_affix.action;
		switch (a_action.type) {
		case ActionType::kDebugNotify:
			ExecuteDebugNotifyAction(a_action);
			break;
		case ActionType::kCastSpell:
			ExecuteCastSpellAction(a_affix, a_owner, a_target, a_hitData);
			break;
		case ActionType::kCastSpellAdaptiveElement:
			ExecuteCastSpellAdaptiveElementAction(a_action, a_owner, a_target, a_hitData);
			break;
		case ActionType::kSpawnTrap:
			ExecuteSpawnTrapAction(a_action, a_owner, a_target, a_hitData);
			break;
		default:
			break;
		}
	}

	void EventBridge::ExecuteActionWithProcDepthGuard(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		_procDepth += 1;
		ExecuteAction(a_affix, a_owner, a_target, a_hitData);
		_procDepth -= 1;
	}

	void EventBridge::ExecuteAction(const AffixRuntime& a_affix, RE::Actor* a_owner, RE::Actor* a_target, const RE::HitData* a_hitData)
	{
		if (!a_owner) {
			return;
		}

		DispatchActionByType(a_affix, a_owner, a_target, a_hitData);
	}

}
