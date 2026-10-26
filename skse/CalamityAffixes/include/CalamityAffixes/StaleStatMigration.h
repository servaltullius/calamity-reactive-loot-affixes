#pragma once

#include <array>
#include <cstdint>
#include <string_view>

// One-time clean-up of actor value modifiers that old saves keep from effects
// whose actor value, archetype or magnitude changed under the same FormID
// (v2.3.0). The engine does not call Start again for an effect it loads from a
// save: the amount the old effect added stays in the actor's saved modifiers,
// while the loaded effect now points at the new actor value. Nothing ever takes
// the old amount off, and removing the effect would take it off the new actor
// value instead.
//
// This header holds the pure part (the table and the arithmetic) so the host
// tests can check it. EventBridge.Serialization.StatMigration.cpp applies it.
namespace CalamityAffixes::StaleStatMigration
{
	// Bit in the MFLG co-save byte (bits 1 and 2 are the misc currency flags).
	// An older DLL reads the byte and drops this bit when it saves again, so the
	// clean-up runs again after a downgrade and a second upgrade. Downgrading is
	// not supported for this clean-up: the old plugin loads the new effects
	// against the old actor values, and the effects this clean-up then finds
	// can be new ones that only look old (for the entries whose magnitude did
	// not change, nothing tells them apart). The checks below keep it from
	// taking off more than an unexplained amount, but the result after such a
	// round trip is not guaranteed.
	inline constexpr std::uint8_t kMigrationFlagBit = 0x04u;

	// Raw RE::ActorValue indices (RE/A/ActorValues.h); the host tests build
	// without CommonLib. The plugin static_asserts them against RE::ActorValue.
	namespace Av
	{
		inline constexpr std::uint32_t kMagickaRate = 28u;
		inline constexpr std::uint32_t kBowSpeedBonus = 78u;
		inline constexpr std::uint32_t kWeaponSpeedMult = 85u;
		inline constexpr std::uint32_t kShoutRecoveryMult = 86u;
		inline constexpr std::uint32_t kOneHandedModifier = 96u;
		inline constexpr std::uint32_t kTwoHandedModifier = 97u;
		inline constexpr std::uint32_t kBlockModifier = 99u;
		inline constexpr std::uint32_t kHeavyArmorModifier = 101u;
		inline constexpr std::uint32_t kAlterationModifier = 108u;
		inline constexpr std::uint32_t kConjurationModifier = 109u;
		inline constexpr std::uint32_t kDestructionModifier = 110u;
		inline constexpr std::uint32_t kIllusionModifier = 111u;
		inline constexpr std::uint32_t kRestorationModifier = 112u;
		inline constexpr std::uint32_t kEnchantingModifier = 113u;
		inline constexpr std::uint32_t kAttackDamageMult = 154u;
	}

	enum class Handling : std::uint8_t
	{
		// Same actor value and archetype, only the magnitude changed: removing the
		// effect takes the old amount off correctly; the runtime adds it again.
		kRefresh,
		// The actor value or archetype changed: zero the loaded effect so its
		// Finish takes nothing off the new actor value and remove it (always),
		// and take the old amount off the old actor value (only when the old
		// actor value still holds it).
		kMovedValue,
		// The effect is now a no-op carrier the runtime still reads: keep it and
		// only take the old amount off the old actor value.
		kCarrierOnly,
	};

	struct Entry
	{
		std::string_view magicEffectEditorId;
		std::uint32_t oldActorValue;
		// What the shipped version put on the effect; a loaded effect matches only
		// with this magnitude, so one the new data already re-applied is left alone.
		float oldMagnitude;
		Handling handling;
		// A timed effect whose archetype changed: the engine may drop it at load
		// and leave only the modifier. Also sweep the old actor value when what is
		// left there is larger than anything vanilla puts on it.
		bool orphanSweep;
		// The current data uses the same magnitude, so a match proves nothing
		// about the effect being an old one: take the old amount off only when
		// the old actor value holds it beyond what live effects explain.
		bool magnitudeUnchanged{ false };
	};

