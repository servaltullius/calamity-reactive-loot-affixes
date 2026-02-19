#pragma once

#include <chrono>
#include <string>
#include <utility>

namespace CalamityAffixes::RuntimeUserSettingsDebounce
{
	struct State
	{
		bool dirty{ false };
		std::string pendingPayload{};
		std::string lastPersistedPayload{};
		std::chrono::steady_clock::time_point nextFlushAt{};
	};

	[[nodiscard]] inline bool MarkDirty(
		State& a_state,
		std::string a_payload,
		std::chrono::steady_clock::time_point a_now,
		std::chrono::milliseconds a_debounce)
	{
		if (a_payload == a_state.lastPersistedPayload) {
			a_state.dirty = false;
			a_state.pendingPayload.clear();
			a_state.nextFlushAt = {};
			return false;
		}

		a_state.dirty = true;
		a_state.pendingPayload = std::move(a_payload);
		a_state.nextFlushAt = a_now + a_debounce;
		return true;
	}

	[[nodiscard]] inline bool ShouldFlush(
		const State& a_state,
		std::chrono::steady_clock::time_point a_now,
		bool a_force)
	{
		if (!a_state.dirty) {
			return false;
		}
		if (!a_force && a_now < a_state.nextFlushAt) {
			return false;
		}
		return true;
	}

	inline void MarkPersistSuccess(State& a_state)
	{
		a_state.dirty = false;
		a_state.pendingPayload.clear();
		a_state.nextFlushAt = {};
	}

	inline void MarkPersistFailure(
		State& a_state,
		std::chrono::steady_clock::time_point a_now,
		std::chrono::milliseconds a_debounce)
	{
		a_state.nextFlushAt = a_now + a_debounce;
	}
}
