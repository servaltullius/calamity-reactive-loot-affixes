#include "CalamityAffixes/HostileEffectDamagePolicy.h"

using CalamityAffixes::detail::HostileEffectDamagePolicyInput;
using CalamityAffixes::detail::IsHostileOnlyEffectTargetAllowed;
using CalamityAffixes::detail::ShouldAcceptRegisteredSummonExplosionDamage;
using CalamityAffixes::detail::ShouldSuppressNonHostileCalamityHealthDamage;

static_assert(!IsHostileOnlyEffectTargetAllowed(true, true, true, false, true, true),
	"the caster is never a hostile-only effect target");
static_assert(!IsHostileOnlyEffectTargetAllowed(true, true, false, false, false, false),
	"allies and neutral actors are not hostile-only effect targets");
static_assert(!IsHostileOnlyEffectTargetAllowed(true, true, false, true, true, true),
	"PlayerRef, teammates, and player-commanded actors remain protected even while hostile");
static_assert(IsHostileOnlyEffectTargetAllowed(true, true, false, false, true, false));
static_assert(IsHostileOnlyEffectTargetAllowed(true, true, false, false, false, true));
static_assert(!IsHostileOnlyEffectTargetAllowed(false, true, false, false, true, true));
static_assert(!IsHostileOnlyEffectTargetAllowed(true, false, false, false, true, true));

static_assert(ShouldAcceptRegisteredSummonExplosionDamage({
	.hitDataIsExplosion = true,
	.hitTargetMatches = true,
	.hitAggressorCompatible = true,
	.sourceExplosionOwnedByRegisteredSummon = true }),
	"the first fully correlated exact-summon explosion callback opens its short damage window");
static_assert(ShouldAcceptRegisteredSummonExplosionDamage({
	.hitDataIsExplosion = true,
	.hitTargetMatches = true,
	.hitAggressorCompatible = true,
	.sourceExplosionOwnedByRegisteredSummon = true,
	.sourceTargetWindowSeen = true,
	.sourceTargetWindowActive = true }),
	"synchronous companion callbacks from the same explosion remain accepted inside the window");
static_assert(!ShouldAcceptRegisteredSummonExplosionDamage({
	.hitDataIsExplosion = true,
	.hitTargetMatches = true,
	.hitAggressorCompatible = true,
	.sourceExplosionOwnedByRegisteredSummon = true,
	.sourceTargetWindowSeen = true }),
	"stale lastHitData cannot reopen an expired source-target window");
static_assert(!ShouldAcceptRegisteredSummonExplosionDamage({
	.hitDataIsExplosion = true,
	.hitTargetMatches = false,
	.hitAggressorCompatible = true,
	.sourceExplosionOwnedByRegisteredSummon = true }),
	"an explosion record for a different target is never accepted");

constexpr HostileEffectDamagePolicyInput kNonHostileDamage{
	.hasTarget = true,
	.hasAttacker = true,
	.hasPlayerOwner = true
};

static_assert(!ShouldSuppressNonHostileCalamityHealthDamage(kNonHostileDamage),
	"ordinary player weapon and manual-spell damage have no Calamity provenance");

static_assert(ShouldSuppressNonHostileCalamityHealthDamage({
	.hasTarget = true,
	.hasAttacker = true,
	.targetIsHostileToPlayerOwner = true,
	.targetIsRegisteredCalamitySummon = true,
	.attackerIsPlayerAlly = true }),
	"a player/allied hit cannot hurt a Calamity summon even after hostility or commander loss");
static_assert(!ShouldSuppressNonHostileCalamityHealthDamage({
	.hasTarget = true,
	.hasAttacker = true,
	.targetIsRegisteredCalamitySummon = true }),
	"enemy attacks still hurt Calamity summons");
static_assert(!ShouldSuppressNonHostileCalamityHealthDamage({
	.hasTarget = true,
	.hasAttacker = true,
	.attackerIsPlayerAlly = true }),
	"player attacks against manual or enemy summons retain their usual behavior");

