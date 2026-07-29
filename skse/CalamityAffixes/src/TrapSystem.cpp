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
		std::atomic_bool g_tickTaskPending{ false };
		std::jthread g_worker;

		class TickTaskPendingReset final
		{
		public:
			~TickTaskPendingReset()
			{
				g_tickTaskPending.store(false, std::memory_order_release);
			}
		};

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
				if (!bridge) {
					continue;
				}
				if (!bridge->HasActiveTraps()) {
					continue;
				}

				auto* tasks = SKSE::GetTaskInterface();
				if (!tasks) {
					continue;
				}

				bool expected = false;
				if (!g_tickTaskPending.compare_exchange_strong(
						expected,
						true,
						std::memory_order_acq_rel)) {
					continue;
				}
				tasks->AddTask([]() {
					[[maybe_unused]] const TickTaskPendingReset pendingReset{};
					Tick();
				});
			}
		});

		SKSE::log::info("CalamityAffixes: TrapSystem enabled (poll={}ms).", kPollInterval.count());
	}
}
