#include "CalamityAffixes/EventBridge.h"

#include <string>

#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	void EventBridge::ApplyVerboseLoggingLevel() const
	{
		spdlog::set_level(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
	}

	void EventBridge::EmitHudNotification(const char* a_message) const
	{
		if (!a_message || *a_message == '\0') {
			return;
		}

		RE::DebugNotification(a_message);
	}

	void EventBridge::EmitDebugHudNotification(const char* a_message) const
	{
		if (!_loot.debugHudNotifications) {
			return;
		}

		EmitHudNotification(a_message);
	}

	void EventBridge::ExecuteDebugNotifyAction(const Action& a_action)
	{
		const std::string msg = a_action.text.empty() ? "CalamityAffixes proc" : ("CalamityAffixes proc: " + a_action.text);
		if (auto* console = RE::ConsoleLog::GetSingleton()) {
			console->Print(msg.c_str());
		}
	}
}
