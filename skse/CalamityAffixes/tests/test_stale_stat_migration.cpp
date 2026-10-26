#include "CalamityAffixes/StaleStatMigration.h"

namespace
{
	using namespace CalamityAffixes::StaleStatMigration;

	constexpr bool Near(float a_value, float a_expected)
	{
		return Abs(a_value - a_expected) < 0.0001f;
	}

	// Old magnitudes match; the new ones (already re-applied) do not.
	static_assert(FindEntry("CAFF_MGEF_RW_GRIEF_PASSIVE_IAS", 0.10f) != nullptr);
	static_assert(FindEntry("CAFF_MGEF_RW_GRIEF_PASSIVE_IAS", 0.06f) == nullptr);
	static_assert(FindEntry("caff_mgef_rw_grief_passive_ias", 0.10f) != nullptr);
	static_assert(FindEntry("CAFF_MGEF_SUFFIX_ATTACK_SPEED_T3", 0.09f) == nullptr);
	static_assert(FindEntry("CAFF_MGEF_RW_INSIGHT_PASSIVE_MPREGEN", 0.75f) == nullptr);
	static_assert(FindEntry("CAFF_MGEF_RW_GROUND_ANCHOR", 0.5f) == nullptr);
	static_assert(FindEntry("CAFF_MGEF_UNRELATED", 0.10f) == nullptr);

	// Fury changed twice: each shipped value maps to its own old actor value.
	static_assert(FindEntry("CAFF_MGEF_RW_FURY_LIFESTEAL", 0.25f)->oldActorValue == Av::kWeaponSpeedMult);
	static_assert(FindEntry("CAFF_MGEF_RW_FURY_LIFESTEAL", 0.15f)->oldActorValue == Av::kAttackDamageMult);
	static_assert(FindEntry("CAFF_MGEF_RW_FURY_LIFESTEAL", 15.0f) == nullptr);

	static_assert(FindEntry("CAFF_MGEF_SUFFIX_SHOUT_COOLDOWN_T2", 10.0f)->handling == Handling::kCarrierOnly);
	static_assert(FindEntry("CAFF_MGEF_RW_INSIGHT_PASSIVE_MPREGEN", 25.0f)->handling == Handling::kRefresh);

	constexpr bool CheckTableIsWellFormed()
	{
		for (std::size_t i = 0; i < kEntries.size(); ++i) {
			const auto& entry = kEntries[i];
			if (entry.magicEffectEditorId.empty() || !(entry.oldMagnitude > 0.0f)) {
				return false;
			}
			// An orphan sweep only makes sense for an actor value nothing vanilla
			// pushes that high.
			if (entry.orphanSweep && entry.oldMagnitude < 1.0f) {
				return false;
			}
			for (std::size_t j = i + 1; j < kEntries.size(); ++j) {
				if (EqualsIgnoreCase(entry.magicEffectEditorId, kEntries[j].magicEffectEditorId) &&
					MagnitudeMatches(entry.oldMagnitude, kEntries[j].oldMagnitude)) {
					return false;
				}
			}
		}
		return true;
	}
	static_assert(CheckTableIsWellFormed());

	constexpr bool CheckRemovalNeedsOneSlotToHoldIt()
	{
		// The stale +0.10 sits in the temporary slot next to other buffs.
		const auto temp = ResolveRemoval({ .temporaryModifier = 0.35f, .permanentModifier = 0.0f }, 0.10f, 0.0f);
		// Only the permanent slot holds it.
		const auto perm = ResolveRemoval({ .temporaryModifier = 0.02f, .permanentModifier = 0.10f }, 0.10f, 0.0f);
		// Neither does: a slow or another mod already moved it, take nothing.
		const auto none = ResolveRemoval({ .temporaryModifier = 0.05f, .permanentModifier = 0.05f }, 0.10f, 0.0f);
		return temp.slot == Slot::kTemporary && Near(temp.amount, 0.10f) &&
		       perm.slot == Slot::kPermanent && Near(perm.amount, 0.10f) &&
		       none.slot == Slot::kNone && none.amount == 0.0f;
	}
	static_assert(CheckRemovalNeedsOneSlotToHoldIt());

	constexpr bool CheckFloatDriftStillCovers()
	{
		const auto removal = ResolveRemoval({ .temporaryModifier = 0.14999f, .permanentModifier = 0.0f }, 0.15f, 0.0f);
		return removal.slot == Slot::kTemporary && Near(removal.amount, 0.15f);
	}
	static_assert(CheckFloatDriftStillCovers());

