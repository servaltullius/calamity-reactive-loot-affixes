#include "CalamityAffixes/InstanceKeyTransferPolicy.h"

using CalamityAffixes::InstanceKeyTransferContext;
using CalamityAffixes::InstanceKeyTransferDecision;
using CalamityAffixes::IsDetachedWorldReferenceMatch;
using CalamityAffixes::IsOrphanedPlayerInstanceKey;
using CalamityAffixes::MakeDetachedWorldInstanceKey;
using CalamityAffixes::MakeTrackedInstanceKey;
using CalamityAffixes::ResolveInstanceKeyTransfer;

namespace
{
	constexpr std::uint32_t kPlayerID = 0x14u;
	constexpr std::uint32_t kMerchantID = 0x1000u;
	constexpr std::uint32_t kChestID = 0x2000u;
	constexpr std::uint32_t kItemFormID = 0x3000u;
	constexpr std::uint32_t kDroppedWeaponRefID = 0xFF001234u;
}

static_assert([] {
	constexpr std::uint16_t recycledPlayerUID = 80u;
	constexpr auto formerPlayerKey = MakeTrackedInstanceKey(kPlayerID, recycledPlayerUID);
	constexpr auto detachedWorldKey = MakeDetachedWorldInstanceKey(kDroppedWeaponRefID);
	constexpr auto unrelatedIncomingKey = MakeTrackedInstanceKey(kPlayerID, recycledPlayerUID);

	return detachedWorldKey != 0u &&
	       detachedWorldKey != formerPlayerKey &&
	       detachedWorldKey != unrelatedIncomingKey &&
	       detachedWorldKey == MakeDetachedWorldInstanceKey(kDroppedWeaponRefID);
}(),
	"Weapon-rack transfer: dropped state must leave the reusable player-UID namespace and remain bound to the world reference");

static_assert(MakeDetachedWorldInstanceKey(0u) == 0u,
	"Weapon-rack transfer: an invalid world reference must not produce a state key");

static_assert([] {
	constexpr auto firstWorldKey = MakeDetachedWorldInstanceKey(kDroppedWeaponRefID);
	constexpr auto secondWorldKey = MakeDetachedWorldInstanceKey(kDroppedWeaponRefID + 1u);

	return IsDetachedWorldReferenceMatch(firstWorldKey, firstWorldKey) &&
	       IsDetachedWorldReferenceMatch(0u, firstWorldKey) &&
	       !IsDetachedWorldReferenceMatch(firstWorldKey, secondWorldKey) &&
	       !IsDetachedWorldReferenceMatch(firstWorldKey, 0u);
}(),
	"Weapon-rack transfer: a recycled reference handle must never authorize state transfer to a different world reference");

static_assert([] {
	constexpr auto playerKey = MakeTrackedInstanceKey(kPlayerID, 90u);
	constexpr auto chestKey = MakeTrackedInstanceKey(kChestID, 90u);

	return IsOrphanedPlayerInstanceKey(playerKey, kPlayerID, false) &&
	       !IsOrphanedPlayerInstanceKey(playerKey, kPlayerID, true) &&
	       !IsOrphanedPlayerInstanceKey(chestKey, kPlayerID, false) &&
	       !IsOrphanedPlayerInstanceKey(0u, kPlayerID, false);
}(),
	"Weapon-rack migration: only player-owned state without a matching inventory instance is orphaned");

static_assert([] {
	const auto plan = ResolveInstanceKeyTransfer(InstanceKeyTransferContext{
		.playerID = kPlayerID,
		.oldOwnerID = kPlayerID,
		.newOwnerID = kMerchantID,
		.itemFormID = kItemFormID,
		.oldUniqueID = 10u,
		.newUniqueID = 11u,
		.playerHasOldKey = false,
		.playerHasNewKey = false,
		.oldKeyTracked = true,
		.newKeyTracked = false,
	});
	return plan.decision == InstanceKeyTransferDecision::kRemap &&
	       plan.oldKey == MakeTrackedInstanceKey(kPlayerID, 10u) &&
	       plan.newKey == MakeTrackedInstanceKey(kMerchantID, 11u);
}(),
	"Instance key transfer: selling must remap when old owner is the player even if inventory snapshots are stale");

static_assert([] {
	const auto plan = ResolveInstanceKeyTransfer(InstanceKeyTransferContext{
		.playerID = kPlayerID,
		.oldOwnerID = kChestID,
		.newOwnerID = kPlayerID,
		.itemFormID = kItemFormID,
		.oldUniqueID = 20u,
		.newUniqueID = 21u,
		.playerHasOldKey = false,
		.playerHasNewKey = false,
		.oldKeyTracked = true,
		.newKeyTracked = false,
	});
	return plan.decision == InstanceKeyTransferDecision::kRemap;
}(),
	"Instance key transfer: container retrieval must remap when new owner is the player even if inventory snapshots are stale");

