#pragma once

#include <chrono>
#include <cstdint>
#include <unordered_map>

namespace CalamityAffixes
{
	class PerTargetCooldownStore
	{
	public:
		[[nodiscard]] static constexpr bool IsValidKey(std::uint64_t a_token, std::uint32_t a_targetFormID) noexcept
		{
			return a_token != 0u && a_targetFormID != 0u;
		}

		[[nodiscard]] static constexpr bool IsValidCommitInput(
			std::uint64_t a_token,
			std::uint32_t a_targetFormID,
			std::chrono::milliseconds a_perTargetIcd) noexcept
		{
			return IsValidKey(a_token, a_targetFormID) && a_perTargetIcd.count() > 0;
		}

		[[nodiscard]] static constexpr bool IsPruneDue(
			std::chrono::steady_clock::duration a_elapsed,
			std::chrono::steady_clock::duration a_pruneInterval) noexcept
		{
			return a_elapsed > a_pruneInterval;
		}

		[[nodiscard]] static constexpr bool IsPruneSizeExceeded(std::size_t a_size, std::size_t a_maxEntries) noexcept
		{
			return a_size > a_maxEntries;
		}

		[[nodiscard]] bool IsBlocked(
			std::uint64_t a_token,
			std::uint32_t a_targetFormID,
			std::chrono::steady_clock::time_point a_now) const
		{
			if (!IsValidKey(a_token, a_targetFormID)) {
				return false;
			}

			const Key key{
				.token = a_token,
				.target = a_targetFormID
			};
			if (const auto it = _nextAllowed.find(key); it != _nextAllowed.end()) {
				return a_now < it->second;
			}
			return false;
		}

		void Commit(
			std::uint64_t a_token,
			std::uint32_t a_targetFormID,
			std::chrono::milliseconds a_perTargetIcd,
			std::chrono::steady_clock::time_point a_now)
		{
			if (!IsValidCommitInput(a_token, a_targetFormID, a_perTargetIcd)) {
				return;
			}

			const Key key{
				.token = a_token,
				.target = a_targetFormID
			};
			_nextAllowed[key] = a_now + a_perTargetIcd;

			if (IsPruneSizeExceeded(_nextAllowed.size(), kMaxEntries)) {
				if (_lastPruneAt.time_since_epoch().count() == 0 ||
					IsPruneDue(a_now - _lastPruneAt, kPruneInterval)) {
					_lastPruneAt = a_now;
					for (auto it = _nextAllowed.begin(); it != _nextAllowed.end();) {
						if (a_now >= it->second) {
							it = _nextAllowed.erase(it);
						} else {
							++it;
						}
					}
				}
			}
		}

		void Clear() noexcept
		{
			_nextAllowed.clear();
			_lastPruneAt = {};
		}

	private:
		struct Key
		{
			std::uint64_t token{ 0 };
			std::uint32_t target{ 0 };
		};

		struct KeyHash
		{
			[[nodiscard]] std::size_t operator()(const Key& a_key) const noexcept
			{
				const std::uint64_t mixed = a_key.token ^ (static_cast<std::uint64_t>(a_key.target) << 1);
				return std::hash<std::uint64_t>{}(mixed);
			}
		};

		struct KeyEq
		{
			[[nodiscard]] bool operator()(const Key& a, const Key& b) const noexcept
			{
				return a.token == b.token && a.target == b.target;
			}
		};

		static constexpr std::size_t kMaxEntries = 8192;
		static constexpr auto kPruneInterval = std::chrono::seconds(10);

		std::unordered_map<Key, std::chrono::steady_clock::time_point, KeyHash, KeyEq> _nextAllowed;
		std::chrono::steady_clock::time_point _lastPruneAt{};
	};
}
