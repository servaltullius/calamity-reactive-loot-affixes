#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace CalamityAffixes
{
	// The shared status vocabulary (v2.3.0). Several affixes apply a status and a
	// few pay off against it, so items combine without their own UI. Burning,
	// Freeze, Bleed and Shock arrive with the build-up meters; Exposed and Doom
	// work on the existing pieces.
	enum class StatusKind : std::uint8_t
	{
		kNone,
		kExposed,
		kDoom,
		kBurning,
		kFreeze,
		kBleed,
		kShock,
	};

	inline constexpr std::size_t kStatusKindCount = 7u;

	[[nodiscard]] constexpr StatusKind ParseStatusKind(std::string_view a_name) noexcept
	{
		if (a_name == "Exposed") return StatusKind::kExposed;
		if (a_name == "Doom") return StatusKind::kDoom;
		if (a_name == "Burning") return StatusKind::kBurning;
		if (a_name == "Freeze") return StatusKind::kFreeze;
		if (a_name == "Bleed") return StatusKind::kBleed;
		if (a_name == "Shock") return StatusKind::kShock;
		return StatusKind::kNone;
	}

	// One status a Calamity spell put on a target. The runtime keeps this ledger
	// itself instead of reading the engine's active effects back: a dying actor's
	// effects are not dependable at the death event, which is exactly when
	// Contagion needs them. Never serialized; every status lasts seconds.
	struct StatusEntry
	{
		std::uint32_t target{ 0u };
		StatusKind kind{ StatusKind::kNone };
		std::uint32_t spellFormID{ 0u };
		std::uint32_t actorValue{ 0u };
		float magnitude{ 0.0f };
		std::int64_t expiresAtMs{ 0 };
	};

	class StatusLedger
	{
	public:
		static constexpr std::size_t kCapacity = 256u;

	private:
		// Defined before its callers: a constant expression cannot call a member
		// template that is declared later in the class.
		template <class Pred>
		constexpr void EraseIf(Pred a_pred) noexcept
		{
			std::size_t kept = 0u;
			for (std::size_t i = 0; i < _count; ++i) {
				if (!a_pred(_entries[i])) {
					_entries[kept++] = _entries[i];
				}
			}
			_count = kept;
		}

	public:

		// Re-applying the same spell to the same target refreshes its entry. A full
		// ledger drops the entry closest to expiring (expired ones first).
		constexpr void Record(const StatusEntry& a_entry) noexcept
		{
			if (a_entry.target == 0u || a_entry.kind == StatusKind::kNone) {
				return;
			}
			for (std::size_t i = 0; i < _count; ++i) {
				if (_entries[i].target == a_entry.target && _entries[i].spellFormID == a_entry.spellFormID &&
					_entries[i].actorValue == a_entry.actorValue) {
					_entries[i] = a_entry;
					return;
				}
			}
			if (_count < kCapacity) {
				_entries[_count++] = a_entry;
				return;
			}
			std::size_t victim = 0u;
			for (std::size_t i = 1; i < _count; ++i) {
				if (_entries[i].expiresAtMs < _entries[victim].expiresAtMs) {
					victim = i;
				}
			}
			_entries[victim] = a_entry;
		}

		// Distinct status kinds active on the target: what "a target with two
		// statuses" counts, so two Exposed effects still count once.
		[[nodiscard]] constexpr std::uint32_t CountDistinctKinds(std::uint32_t a_target, std::int64_t a_nowMs) const noexcept
		{
			std::array<bool, kStatusKindCount> seen{};
			std::uint32_t distinct = 0u;
			for (std::size_t i = 0; i < _count; ++i) {
				const auto& entry = _entries[i];
				if (entry.target != a_target || entry.expiresAtMs <= a_nowMs) {
					continue;
				}
				const auto index = static_cast<std::size_t>(entry.kind);
				if (index < seen.size() && !seen[index]) {
					seen[index] = true;
					++distinct;
				}
			}
			return distinct;
		}

		// The strongest active entry of this kind on this value, ignoring a_exceptSpell.
		[[nodiscard]] constexpr std::optional<StatusEntry> Strongest(
			std::uint32_t a_target, StatusKind a_kind, std::uint32_t a_actorValue, std::int64_t a_nowMs,
			std::uint32_t a_exceptSpell = 0u) const noexcept
		{
			std::optional<StatusEntry> best;
			for (std::size_t i = 0; i < _count; ++i) {
				const auto& entry = _entries[i];
				if (entry.target != a_target || entry.kind != a_kind || entry.actorValue != a_actorValue ||
					entry.expiresAtMs <= a_nowMs || (a_exceptSpell != 0u && entry.spellFormID == a_exceptSpell)) {
					continue;
				}
				if (!best || entry.magnitude > best->magnitude) {
					best = entry;
				}
			}
			return best;
		}

		// Copies the target's active entries into a_out; returns how many.
		template <std::size_t N>
		constexpr std::size_t Collect(std::uint32_t a_target, std::int64_t a_nowMs, std::array<StatusEntry, N>& a_out) const noexcept
		{
			std::size_t written = 0u;
			for (std::size_t i = 0; i < _count && written < N; ++i) {
				const auto& entry = _entries[i];
				if (entry.target == a_target && entry.expiresAtMs > a_nowMs) {
					a_out[written++] = entry;
				}
			}
			return written;
		}

		constexpr void EraseSpell(std::uint32_t a_target, std::uint32_t a_spellFormID, std::uint32_t a_actorValue) noexcept
		{
			EraseIf([&](const StatusEntry& e) {
				return e.target == a_target && e.spellFormID == a_spellFormID && e.actorValue == a_actorValue;
			});
		}

		constexpr void EraseTarget(std::uint32_t a_target) noexcept
		{
			EraseIf([&](const StatusEntry& e) { return e.target == a_target; });
		}

		constexpr void Prune(std::int64_t a_nowMs) noexcept
		{
			EraseIf([&](const StatusEntry& e) { return e.expiresAtMs <= a_nowMs; });
		}

		constexpr void Clear() noexcept { _count = 0u; }
		[[nodiscard]] constexpr std::size_t Size() const noexcept { return _count; }

	private:
		std::array<StatusEntry, kCapacity> _entries{};
		std::size_t _count{ 0u };
	};

	// One value an Exposed spell lowers, for planning the cast.
	struct ExposureValue
	{
		std::uint32_t actorValue{ 0u };
		float magnitude{ 0.0f };
	};

	// What an Exposed cast changes. Only the strongest Calamity Exposed per value
	// applies: the cast goes ahead only if it beats what is there on some value;
	// older entries it matches or beats are removed once it lands, and on values
	// where a stronger one stays, the new spell's own effect is removed.
	struct ExposurePlan
	{
		static constexpr std::size_t kMaxValues = 8u;
		bool improves{ false };
		std::array<StatusEntry, kMaxValues> replaced{};
		std::size_t replacedCount{ 0u };
		std::array<std::uint32_t, kMaxValues> keepStronger{};
		std::size_t keepStrongerCount{ 0u };
	};

	template <std::size_t N>
	[[nodiscard]] constexpr ExposurePlan PlanExposure(
		const StatusLedger& a_ledger, std::uint32_t a_target, std::uint32_t a_spellFormID,
		const std::array<ExposureValue, N>& a_values, std::size_t a_valueCount, std::int64_t a_nowMs) noexcept
	{
		ExposurePlan plan{};
		std::array<StatusEntry, 32> current{};
		const auto currentCount = a_ledger.Collect(a_target, a_nowMs, current);
		for (std::size_t v = 0; v < a_valueCount && v < N; ++v) {
			const auto& value = a_values[v];
			const auto best = a_ledger.Strongest(a_target, StatusKind::kExposed, value.actorValue, a_nowMs, a_spellFormID);
			if (best && best->magnitude > value.magnitude) {
				if (plan.keepStrongerCount < ExposurePlan::kMaxValues) {
					plan.keepStronger[plan.keepStrongerCount++] = value.actorValue;
				}
				continue;
			}
			// Two affixes can share a spell at different strengths. A recast
			// replaces the live one, so a weaker recast is no gain here.
			bool ownStronger = false;
			for (std::size_t i = 0; i < currentCount; ++i) {
				const auto& entry = current[i];
				if (entry.kind == StatusKind::kExposed && entry.spellFormID == a_spellFormID &&
					entry.actorValue == value.actorValue && entry.magnitude > value.magnitude) {
					ownStronger = true;
				}
			}
			if (ownStronger) {
				continue;
			}
			if (!best || value.magnitude > best->magnitude) {
				plan.improves = true;
			}
			for (std::size_t i = 0; i < currentCount; ++i) {
				const auto& entry = current[i];
				if (entry.kind == StatusKind::kExposed && entry.actorValue == value.actorValue &&
					entry.spellFormID != a_spellFormID && entry.magnitude <= value.magnitude &&
					plan.replacedCount < ExposurePlan::kMaxValues) {
					plan.replaced[plan.replacedCount++] = entry;
				}
			}
		}
		return plan;
	}

	// Doom: a mark that bursts after a delay. Marking a target that already waits
	// only moves its timer and keeps the larger burst, so Doom never stacks.
	struct DoomPending
	{
		std::uint64_t affixToken{ 0u };
		std::uint32_t target{ 0u };
		float magnitude{ 0.0f };
		std::int64_t fireAtMs{ 0 };
	};

	class DoomQueue
	{
	public:
		static constexpr std::size_t kCapacity = 32u;

		// Returns false when the queue is full and the target was not already marked.
		constexpr bool Mark(const DoomPending& a_doom) noexcept
		{
			for (std::size_t i = 0; i < _count; ++i) {
				if (_pending[i].target == a_doom.target) {
					_pending[i].fireAtMs = a_doom.fireAtMs;
					// The larger burst stays, with the affix that made it.
					if (a_doom.magnitude > _pending[i].magnitude) {
						_pending[i].magnitude = a_doom.magnitude;
						_pending[i].affixToken = a_doom.affixToken;
					}
					return true;
				}
			}
			if (_count >= kCapacity) {
				return false;
			}
			_pending[_count++] = a_doom;
			return true;
		}

		[[nodiscard]] constexpr std::optional<DoomPending> Find(std::uint32_t a_target) const noexcept
		{
			for (std::size_t i = 0; i < _count; ++i) {
				if (_pending[i].target == a_target) {
					return _pending[i];
				}
			}
			return std::nullopt;
		}

		// Moves every burst due at a_nowMs into a_out (up to N); returns how many.
		template <std::size_t N>
		constexpr std::size_t TakeDue(std::int64_t a_nowMs, std::array<DoomPending, N>& a_out) noexcept
		{
			std::size_t written = 0u;
			std::size_t kept = 0u;
			for (std::size_t i = 0; i < _count; ++i) {
				if (_pending[i].fireAtMs <= a_nowMs && written < N) {
					a_out[written++] = _pending[i];
				} else {
					_pending[kept++] = _pending[i];
				}
			}
			_count = kept;
			return written;
		}

		constexpr void Remove(std::uint32_t a_target) noexcept
		{
			std::size_t kept = 0u;
			for (std::size_t i = 0; i < _count; ++i) {
				if (_pending[i].target != a_target) {
					_pending[kept++] = _pending[i];
				}
			}
			_count = kept;
		}

		constexpr void Clear() noexcept { _count = 0u; }
		[[nodiscard]] constexpr bool Empty() const noexcept { return _count == 0u; }
		[[nodiscard]] constexpr std::size_t Size() const noexcept { return _count; }

	private:
		std::array<DoomPending, kCapacity> _pending{};
		std::size_t _count{ 0u };
	};

	// A target killed before its Doom burst. TESDeathEvent(dead=false) moves its
	// pending Doom here; it never bursts on the dying body, and Contagion reads
	// it at dead=true, which can arrive after the burst time and after the
	// mark's ledger entry expired. Bounded and short-lived.
	struct DyingDoom
	{
		DoomPending doom{};
		std::uint32_t markSpellFormID{ 0u };
		std::int64_t keepUntilMs{ 0 };
	};

	class DyingDoomLedger
	{
	public:
		static constexpr std::size_t kCapacity = 16u;
		static constexpr std::int64_t kGraceMs = 8000;

		constexpr void Keep(const DoomPending& a_doom, std::uint32_t a_markSpellFormID, std::int64_t a_nowMs) noexcept
		{
			const DyingDoom entry{ a_doom, a_markSpellFormID, a_nowMs + kGraceMs };
			std::size_t oldest = 0u;
			for (std::size_t i = 0; i < _count; ++i) {
				if (_entries[i].doom.target == a_doom.target || _entries[i].keepUntilMs <= a_nowMs) {
					_entries[i] = entry;
					return;
				}
				if (_entries[i].keepUntilMs < _entries[oldest].keepUntilMs) {
					oldest = i;
				}
			}
			if (_count < kCapacity) {
				_entries[_count++] = entry;
				return;
			}
			_entries[oldest] = entry;
		}

		[[nodiscard]] constexpr std::optional<DyingDoom> Find(std::uint32_t a_target, std::int64_t a_nowMs) const noexcept
		{
			for (std::size_t i = 0; i < _count; ++i) {
				if (_entries[i].doom.target == a_target && _entries[i].keepUntilMs > a_nowMs) {
					return _entries[i];
				}
			}
			return std::nullopt;
		}

		constexpr void Forget(std::uint32_t a_target) noexcept
		{
			std::size_t kept = 0u;
			for (std::size_t i = 0; i < _count; ++i) {
				if (_entries[i].doom.target != a_target) {
					_entries[kept++] = _entries[i];
				}
			}
			_count = kept;
		}

		constexpr void Clear() noexcept { _count = 0u; }

	private:
		std::array<DyingDoom, kCapacity> _entries{};
		std::size_t _count{ 0u };
	};
}
