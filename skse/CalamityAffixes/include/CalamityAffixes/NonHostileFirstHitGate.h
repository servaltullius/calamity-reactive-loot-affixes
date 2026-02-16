#pragma once

#include <chrono>
#include <cstdint>
#include <unordered_map>

namespace CalamityAffixes
{
	namespace detail
	{
		[[nodiscard]] constexpr bool IsValidNonHostileFirstHitKey(std::uint32_t a_ownerFormID, std::uint32_t a_targetFormID) noexcept
		{
			return a_ownerFormID != 0u && a_targetFormID != 0u;
		}

		[[nodiscard]] constexpr bool ShouldTrackNonHostileFirstHit(
			bool a_allowNonHostileOutgoing,
			bool a_hostileEitherDirection,
			bool a_targetIsPlayer) noexcept
		{
			return a_allowNonHostileOutgoing && !a_hostileEitherDirection && !a_targetIsPlayer;
		}

		[[nodiscard]] constexpr bool IsNonHostileFirstHitPruneDue(
			std::chrono::steady_clock::duration a_elapsed,
			std::chrono::steady_clock::duration a_pruneInterval) noexcept
		{
			return a_elapsed > a_pruneInterval;
		}

		[[nodiscard]] constexpr bool IsNonHostileFirstHitReentryAllowed(
			std::chrono::steady_clock::duration a_elapsed,
			std::chrono::steady_clock::duration a_reentryWindow) noexcept
		{
			return a_elapsed <= a_reentryWindow;
		}
	}

	class NonHostileFirstHitGate
	{
	public:
		[[nodiscard]] bool Resolve(
			std::uint32_t a_ownerFormID,
			std::uint32_t a_targetFormID,
			bool a_allowNonHostileOutgoing,
			bool a_hostileEitherDirection,
			bool a_targetIsPlayer,
			std::chrono::steady_clock::time_point a_now)
		{
			const Key key{
				.owner = a_ownerFormID,
				.target = a_targetFormID
			};
			if (!detail::IsValidNonHostileFirstHitKey(key.owner, key.target)) {
				return false;
			}

			if (a_hostileEitherDirection) {
				_seen.erase(key);
				return false;
			}

			if (!detail::ShouldTrackNonHostileFirstHit(a_allowNonHostileOutgoing, a_hostileEitherDirection, a_targetIsPlayer)) {
				return false;
			}

			const bool shouldPrune =
				_lastPruneAt.time_since_epoch().count() == 0 ||
				detail::IsNonHostileFirstHitPruneDue(a_now - _lastPruneAt, kPruneInterval) ||
				_seen.size() > kMaxEntries;
			if (shouldPrune) {
				_lastPruneAt = a_now;
				PruneExpiredAndOversized(a_now);
			}

			const auto [it, inserted] = _seen.emplace(key, a_now);
			if (inserted) {
				return true;
			}

			if (a_now < it->second) {
				it->second = a_now;
				return true;
			}

			return detail::IsNonHostileFirstHitReentryAllowed(a_now - it->second, kReentryWindow);
		}

		void Clear() noexcept
		{
			_seen.clear();
			_lastPruneAt = {};
		}

	private:
		struct Key
		{
			std::uint32_t owner{ 0 };
			std::uint32_t target{ 0 };
		};

		struct KeyHash
		{
			[[nodiscard]] std::size_t operator()(const Key& a_key) const noexcept
			{
				const std::uint64_t packed =
					(static_cast<std::uint64_t>(a_key.owner) << 32) |
					static_cast<std::uint64_t>(a_key.target);
				return std::hash<std::uint64_t>{}(packed);
			}
		};

		struct KeyEq
		{
			[[nodiscard]] bool operator()(const Key& a, const Key& b) const noexcept
			{
				return a.owner == b.owner && a.target == b.target;
			}
		};

		void PruneExpiredAndOversized(std::chrono::steady_clock::time_point a_now)
		{
			const auto expireBefore = a_now - kTtl;
			for (auto it = _seen.begin(); it != _seen.end();) {
				if (it->second <= expireBefore) {
					it = _seen.erase(it);
				} else {
					++it;
				}
			}

			while (_seen.size() > kMaxEntries) {
				auto oldest = _seen.begin();
				for (auto it = _seen.begin(); it != _seen.end(); ++it) {
					if (it->second < oldest->second) {
						oldest = it;
					}
				}
				_seen.erase(oldest);
			}
		}

		static constexpr std::size_t kMaxEntries = 8192;
		static constexpr auto kPruneInterval = std::chrono::seconds(10);
		static constexpr auto kTtl = std::chrono::seconds(120);
		static constexpr auto kReentryWindow = std::chrono::milliseconds(20);

		std::unordered_map<Key, std::chrono::steady_clock::time_point, KeyHash, KeyEq> _seen;
		std::chrono::steady_clock::time_point _lastPruneAt{};
	};
}
