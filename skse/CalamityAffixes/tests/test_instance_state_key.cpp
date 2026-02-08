#include "CalamityAffixes/InstanceStateKey.h"

static_assert([] {
	constexpr auto key = CalamityAffixes::MakeInstanceStateKey(0x123456780001ULL, 0xABCDEFULL);
	return key.instanceKey == 0x123456780001ULL && key.affixToken == 0xABCDEFULL;
}(),
	"MakeInstanceStateKey should preserve instance and affix tokens");

static_assert([] {
	constexpr auto lhs = CalamityAffixes::MakeInstanceStateKey(0x1111ULL, 0xAAAAULL);
	constexpr auto rhs = CalamityAffixes::MakeInstanceStateKey(0x1111ULL, 0xBBBBULL);
	return lhs != rhs;
}(),
	"Different affix tokens on same instance must produce distinct runtime keys");

static_assert([] {
	constexpr auto lhs = CalamityAffixes::MakeInstanceStateKey(0x1111ULL, 0xAAAAULL);
	constexpr auto rhs = CalamityAffixes::MakeInstanceStateKey(0x2222ULL, 0xAAAAULL);
	return lhs != rhs;
}(),
	"Different instance keys must produce distinct runtime keys");

static_assert([] {
	constexpr auto key = CalamityAffixes::MakeInstanceStateKey(0xBEEFULL, 0x42ULL);
	return CalamityAffixes::HashInstanceStateKey(key) == CalamityAffixes::HashInstanceStateKey(0xBEEFULL, 0x42ULL);
}(),
	"Hash helper overloads must be equivalent");

