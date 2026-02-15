#pragma once

#include <chrono>
#include <cstdint>
#include <unordered_map>

namespace CalamityAffixes
{
	class PerTargetCooldownStore
	{
	public:
		[[nodiscard]] bool IsBlocked(
			std::uint64_t a_token,
			std::uint32_t a_targetFormID,
			std::chrono::steady_clock::time_point a_now) const
		{
			if (a_token == 0u || a_targetFormID == 0u) {
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
			if (a_token == 0u || a_targetFormID == 0u || a_perTargetIcd.count() <= 0) {
				return;
			}

			const Key key{
				.token = a_token,
				.target = a_targetFormID
			};
			_nextAllowed[key] = a_now + a_perTargetIcd;

			if (_nextAllowed.size() > kMaxEntries) {
				if (_lastPruneAt.time_since_epoch().count() == 0 ||
					(a_now - _lastPruneAt) > kPruneInterval) {
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
