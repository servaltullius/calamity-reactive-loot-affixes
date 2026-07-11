#pragma once

#include <cstdint>

namespace CalamityAffixes
{
	using InstanceKey = std::uint64_t;

	enum class InstanceKeyTransferDecision : std::uint8_t
	{
		kIgnore,
		kRemap,
		kConflict
	};

	struct InstanceKeyTransferContext
	{
		// Normalized TESUniqueIDChangeEvent semantics: ExtraUniqueID is keyed by
		// owner + unique ID. itemFormID identifies the moved item and is retained
		// only for legacy recovery and diagnostics.
		std::uint32_t playerID{ 0 };
		std::uint32_t oldOwnerID{ 0 };
		std::uint32_t newOwnerID{ 0 };
		std::uint32_t itemFormID{ 0 };
		std::uint16_t oldUniqueID{ 0 };
		std::uint16_t newUniqueID{ 0 };
		bool playerHasOldKey{ false };
		bool playerHasNewKey{ false };
		bool oldKeyTracked{ false };
		bool legacyItemKeyTracked{ false };
		bool newKeyTracked{ false };
	};

	struct InstanceKeyTransferPlan
	{
		InstanceKeyTransferDecision decision{ InstanceKeyTransferDecision::kIgnore };
		InstanceKey oldKey{ 0 };
		InstanceKey newKey{ 0 };
	};

	[[nodiscard]] constexpr InstanceKey MakeTrackedInstanceKey(
		std::uint32_t a_ownerFormID,
		std::uint16_t a_uniqueID) noexcept
	{
		return (static_cast<InstanceKey>(a_ownerFormID) << 16u) |
		       static_cast<InstanceKey>(a_uniqueID);
	}

	// A dropped item is no longer owned by the player's UID namespace. Bind its
	// transient state to the concrete world reference instead, using a
	// Calamity-internal sentinel UID so the key remains compatible with existing
	// serialization. Safety comes from matching the exact reference FormID, not
	// from assuming the engine reserves this UID value.
	inline constexpr std::uint16_t kDetachedWorldInstanceUID = 0xFFFFu;

	[[nodiscard]] constexpr InstanceKey MakeDetachedWorldInstanceKey(
		std::uint32_t a_referenceFormID) noexcept
	{
		return a_referenceFormID == 0u ?
			InstanceKey{ 0 } :
			MakeTrackedInstanceKey(a_referenceFormID, kDetachedWorldInstanceUID);
	}

	[[nodiscard]] constexpr bool IsDetachedWorldReferenceMatch(
		InstanceKey a_guardedKey,
		InstanceKey a_referenceKey) noexcept
	{
		// A zero guarded key is a valid "matched drop, no state" marker. Any
		// transferable key requires the concrete reference identity to agree.
		return a_guardedKey == 0u ||
		       (a_referenceKey != 0u && a_guardedKey == a_referenceKey);
	}

	[[nodiscard]] constexpr bool IsOrphanedPlayerInstanceKey(
		InstanceKey a_instanceKey,
		std::uint32_t a_playerID,
		bool a_playerInventoryContainsKey) noexcept
	{
		return a_instanceKey != 0u &&
		       a_playerID != 0u &&
		       static_cast<std::uint32_t>(a_instanceKey >> 16u) == a_playerID &&
		       !a_playerInventoryContainsKey;
	}

	[[nodiscard]] constexpr InstanceKeyTransferPlan ResolveInstanceKeyTransfer(
		const InstanceKeyTransferContext& a_context) noexcept
	{
		const auto oldKey = MakeTrackedInstanceKey(a_context.oldOwnerID, a_context.oldUniqueID);
		const auto newKey = MakeTrackedInstanceKey(a_context.newOwnerID, a_context.newUniqueID);
		const auto legacyItemKey = MakeTrackedInstanceKey(a_context.itemFormID, a_context.oldUniqueID);

		if (a_context.playerID == 0u ||
			a_context.oldOwnerID == 0u ||
			a_context.newOwnerID == 0u ||
			a_context.itemFormID == 0u ||
			a_context.oldUniqueID == 0u ||
			a_context.newUniqueID == 0u) {
			return { InstanceKeyTransferDecision::kIgnore, oldKey, newKey };
		}

		const auto sourceKey = a_context.oldKeyTracked ?
			oldKey :
			(a_context.legacyItemKeyTracked ? legacyItemKey : InstanceKey{ 0 });
		if (sourceKey == 0u || sourceKey == newKey) {
			return { InstanceKeyTransferDecision::kIgnore, sourceKey, newKey };
		}

		const bool ownerTouchesPlayer =
			a_context.oldOwnerID == a_context.playerID ||
			a_context.newOwnerID == a_context.playerID;
		const bool playerInventoryContainsEither =
			a_context.playerHasOldKey || a_context.playerHasNewKey;
		if (!ownerTouchesPlayer && !playerInventoryContainsEither) {
			return { InstanceKeyTransferDecision::kIgnore, sourceKey, newKey };
		}

		if (a_context.newKeyTracked) {
			return { InstanceKeyTransferDecision::kConflict, sourceKey, newKey };
		}

		return { InstanceKeyTransferDecision::kRemap, sourceKey, newKey };
	}
}
