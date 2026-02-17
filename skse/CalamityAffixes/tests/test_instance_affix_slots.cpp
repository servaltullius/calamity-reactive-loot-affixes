#include "CalamityAffixes/InstanceAffixSlots.h"

// --- AddToken ---

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	return slots.count == 0 && slots.GetPrimary() == 0u;
}(),
	"Default-constructed slots should be empty");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	const bool added = slots.AddToken(0xAAAAu);
	return added && slots.count == 1 && slots.tokens[0] == 0xAAAAu;
}(),
	"AddToken should insert first token at index 0");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	slots.AddToken(0xBBBBu);
	slots.AddToken(0xCCCCu);
	return slots.count == 3 &&
		slots.tokens[0] == 0xAAAAu &&
		slots.tokens[1] == 0xBBBBu &&
		slots.tokens[2] == 0xCCCCu;
}(),
	"AddToken should fill all 3 slots in order");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	slots.AddToken(0xBBBBu);
	slots.AddToken(0xCCCCu);
	const bool added = slots.AddToken(0xDDDDu);
	return !added && slots.count == 3;
}(),
	"AddToken should reject when full (capacity exceeded)");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	const bool added = slots.AddToken(0u);
	return !added && slots.count == 0;
}(),
	"AddToken should reject zero token");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	const bool added = slots.AddToken(0xAAAAu);
	return !added && slots.count == 1;
}(),
	"AddToken should reject duplicate token");

// --- HasToken ---

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	return !slots.HasToken(0xAAAAu);
}(),
	"HasToken should return false on empty slots");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	slots.AddToken(0xBBBBu);
	return slots.HasToken(0xAAAAu) && slots.HasToken(0xBBBBu) && !slots.HasToken(0xCCCCu);
}(),
	"HasToken should find existing tokens and reject missing ones");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	return !slots.HasToken(0u);
}(),
	"HasToken should return false for zero token");

// --- GetPrimary ---

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	slots.AddToken(0xBBBBu);
	return slots.GetPrimary() == 0xAAAAu;
}(),
	"GetPrimary should return first token");

// --- Clear ---

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	slots.AddToken(0xBBBBu);
	slots.Clear();
	return slots.count == 0 &&
		slots.tokens[0] == 0u &&
		slots.tokens[1] == 0u &&
		slots.tokens[2] == 0u &&
		slots.GetPrimary() == 0u;
}(),
	"Clear should zero all tokens and reset count");

// --- ReplaceAll ---

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	slots.AddToken(0xBBBBu);
	slots.AddToken(0xCCCCu);
	slots.ReplaceAll(0xDDDDu);
	return slots.count == 1 &&
		slots.tokens[0] == 0xDDDDu &&
		slots.tokens[1] == 0u &&
		slots.tokens[2] == 0u;
}(),
	"ReplaceAll should clear all slots and set single token");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	slots.ReplaceAll(0u);
	return slots.count == 0 && slots.tokens[0] == 0u;
}(),
	"ReplaceAll with zero token should result in empty slots");

// --- PromoteTokenToPrimary ---

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	slots.AddToken(0xBBBBu);
	slots.AddToken(0xCCCCu);
	const bool promoted = slots.PromoteTokenToPrimary(0xBBBBu);
	return promoted &&
		slots.count == 3 &&
		slots.tokens[0] == 0xBBBBu &&
		slots.tokens[1] == 0xAAAAu &&
		slots.tokens[2] == 0xCCCCu;
}(),
	"PromoteTokenToPrimary should move existing token to index 0 and preserve others");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	slots.AddToken(0xBBBBu);
	const bool promoted = slots.PromoteTokenToPrimary(0xCCCCu);
	return promoted &&
		slots.count == 3 &&
		slots.tokens[0] == 0xCCCCu &&
		slots.tokens[1] == 0xAAAAu &&
		slots.tokens[2] == 0xBBBBu;
}(),
	"PromoteTokenToPrimary should insert missing token at index 0 when capacity is available");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	slots.AddToken(0xBBBBu);
	slots.AddToken(0xCCCCu);
	const bool promoted = slots.PromoteTokenToPrimary(0xDDDDu);
	return !promoted &&
		slots.count == 3 &&
		slots.tokens[0] == 0xAAAAu &&
		slots.tokens[1] == 0xBBBBu &&
		slots.tokens[2] == 0xCCCCu;
}(),
	"PromoteTokenToPrimary should reject missing token when slots are full");

// --- begin/end iteration ---

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	return slots.begin() == slots.end();
}(),
	"Empty slots should have begin == end");

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	slots.AddToken(0xBBBBu);
	auto it = slots.begin();
	const bool first = (*it == 0xAAAAu);
	++it;
	const bool second = (*it == 0xBBBBu);
	++it;
	const bool atEnd = (it == slots.end());
	return first && second && atEnd;
}(),
	"Iteration should cover exactly count elements");

// --- AddToken after ReplaceAll ---

static_assert([] {
	CalamityAffixes::InstanceAffixSlots slots{};
	slots.AddToken(0xAAAAu);
	slots.AddToken(0xBBBBu);
	slots.ReplaceAll(0xCCCCu);
	const bool added = slots.AddToken(0xDDDDu);
	return added && slots.count == 2 &&
		slots.tokens[0] == 0xCCCCu &&
		slots.tokens[1] == 0xDDDDu;
}(),
	"AddToken should work after ReplaceAll (slots freed)");
