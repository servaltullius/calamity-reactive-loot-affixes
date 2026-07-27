#include "CalamityAffixes/SerializationDrainPolicy.h"

using CalamityAffixes::detail::ResolveSerializationDrainChunkSize;
using CalamityAffixes::detail::ShouldWarnUnusuallyLargeSerializationDrain;

static_assert(ResolveSerializationDrainChunkSize(0u) == 0u);
static_assert(ResolveSerializationDrainChunkSize(1u) == 1u);
static_assert(ResolveSerializationDrainChunkSize(4096u) == 4096u);
static_assert(ResolveSerializationDrainChunkSize(4097u) == 4096u);
static_assert(!ShouldWarnUnusuallyLargeSerializationDrain(10'000'000u));
static_assert(ShouldWarnUnusuallyLargeSerializationDrain(10'000'001u));