static_assert([] {
	const auto plan = ResolveInstanceKeyTransfer(InstanceKeyTransferContext{
		.playerID = kPlayerID,
		.oldOwnerID = kMerchantID,
		.newOwnerID = kChestID,
		.itemFormID = kItemFormID,
		.oldUniqueID = 30u,
		.newUniqueID = 31u,
		.oldKeyTracked = true,
		.newKeyTracked = false,
	});
	return plan.decision == InstanceKeyTransferDecision::kIgnore;
}(),
	"Instance key transfer: unrelated owner changes must not move player-tracked state");

static_assert([] {
	const auto plan = ResolveInstanceKeyTransfer(InstanceKeyTransferContext{
		.playerID = kPlayerID,
		.oldOwnerID = kMerchantID,
		.newOwnerID = kChestID,
		.itemFormID = kItemFormID,
		.oldUniqueID = 32u,
		.newUniqueID = 33u,
		.playerHasOldKey = true,
		.oldKeyTracked = true,
		.newKeyTracked = false,
	});
	return plan.decision == InstanceKeyTransferDecision::kRemap;
}(),
	"Instance key transfer: player inventory snapshot remains a legacy compatibility remap path");

static_assert([] {
	const auto plan = ResolveInstanceKeyTransfer(InstanceKeyTransferContext{
		.playerID = kPlayerID,
		.oldOwnerID = kMerchantID,
		.newOwnerID = kChestID,
		.itemFormID = kPlayerID,
		.oldUniqueID = 40u,
		.newUniqueID = 41u,
		.oldKeyTracked = true,
		.newKeyTracked = false,
	});
	return plan.decision == InstanceKeyTransferDecision::kIgnore;
}(),
	"Instance key transfer: item form ID must never be treated as current-owner evidence");

static_assert([] {
	const auto plan = ResolveInstanceKeyTransfer(InstanceKeyTransferContext{
		.playerID = kPlayerID,
		.oldOwnerID = kPlayerID,
		.newOwnerID = kChestID,
		.itemFormID = kItemFormID,
		.oldUniqueID = 45u,
		.newUniqueID = 46u,
		.oldKeyTracked = false,
		.legacyItemKeyTracked = true,
		.newKeyTracked = false,
	});
	return plan.decision == InstanceKeyTransferDecision::kRemap &&
	       plan.oldKey == MakeTrackedInstanceKey(kItemFormID, 45u) &&
	       plan.newKey == MakeTrackedInstanceKey(kChestID, 46u);
}(),
	"Instance key transfer: legacy item-form keys must migrate to the event's new owner key");

static_assert([] {
	const auto plan = ResolveInstanceKeyTransfer(InstanceKeyTransferContext{
		.playerID = kPlayerID,
		.oldOwnerID = kPlayerID,
		.newOwnerID = kMerchantID,
		.itemFormID = kItemFormID,
		.oldUniqueID = 50u,
		.newUniqueID = 51u,
		.oldKeyTracked = true,
		.newKeyTracked = true,
	});
	return plan.decision == InstanceKeyTransferDecision::kConflict;
}(),
	"Instance key transfer: occupied destination is a conflict and must never merge source tokens into destination tokens");

static_assert([] {
	const auto first = ResolveInstanceKeyTransfer(InstanceKeyTransferContext{
		.playerID = kPlayerID,
		.oldOwnerID = kPlayerID,
		.newOwnerID = kMerchantID,
		.itemFormID = kItemFormID,
		.oldUniqueID = 60u,
		.newUniqueID = 61u,
		.oldKeyTracked = true,
		.newKeyTracked = false,
	});
	const auto duplicate = ResolveInstanceKeyTransfer(InstanceKeyTransferContext{
		.playerID = kPlayerID,
		.oldOwnerID = kPlayerID,
		.newOwnerID = kMerchantID,
		.itemFormID = kItemFormID,
		.oldUniqueID = 60u,
		.newUniqueID = 61u,
		.oldKeyTracked = false,
		.newKeyTracked = true,
	});
	return first.decision == InstanceKeyTransferDecision::kRemap &&
	       duplicate.decision == InstanceKeyTransferDecision::kIgnore;
}(),
	"Instance key transfer: a duplicate event after successful remap must be idempotent");

static_assert([] {
	const auto plan = ResolveInstanceKeyTransfer(InstanceKeyTransferContext{
		.playerID = kPlayerID,
		.oldOwnerID = kPlayerID,
		.newOwnerID = kMerchantID,
		.itemFormID = kItemFormID,
		.oldUniqueID = 70u,
		.newUniqueID = 71u,
		.oldKeyTracked = false,
		.legacyItemKeyTracked = true,
		.newKeyTracked = true,
	});
	return plan.decision == InstanceKeyTransferDecision::kConflict;
}(),
	"Instance key transfer: legacy recovery must not overwrite an occupied canonical destination");
