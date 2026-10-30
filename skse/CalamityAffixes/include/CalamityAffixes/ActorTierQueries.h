#pragma once

#include <RE/Skyrim.h>

#include "CalamityAffixes/ActorValueMax.h"

namespace CalamityAffixes
{
	// Maximum health or magicka as the bar shows it (Fortify effects included).
	[[nodiscard]] inline float ResolveActorValueMax(RE::Actor* a_actor, RE::ActorValue a_actorValue)
	{
		auto* avOwner = a_actor ? a_actor->AsActorValueOwner() : nullptr;
		if (!avOwner) {
			return 0.0f;
		}
		return CombineActorValueMax(
			avOwner->GetPermanentActorValue(a_actorValue),
			a_actor->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, a_actorValue));
	}

	// The actor carries the Boss location ref type (dungeon bosses).
	[[nodiscard]] inline bool HasBossLocationRefType(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return false;
		}

		auto* defaultObjects = RE::BGSDefaultObjectManager::GetSingleton();
		constexpr auto bossObjectIndex = static_cast<std::size_t>(
			RE::BGSDefaultObjectManager::DefaultObject::kLocRefTypeBoss);
		// The vendored CommonLib GetObject(DefaultObject) path reads objectInit through
		// RelocateMember<bool*>, which can reinterpret initialized flag bytes as a pointer.
		// The default-object form array itself is stable and null until populated.
		auto* bossObject = defaultObjects ? defaultObjects->objects[bossObjectIndex] : nullptr;
		auto* bossLocationRefType = bossObject ? bossObject->As<RE::BGSLocationRefType>() : nullptr;
		auto* locationRefType = a_actor->extraList.GetByType<RE::ExtraLocationRefType>();
		return bossLocationRefType && locationRefType && locationRefType->locRefType == bossLocationRefType;
	}

	// Bosses, dragons and unique actors: the targets the corpse rewards and the
	// build-up thresholds treat as tough.
	[[nodiscard]] inline bool IsToughActor(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return false;
		}
		const auto* actorBase = a_actor->GetActorBase();
		return HasBossLocationRefType(a_actor) || a_actor->HasKeywordString("ActorTypeDragon") ||
		       (actorBase && actorBase->IsUnique());
	}
}
