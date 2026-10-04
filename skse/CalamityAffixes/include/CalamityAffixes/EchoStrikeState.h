#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace CalamityAffixes
{
	namespace detail
	{
		// Echo strikes follow the owner's own weapon swings. Spells, bashes,
		// explosions, and ranged hits never echo, and a hit whose aggressor is
		// not the owner (a summon routed to the player) is not the owner's swing.
		[[nodiscard]] constexpr bool IsEchoStrikeEligibleHit(
			bool a_aggressorIsOwner,
			bool a_hasMeleeEvidence,
			bool a_isRangedWeapon,
			bool a_hasAttackSpell,
			bool a_isBash,
			bool a_isExplosion) noexcept
		{
			return a_aggressorIsOwner &&
			       a_hasMeleeEvidence &&
			       !a_isRangedWeapon &&
			       !a_hasAttackSpell &&
			       !a_isBash &&
			       !a_isExplosion;
		}

		// Any eligible swing opens the echo window unless the action asks for a
		// power attack; while it is open every eligible swing echoes.
		[[nodiscard]] constexpr bool IsEchoStrikeActivationHit(
			bool a_eligibleHit,
			bool a_isPowerAttack,
			bool a_requirePowerAttack) noexcept
		{
			return a_eligibleHit && (a_isPowerAttack || !a_requirePowerAttack);
		}
	}

	struct EchoStrikePending
	{
		std::uint64_t affixToken{ 0u };
		std::uint32_t targetFormID{ 0u };
		float magnitude{ 0.0f };
		std::chrono::steady_clock::time_point fireAt{};
	};

	// Transient Shadow Boxer state: the open window and the echoes waiting to land.
	// Never serialized; save/load, revert, and config reload clear it.
	struct EchoStrikeRuntimeState
	{
		// Bounds the queue when many swings land in one delay interval.
		static constexpr std::size_t kMaxPending = 24u;
		// One echo per source hit. The echo's own damage re-reads the target's
		// last HitData, so it can look like the hit that spawned it; remembering
		// recent sources stops an echo from ever echoing itself.
		static constexpr std::size_t kRecentSourceCount = 32u;

		std::uint64_t windowToken{ 0u };
		std::chrono::steady_clock::time_point windowUntil{};
		std::vector<EchoStrikePending> pending{};
		std::array<std::uint64_t, kRecentSourceCount> recentSources{};
		std::size_t recentCursor{ 0u };
		std::atomic_bool hasPending{ false };

		[[nodiscard]] bool IsWindowOpen(std::chrono::steady_clock::time_point a_now) const noexcept
		{
			return windowToken != 0u && a_now < windowUntil;
		}

		void OpenWindow(std::uint64_t a_token, std::chrono::steady_clock::time_point a_until) noexcept
		{
			windowToken = a_token;
			windowUntil = a_until;
		}

		void CloseWindow() noexcept
		{
			windowToken = 0u;
			windowUntil = {};
		}

		// Returns false when this source hit already produced an echo.
		[[nodiscard]] bool TryRememberSource(std::uint64_t a_signature) noexcept
		{
			if (a_signature == 0u) {
				return false;
			}
			for (const auto seen : recentSources) {
				if (seen == a_signature) {
					return false;
				}
			}
			recentSources[recentCursor] = a_signature;
			recentCursor = (recentCursor + 1u) % recentSources.size();
			return true;
		}

		[[nodiscard]] bool Enqueue(const EchoStrikePending& a_echo)
		{
			if (pending.size() >= kMaxPending) {
				return false;
			}
			pending.push_back(a_echo);
			hasPending.store(true, std::memory_order_release);
			return true;
		}

		// Moves every echo due at a_now into a_out, keeping the rest in order.
		std::size_t TakeDue(std::chrono::steady_clock::time_point a_now, std::vector<EchoStrikePending>& a_out)
		{
			a_out.clear();
			std::size_t kept = 0u;
			for (auto& echo : pending) {
				if (echo.fireAt <= a_now) {
					a_out.push_back(echo);
				} else {
					pending[kept++] = echo;
				}
			}
			pending.resize(kept);
			hasPending.store(!pending.empty(), std::memory_order_release);
			return a_out.size();
		}

		void Reset() noexcept
		{
			CloseWindow();
			pending.clear();
			recentSources.fill(0u);
			recentCursor = 0u;
			hasPending.store(false, std::memory_order_release);
		}
	};
}
