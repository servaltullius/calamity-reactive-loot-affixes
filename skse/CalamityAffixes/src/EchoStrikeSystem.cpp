#include "CalamityAffixes/EchoStrikeSystem.h"

#include <atomic>
#include <chrono>
#include <thread>

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include "CalamityAffixes/EventBridge.h"

namespace CalamityAffixes::EchoStrikeSystem
{
	namespace
	{
		// Echo delays are a few hundred milliseconds, so the trap system's 250 ms
		// poll would make them visibly uneven. The worker only posts a game task
		// while echoes are queued.
		constexpr auto kPollInterval = std::chrono::milliseconds(15);

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
				if (!bridge || !bridge->HasPendingEchoStrikes()) {
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
					if (auto* bridge = CalamityAffixes::EventBridge::GetSingleton()) {
						bridge->TickEchoStrikes();
					}
				});
			}
		});

		SKSE::log::info("CalamityAffixes: EchoStrikeSystem enabled (poll={}ms).", kPollInterval.count());
	}
}
