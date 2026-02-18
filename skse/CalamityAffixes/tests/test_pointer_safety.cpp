#include "CalamityAffixes/PointerSafety.h"

using CalamityAffixes::IsLikelyValidObjectAddress;
using CalamityAffixes::ShouldProcessHealthDamageHookPointers;

static_assert(!IsLikelyValidObjectAddress(0u),
	"IsLikelyValidObjectAddress: null address must be rejected");

static_assert(!IsLikelyValidObjectAddress(0x2u),
	"IsLikelyValidObjectAddress: low sentinel-style addresses must be rejected");

static_assert(!IsLikelyValidObjectAddress(0x1003u),
	"IsLikelyValidObjectAddress: misaligned addresses must be rejected");

static_assert(IsLikelyValidObjectAddress(0x10000u),
	"IsLikelyValidObjectAddress: first user-space page boundary should be accepted");

#if UINTPTR_MAX > 0xFFFFFFFFu
static_assert(!IsLikelyValidObjectAddress(0x0000800000000000ull),
	"IsLikelyValidObjectAddress: non-canonical user addresses must be rejected");

static_assert(!IsLikelyValidObjectAddress(0xFFFF800000000000ull),
	"IsLikelyValidObjectAddress: kernel-space canonical addresses must be rejected");

static_assert(IsLikelyValidObjectAddress(0x00007FFF00001000ull),
	"IsLikelyValidObjectAddress: high canonical user addresses should be accepted");
#endif

static_assert(!ShouldProcessHealthDamageHookPointers(0x10000u, 0x2u),
	"ShouldProcessHealthDamageHookPointers: invalid attacker pointer must skip hook path");

static_assert(!ShouldProcessHealthDamageHookPointers(0x2u, 0x10000u),
	"ShouldProcessHealthDamageHookPointers: invalid target pointer must skip hook path");

static_assert(ShouldProcessHealthDamageHookPointers(0x10000u, 0x20000u),
	"ShouldProcessHealthDamageHookPointers: valid actor pointers should continue hook path");
