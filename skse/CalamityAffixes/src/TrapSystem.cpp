#include "CalamityAffixes/TrapSystem.h"

#include <atomic>
#include <chrono>
#include <thread>

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes::TrapSystem
{
	namespace
	{
		constexpr auto kPollInterval = std::chrono::milliseconds(250);

		std::atomic_bool g_installed{ false };
		std::jthread g_worker;

		void Tick()
		{
			auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
			if (!bridge) {
				return;
			}

			bridge->TickTraps();
		}
	}

	void Install()
	{
		bool expected = false;
		if (!g_installed.compare_exchange_strong(expected, true)) {
			return;
		}

		g_worker = std::jthread([](std::stop_token stopToken) {
			while (!stopToken.stop_requested()) {
				std::this_thread::sleep_for(kPollInterval);

				auto* bridge = CalamityAffixes::EventBridge::GetSingleton();
				if (!bridge || !bridge->HasActiveTraps()) {
					continue;
				}

				auto* tasks = SKSE::GetTaskInterface();
				if (!tasks) {
					continue;
				}

				tasks->AddTask([]() {
					Tick();
				});
			}
		});

		SKSE::log::info("CalamityAffixes: TrapSystem enabled (poll={}ms).", kPollInterval.count());
	}
}
