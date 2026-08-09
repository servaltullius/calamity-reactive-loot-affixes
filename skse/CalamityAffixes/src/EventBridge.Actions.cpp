#include "CalamityAffixes/EventBridge.h"

#include <string>

#include <spdlog/spdlog.h>

namespace CalamityAffixes
{
	void EventBridge::ApplyVerboseLoggingLevel() const
	{
		spdlog::set_level(_loot.debugLog ? spdlog::level::debug : spdlog::level::info);
		// Verbose sessions flush on a fixed cadence so a freeze or forced kill
		// loses at most a few seconds of tail — the 2026-08-07 freeze lost the
		// whole combat window to buffering (flush_on is warn+). Interval zero
		// tears the periodic flusher back down; the sink is basic_file_sink_mt,
		// so the flusher thread's cross-thread flush is safe.
		spdlog::flush_every(_loot.debugLog ? std::chrono::seconds(5) : std::chrono::seconds(0));
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
