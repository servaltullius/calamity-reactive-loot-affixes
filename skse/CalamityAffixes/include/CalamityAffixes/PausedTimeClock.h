#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>

namespace CalamityAffixes
{
	// Milliseconds that stop while the game is paused. Magic effect durations do
	// not run in a menu that pauses the game, so status records (Exposed, meters,
	// Doom) must not either: on wall time a record could end while its effect was
	// still on the target (v2.3.1).
	class PausedTimeClock
	{
	public:
		constexpr void SetPaused(bool a_paused, std::int64_t a_realMs) noexcept
		{
			if (a_paused == _paused) {
				return;
			}
			if (a_paused) {
				_pauseStartMs = a_realMs;
			} else if (a_realMs > _pauseStartMs) {
				_pausedTotalMs += a_realMs - _pauseStartMs;
			}
			_paused = a_paused;
		}

		[[nodiscard]] constexpr std::int64_t Now(std::int64_t a_realMs) const noexcept
		{
			const std::int64_t frozenAt = _paused && _pauseStartMs < a_realMs ? _pauseStartMs : a_realMs;
			return frozenAt - _pausedTotalMs;
		}

		[[nodiscard]] constexpr bool IsPaused() const noexcept { return _paused; }

	private:
		bool _paused{ false };
		std::int64_t _pauseStartMs{ 0 };
		std::int64_t _pausedTotalMs{ 0 };
	};

	namespace StatusClock
	{
		[[nodiscard]] inline std::int64_t RealNowMs() noexcept
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count();
		}

		struct State
		{
			std::mutex lock;
			PausedTimeClock clock;
		};

		[[nodiscard]] inline State& GetState() noexcept
		{
			static State state;
			return state;
		}

		// Fed from the menu open/close sink (EventBridge.cpp).
		inline void SetGamePaused(bool a_paused) noexcept
		{
			auto& state = GetState();
			const std::scoped_lock guard(state.lock);
			state.clock.SetPaused(a_paused, RealNowMs());
		}

		[[nodiscard]] inline std::int64_t NowMs() noexcept
		{
			auto& state = GetState();
			const std::scoped_lock guard(state.lock);
			return state.clock.Now(RealNowMs());
		}
	}
}
