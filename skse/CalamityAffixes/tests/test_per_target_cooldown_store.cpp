#include "CalamityAffixes/PerTargetCooldownStore.h"

using CalamityAffixes::PerTargetCooldownStore;

static_assert(PerTargetCooldownStore::IsValidKey(0xA5u, 0x14u));
static_assert(!PerTargetCooldownStore::IsValidKey(0u, 0x14u));
static_assert(!PerTargetCooldownStore::IsValidKey(0xA5u, 0u));

static_assert(PerTargetCooldownStore::IsValidCommitInput(0xA5u, 0x14u, std::chrono::milliseconds(250)));
static_assert(!PerTargetCooldownStore::IsValidCommitInput(0xA5u, 0x14u, std::chrono::milliseconds(0)));
static_assert(!PerTargetCooldownStore::IsValidCommitInput(0u, 0x14u, std::chrono::milliseconds(250)));

static_assert(
	PerTargetCooldownStore::IsPruneDue(std::chrono::seconds(11), std::chrono::seconds(10)));
static_assert(
	!PerTargetCooldownStore::IsPruneDue(std::chrono::seconds(10), std::chrono::seconds(10)));

static_assert(PerTargetCooldownStore::IsPruneSizeExceeded(8193u, 8192u));
static_assert(!PerTargetCooldownStore::IsPruneSizeExceeded(8192u, 8192u));
