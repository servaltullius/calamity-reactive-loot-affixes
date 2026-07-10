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