	// Shipped values per version (git tags v2.2.8, v2.3.0-rc1..rc3). Magnitudes
	// that two versions shared appear once. The last field marks entries whose
	// current magnitude is still the old one (Eagle Eye, Enchanting, Zephyr,
	// Cure, Enlightenment).
	inline constexpr std::array kEntries{
		// Passive abilities (any save made while the item was worn).
		Entry{ "CAFF_MGEF_SUFFIX_ATTACK_SPEED_T1", Av::kWeaponSpeedMult, 0.05f, Handling::kMovedValue, false },  // Gladiator, <= rc2
		Entry{ "CAFF_MGEF_SUFFIX_ATTACK_SPEED_T2", Av::kWeaponSpeedMult, 0.10f, Handling::kMovedValue, false },
		Entry{ "CAFF_MGEF_SUFFIX_ATTACK_SPEED_T3", Av::kWeaponSpeedMult, 0.15f, Handling::kMovedValue, false },
		Entry{ "CAFF_MGEF_RW_GRIEF_PASSIVE_IAS", Av::kWeaponSpeedMult, 0.10f, Handling::kMovedValue, false },  // <= rc2
		Entry{ "CAFF_MGEF_RW_FAITH_PASSIVE_IAS", Av::kWeaponSpeedMult, 0.15f, Handling::kMovedValue, false },
		Entry{ "CAFF_MGEF_RW_HUSTLEW_PASSIVE_IAS", Av::kWeaponSpeedMult, 0.12f, Handling::kMovedValue, false },
		Entry{ "CAFF_MGEF_RW_TREACHERY_PASSIVE_IAS", Av::kWeaponSpeedMult, 0.10f, Handling::kMovedValue, false },
		Entry{ "CAFF_MGEF_SUFFIX_SHOUT_COOLDOWN_T1", Av::kBowSpeedBonus, 5.0f, Handling::kCarrierOnly, false, true },  // Eagle Eye, <= rc3
		Entry{ "CAFF_MGEF_SUFFIX_SHOUT_COOLDOWN_T2", Av::kBowSpeedBonus, 10.0f, Handling::kCarrierOnly, false, true },
		Entry{ "CAFF_MGEF_SUFFIX_SHOUT_COOLDOWN_T3", Av::kBowSpeedBonus, 15.0f, Handling::kCarrierOnly, false, true },
		Entry{ "CAFF_MGEF_SUFFIX_ENCHANTING_T1", Av::kEnchantingModifier, 5.0f, Handling::kMovedValue, false, true },  // <= rc3
		Entry{ "CAFF_MGEF_SUFFIX_ENCHANTING_T2", Av::kEnchantingModifier, 10.0f, Handling::kMovedValue, false, true },
		Entry{ "CAFF_MGEF_SUFFIX_ENCHANTING_T3", Av::kEnchantingModifier, 15.0f, Handling::kMovedValue, false, true },
		Entry{ "CAFF_MGEF_RW_INSIGHT_PASSIVE_MPREGEN", Av::kMagickaRate, 25.0f, Handling::kRefresh, false },  // <= rc3
		// Timed buffs (only a save made while one was running).
		Entry{ "CAFF_MGEF_RW_GROUND_ANCHOR", Av::kShoutRecoveryMult, 50.0f, Handling::kMovedValue, true },  // <= rc3
		Entry{ "CAFF_MGEF_RW_ZEPHYR_RUSH", Av::kBowSpeedBonus, 30.0f, Handling::kCarrierOnly, true, true },  // <= rc3
		Entry{ "CAFF_MGEF_RW_FURY_LIFESTEAL", Av::kWeaponSpeedMult, 0.25f, Handling::kMovedValue, false },  // <= rc2
		Entry{ "CAFF_MGEF_RW_FURY_LIFESTEAL", Av::kAttackDamageMult, 0.15f, Handling::kMovedValue, false },  // rc3
		Entry{ "CAFF_MGEF_RW_FAITH_FANATIC_SPEED", Av::kWeaponSpeedMult, 0.15f, Handling::kMovedValue, false },  // Faith, Beast Rage, <= rc2
		Entry{ "CAFF_MGEF_SWAP_JACKPOT_HASTE", Av::kWeaponSpeedMult, 0.15f, Handling::kMovedValue, false },  // <= rc2
		Entry{ "CAFF_MGEF_RW_LEAF_FLAME", Av::kDestructionModifier, 20.0f, Handling::kMovedValue, false },  // <= rc3
		Entry{ "CAFF_MGEF_RW_STRENGTH_POWER", Av::kTwoHandedModifier, 25.0f, Handling::kMovedValue, false },
		Entry{ "CAFF_MGEF_RW_UNBENDING_WILL_RESOLVE", Av::kBlockModifier, 30.0f, Handling::kMovedValue, false },
		Entry{ "CAFF_MGEF_RW_PASSION_FURY", Av::kIllusionModifier, 20.0f, Handling::kMovedValue, false },
		Entry{ "CAFF_MGEF_RW_OATH_SWORN", Av::kOneHandedModifier, 30.0f, Handling::kMovedValue, false },
		Entry{ "CAFF_MGEF_RW_MYTH_SHIELD", Av::kHeavyArmorModifier, 25.0f, Handling::kMovedValue, false },
		Entry{ "CAFF_MGEF_RW_CURE_HEAL", Av::kRestorationModifier, 20.0f, Handling::kMovedValue, false, true },
		Entry{ "CAFF_MGEF_RW_HEARTH_WARMTH", Av::kConjurationModifier, 20.0f, Handling::kMovedValue, false },
		Entry{ "CAFF_MGEF_RW_ENLIGHTENMENT_AURA", Av::kAlterationModifier, 25.0f, Handling::kMovedValue, false, true },
	};

