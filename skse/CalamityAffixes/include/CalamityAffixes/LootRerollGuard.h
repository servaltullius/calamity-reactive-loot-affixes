#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace CalamityAffixes
{
	// Tracks player drop -> pickup cycles so "throw on ground and pick up again" can't reroll affixes.
	// Implemented as a small, allocation-free helper to keep the event path cheap.
	class LootRerollGuard
	{
	public:
		using FormID = std::uint32_t;
		using RefHandle = std::uint32_t;
		using InstanceKey = std::uint64_t;

		static constexpr FormID kWorldContainer = 0;

		constexpr void Reset() noexcept
		{
			_refs.fill({});
			_cursor = 0;
		}

		constexpr void NotePlayerDrop(
			FormID a_player,
			FormID a_oldContainer,
			FormID a_newContainer,
			std::int32_t a_itemCount,
			RefHandle a_reference,
			InstanceKey a_instanceKey) noexcept
		{
			// Only record transfers OUT of the player, into the "world" (i.e. dropped).
			if (a_reference == 0) {
				return;
			}
			if (a_itemCount == 0) {
				return;
			}
			if (a_oldContainer != a_player) {
				return;
			}
			if (a_newContainer != kWorldContainer) {
				return;
			}

			_refs[_cursor] = { .ref = a_reference, .instanceKey = a_instanceKey };
			_cursor = (_cursor + 1) % _refs.size();
		}

		[[nodiscard]] constexpr bool ConsumeIfPlayerDropPickup(
			FormID a_player,
			FormID,
			FormID a_newContainer,
			std::int32_t a_itemCount,
			RefHandle a_reference) noexcept
		{
			if (a_reference == 0) {
				return false;
			}
			if (a_itemCount == 0) {
				return false;
			}
			if (a_newContainer != a_player) {
				return false;
			}

			for (auto& entry : _refs) {
				if (entry.ref == a_reference) {
					entry = {};
					return true;
				}
			}
			return false;
		}

		// Called when a previously dropped world reference is deleted (e.g. cell cleanup).
		// Returns the associated instance key (if recorded) and consumes the entry.
		[[nodiscard]] constexpr std::optional<InstanceKey> ConsumeIfPlayerDropDeleted(RefHandle a_reference) noexcept
		{
			if (a_reference == 0) {
				return std::nullopt;
			}

			for (auto& entry : _refs) {
				if (entry.ref == a_reference) {
					const auto key = entry.instanceKey;
					entry = {};
					if (key == 0) {
						return std::nullopt;
					}
					return key;
				}
			}

			return std::nullopt;
		}

	private:
		static constexpr std::size_t kMaxTrackedRefs = 512;
		struct Entry
		{
			RefHandle ref{ 0 };
			InstanceKey instanceKey{ 0 };
		};

		std::array<Entry, kMaxTrackedRefs> _refs{};
		std::size_t _cursor{ 0 };
	};
}