static_assert(ShouldSuppressNonHostileCalamityHealthDamage({
	.hasTarget = true,
	.hasAttacker = true,
	.hasPlayerOwner = true,
	.hostileOnlyCastScopeActive = true }),
	"a synchronous Calamity hostile-only cast cannot damage a non-hostile actor");
static_assert(ShouldSuppressNonHostileCalamityHealthDamage({
	.hasTarget = true,
	.hasAttacker = true,
	.hasPlayerOwner = true,
	.attackerIsRegisteredCalamitySummon = true }),
	"an exact summoned actor created by a Calamity cast cannot damage a non-hostile actor");
static_assert(ShouldSuppressNonHostileCalamityHealthDamage({
	.hasTarget = true,
	.hasPlayerOwner = true,
	.sourceExplosionOwnedByRegisteredCalamitySummon = true }),
	"an explosion owned by an exact Calamity summon is sufficient provenance even if the engine omits the attacker");

static_assert(!ShouldSuppressNonHostileCalamityHealthDamage({
	.hasTarget = true,
	.hasAttacker = true,
	.hasPlayerOwner = true,
	.attackerIsRegisteredCalamitySummon = false }),
	"an ordinary or merely same-base summon is not a suppression source");

static_assert(ShouldSuppressNonHostileCalamityHealthDamage({
	.hasTarget = true,
	.hasPlayerOwner = true,
	.hostileOnlyCastScopeActive = true }),
	"the scoped player owner makes a synchronous Calamity cast exact even when the engine omits the attacker");

static_assert(!ShouldSuppressNonHostileCalamityHealthDamage({
	.hasTarget = true,
	.hasAttacker = true,
	.hasPlayerOwner = true,
	.targetIsHostileToPlayerOwner = true,
	.hostileOnlyCastScopeActive = true,
	.attackerIsRegisteredCalamitySummon = true,
	.sourceExplosionOwnedByRegisteredCalamitySummon = true }),
	"hostile targets are never protected, even with every Calamity provenance flag");

static_assert(!ShouldSuppressNonHostileCalamityHealthDamage({
	.hasAttacker = true,
	.hasPlayerOwner = true,
	.hostileOnlyCastScopeActive = true }),
	"suppression requires a target");
static_assert(!ShouldSuppressNonHostileCalamityHealthDamage({
	.hasTarget = true,
	.hasAttacker = true,
	.hostileOnlyCastScopeActive = true }),
	"suppression requires a resolved player owner");

static_assert([] {
	for (unsigned mask = 0u; mask < 512u; ++mask) {
		const HostileEffectDamagePolicyInput input{
			.hasTarget = (mask & (1u << 0u)) != 0u,
			.hasAttacker = (mask & (1u << 1u)) != 0u,
			.hasPlayerOwner = (mask & (1u << 2u)) != 0u,
			.targetIsHostileToPlayerOwner = (mask & (1u << 3u)) != 0u,
			.hostileOnlyCastScopeActive = (mask & (1u << 4u)) != 0u,
			.attackerIsRegisteredCalamitySummon = (mask & (1u << 5u)) != 0u,
			.sourceExplosionOwnedByRegisteredCalamitySummon = (mask & (1u << 6u)) != 0u,
			.targetIsRegisteredCalamitySummon = (mask & (1u << 7u)) != 0u,
			.attackerIsPlayerAlly = (mask & (1u << 8u)) != 0u
		};
		const bool hasCalamityProvenance =
			input.hostileOnlyCastScopeActive ||
			(input.hasAttacker && input.attackerIsRegisteredCalamitySummon) ||
			input.sourceExplosionOwnedByRegisteredCalamitySummon;
		const bool incomingAllyHit = input.hasAttacker && input.targetIsRegisteredCalamitySummon && input.attackerIsPlayerAlly;
		const bool expected = input.hasTarget && (incomingAllyHit || (input.hasPlayerOwner &&
		                      !input.targetIsHostileToPlayerOwner && hasCalamityProvenance));
		if (ShouldSuppressNonHostileCalamityHealthDamage(input) != expected) {
			return false;
		}
	}
	return true;
}(), "the policy matches its complete nine-boolean truth table");