	constexpr bool CheckOrphanOnlyWhenThereIsRoom()
	{
		// Ground's +50 with no effect left: swept.
		const auto ground = ResolveRemoval({ .temporaryModifier = 50.0f, .permanentModifier = 0.0f }, 0.0f, 50.0f);
		// Vanilla amulet values: nothing to sweep.
		const auto vanilla = ResolveRemoval({ .temporaryModifier = -0.2f, .permanentModifier = 0.0f }, 0.0f, 50.0f);
		// Eagle Eye 15 found, Zephyr's 30 not there: only the 15 goes.
		const auto partial = ResolveRemoval({ .temporaryModifier = 15.25f, .permanentModifier = 0.0f }, 15.0f, 30.0f);
		// Both there.
		const auto both = ResolveRemoval({ .temporaryModifier = 45.25f, .permanentModifier = 0.0f }, 15.0f, 30.0f);
		return ground.slot == Slot::kTemporary && Near(ground.amount, 50.0f) && ground.includesOrphan &&
		       vanilla.slot == Slot::kNone &&
		       partial.slot == Slot::kTemporary && Near(partial.amount, 15.0f) && !partial.includesOrphan &&
		       both.slot == Slot::kTemporary && Near(both.amount, 45.0f) && both.includesOrphan;
	}
	static_assert(CheckOrphanOnlyWhenThereIsRoom());

	constexpr bool CheckSumOfSlotsMustCoverToo()
	{
		// The temporary slot holds the stale +0.10, but a -0.20 permanent
		// modifier means the value is already below what the stale amount
		// explains: take nothing.
		const auto negativeOther = ResolveRemoval({ .temporaryModifier = 0.10f, .permanentModifier = -0.20f }, 0.10f, 0.0f);
		// A small negative other slot still leaves enough in total.
		const auto enough = ResolveRemoval({ .temporaryModifier = 0.35f, .permanentModifier = -0.05f }, 0.10f, 0.0f);
		// The orphan fits one slot but not the total: only the found effects go.
		const auto orphanNoRoom = ResolveRemoval({ .temporaryModifier = 45.0f, .permanentModifier = -20.0f }, 15.0f, 30.0f);
		return negativeOther.slot == Slot::kNone &&
		       enough.slot == Slot::kTemporary && Near(enough.amount, 0.10f) &&
		       orphanNoRoom.slot == Slot::kTemporary && Near(orphanNoRoom.amount, 15.0f) && !orphanNoRoom.includesOrphan;
	}
	static_assert(CheckSumOfSlotsMustCoverToo());

	// Entries whose current magnitude is still the old one are marked.
	static_assert(FindEntry("CAFF_MGEF_SUFFIX_ENCHANTING_T2", 10.0f)->magnitudeUnchanged);
	static_assert(FindEntry("CAFF_MGEF_SUFFIX_SHOUT_COOLDOWN_T3", 15.0f)->magnitudeUnchanged);
	static_assert(FindEntry("CAFF_MGEF_RW_ZEPHYR_RUSH", 30.0f)->magnitudeUnchanged);
	static_assert(!FindEntry("CAFF_MGEF_RW_GRIEF_PASSIVE_IAS", 0.10f)->magnitudeUnchanged);
	static_assert(!FindEntry("CAFF_MGEF_RW_GROUND_ANCHOR", 50.0f)->magnitudeUnchanged);

	constexpr bool CheckOldFormatEvidence()
	{
		const auto& grief = *FindEntry("CAFF_MGEF_RW_GRIEF_PASSIVE_IAS", 0.10f);
		const auto& enchanting = *FindEntry("CAFF_MGEF_SUFFIX_ENCHANTING_T2", 10.0f);
		const auto& eagleEye = *FindEntry("CAFF_MGEF_SUFFIX_SHOUT_COOLDOWN_T3", 15.0f);
		// A changed magnitude proves an old effect on its own.
		const bool griefOld = IsClearlyOldFormat(grief, false, 0.0f, 0.0f);
		// Its base effect still modifies the old value (an old plugin won): not stale.
		const bool griefOldPlugin = !IsClearlyOldFormat(grief, true, 0.10f, 0.10f);
		// Enchanting +10 with nothing live on EnchantingModifier: the 10 is stale.
		const bool enchantingStale = IsClearlyOldFormat(enchanting, false, 10.0f, 0.0f);
		// A Fortify Enchanting potion explains the 10 that is there: leave it.
		const bool enchantingPotion = !IsClearlyOldFormat(enchanting, false, 10.0f, 10.0f);
		// Potion 20 plus the stale 10.
		const bool enchantingBoth = IsClearlyOldFormat(enchanting, false, 30.0f, 20.0f);
		// A bow-speed mod's +15 is live: the matching Eagle Eye amount is not stale.
		const bool eagleEyeOtherMod = !IsClearlyOldFormat(eagleEye, false, 15.0f, 15.0f);
		return griefOld && griefOldPlugin && enchantingStale && enchantingPotion && enchantingBoth && eagleEyeOtherMod;
	}
	static_assert(CheckOldFormatEvidence());

	static_assert(ResolveRemoval({}, 0.0f, 0.0f).slot == Slot::kNone);
	static_assert((kMigrationFlagBit & 0x03u) == 0u, "bits 1 and 2 are the misc currency flags");
}
