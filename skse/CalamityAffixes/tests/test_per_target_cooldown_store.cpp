#include "CalamityAffixes/PerTargetCooldownStore.h"

using CalamityAffixes::detail::IsPerTargetCooldownPruneDue;
using CalamityAffixes::detail::IsPerTargetCooldownPruneSizeExceeded;
using CalamityAffixes::detail::IsValidPerTargetCooldownCommitInput;
using CalamityAffixes::detail::IsValidPerTargetCooldownKey;

static_assert(IsValidPerTargetCooldownKey(0xA5u, 0x14u));
static_assert(!IsValidPerTargetCooldownKey(0u, 0x14u));
static_assert(!IsValidPerTargetCooldownKey(0xA5u, 0u));

static_assert(IsValidPerTargetCooldownCommitInput(0xA5u, 0x14u, std::chrono::milliseconds(250)));
static_assert(!IsValidPerTargetCooldownCommitInput(0xA5u, 0x14u, std::chrono::milliseconds(0)));
static_assert(!IsValidPerTargetCooldownCommitInput(0u, 0x14u, std::chrono::milliseconds(250)));

static_assert(
	IsPerTargetCooldownPruneDue(std::chrono::seconds(11), std::chrono::seconds(10)));
static_assert(
	!IsPerTargetCooldownPruneDue(std::chrono::seconds(10), std::chrono::seconds(10)));

static_assert(IsPerTargetCooldownPruneSizeExceeded(8193u, 8192u));
static_assert(!IsPerTargetCooldownPruneSizeExceeded(8192u, 8192u));
