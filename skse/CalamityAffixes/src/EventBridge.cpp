#include "CalamityAffixes/EventBridge.h"

#include <memory>

namespace CalamityAffixes
{
	namespace
	{
		// Feeds StatusClock: the game is paused while any open menu pauses it.
		class StatusPauseMenuSink final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(
				const RE::MenuOpenCloseEvent* a_event,
				RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				auto* ui = RE::UI::GetSingleton();
				if (!a_event || !ui) {
					return RE::BSEventNotifyControl::kContinue;
				}
				// Paused only when both the open menus (the opening one counts,
				// the closing one does not) and the UI's own pause count say so.
				// Whichever order the event and the count update come in, a
				// mistake then only lets the clock run, as before v2.3.1; it
				// never stops it during play.
				const auto changed = ui->GetMenu(a_event->menuName);
				bool menusPause = a_event->opening && changed && changed->PausesGame();
				for (const auto& menu : ui->menuStack) {
					if (menusPause) {
						break;
					}
					if (menu && menu->PausesGame() && !(!a_event->opening && menu.get() == changed.get())) {
						menusPause = true;
					}
				}
				StatusClock::SetGamePaused(menusPause && ui->GameIsPaused());
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		StatusPauseMenuSink g_statusPauseMenuSink;
	}

	EventBridge* EventBridge::GetSingleton()
	{
		static EventBridge singleton;
		return std::addressof(singleton);
	}

	void EventBridge::Register()
	{
		bool expected = false;
		if (!_eventSinksRegistered.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
			return;
		}

		if (auto* holder = RE::ScriptEventSourceHolder::GetSingleton()) {
			holder->AddEventSink<RE::TESHitEvent>(this);
			holder->AddEventSink<RE::TESDeathEvent>(this);
			holder->AddEventSink<RE::TESEquipEvent>(this);
			holder->AddEventSink<RE::TESActivateEvent>(this);
			holder->AddEventSink<RE::TESMagicEffectApplyEvent>(this);
			holder->AddEventSink<RE::TESContainerChangedEvent>(this);
			holder->AddEventSink<RE::TESUniqueIDChangeEvent>(this);
		}

		if (auto* modCallbackSource = SKSE::GetModCallbackEventSource()) {
			modCallbackSource->AddEventSink(this);
		}

		if (auto* ui = RE::UI::GetSingleton()) {
			ui->AddEventSink<RE::MenuOpenCloseEvent>(&g_statusPauseMenuSink);
		}
		SKSE::log::info("CalamityAffixes: event ingress uses nonblocking state admission and generation-scoped FIFO deferral (combat-lock test).");
	}
}