	[[nodiscard]] constexpr float Abs(float a_value) noexcept
	{
		return a_value < 0.0f ? -a_value : a_value;
	}

	[[nodiscard]] constexpr bool MagnitudeMatches(float a_loaded, float a_old) noexcept
	{
		const float tolerance = a_old * 0.001f > 0.0001f ? a_old * 0.001f : 0.0001f;
		return Abs(a_loaded - a_old) <= tolerance;
	}

	[[nodiscard]] constexpr bool EqualsIgnoreCase(std::string_view a_lhs, std::string_view a_rhs) noexcept
	{
		if (a_lhs.size() != a_rhs.size()) {
			return false;
		}
		for (std::size_t i = 0; i < a_lhs.size(); ++i) {
			auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; };
			if (lower(a_lhs[i]) != lower(a_rhs[i])) {
				return false;
			}
		}
		return true;
	}

	// The entry a loaded effect belongs to, or nullptr when it is not an old one.
	[[nodiscard]] constexpr const Entry* FindEntry(std::string_view a_magicEffectEditorId, float a_loadedMagnitude) noexcept
	{
		for (const auto& entry : kEntries) {
			if (EqualsIgnoreCase(entry.magicEffectEditorId, a_magicEffectEditorId) &&
				MagnitudeMatches(a_loadedMagnitude, entry.oldMagnitude)) {
				return &entry;
			}
		}
		return nullptr;
	}

	enum class Slot : std::uint8_t
	{
		kNone,
		kTemporary,
		kPermanent,
	};

	struct SlotValues
	{
		float temporaryModifier{ 0.0f };
		float permanentModifier{ 0.0f };
	};

	struct Removal
	{
		Slot slot{ Slot::kNone };
		float amount{ 0.0f };
		bool includesOrphan{ false };
	};

	[[nodiscard]] constexpr bool Covers(float a_slotValue, float a_amount) noexcept
	{
		const float tolerance = a_amount * 0.001f > 0.0001f ? a_amount * 0.001f : 0.0001f;
		return a_slotValue + tolerance >= a_amount;
	}

	// How much to take off one old actor value. a_expected is the sum of the old
	// magnitudes of the loaded effects found for it, a_orphan what a dropped timed
	// effect may have left. Nothing is taken unless one modifier slot holds at
	// least that much (a magic effect writes all of its amount to one slot) and
	// the two slots together do too (a negative other slot means the value is
	// already lower than the stale amount explains), so the migration never
	// takes off more than was added; when the slots cannot hold the orphan too,
	// only the effects found are cleared.
	[[nodiscard]] constexpr Removal ResolveRemoval(SlotValues a_slots, float a_expected, float a_orphan) noexcept
	{
		const auto pick = [&](float a_amount) -> Slot {
			if (a_amount <= 0.0f) {
				return Slot::kNone;
			}
			if (!Covers(a_slots.temporaryModifier + a_slots.permanentModifier, a_amount)) {
				return Slot::kNone;
			}
			if (Covers(a_slots.temporaryModifier, a_amount)) {
				return Slot::kTemporary;
			}
			if (Covers(a_slots.permanentModifier, a_amount)) {
				return Slot::kPermanent;
			}
			return Slot::kNone;
		};
		if (a_orphan > 0.0f) {
			if (const auto slot = pick(a_expected + a_orphan); slot != Slot::kNone) {
				return Removal{ slot, a_expected + a_orphan, true };
			}
		}
		if (const auto slot = pick(a_expected); slot != Slot::kNone) {
			return Removal{ slot, a_expected, false };
		}
		return Removal{};
	}

	// Whether a found effect's old amount may be taken off its old actor value.
	// a_baseEffectOnOldActorValue: the effect's base effect (as loaded now)
	// still modifies the old actor value, e.g. an old Calamity plugin won a
	// conflict; then nothing is stale and the effect is only refreshed.
	// a_oldValueTotal is the old actor value's temporary + permanent modifier,
	// a_explainedByLiveEffects what the active value-modifier effects that
	// point at it now account for (the effect itself included when it does).
	[[nodiscard]] constexpr bool IsClearlyOldFormat(
		const Entry& a_entry,
		bool a_baseEffectOnOldActorValue,
		float a_oldValueTotal,
		float a_explainedByLiveEffects) noexcept
	{
		if (a_baseEffectOnOldActorValue) {
			return false;
		}
		if (!a_entry.magnitudeUnchanged) {
			return true;
		}
		return Covers(a_oldValueTotal - a_explainedByLiveEffects, a_entry.oldMagnitude);
	}
}
