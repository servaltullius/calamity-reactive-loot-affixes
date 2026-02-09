#include "CalamityAffixes/LootUiGuards.h"

#include <array>

using CalamityAffixes::ResolveRunewordBaseCycleCursor;
using CalamityAffixes::ResolveTooltipCandidateSelection;
using CalamityAffixes::TooltipResolutionCandidate;

static_assert([] {
	constexpr std::array<std::uint64_t, 4> candidates{ 11u, 22u, 33u, 44u };
	const auto cursor = ResolveRunewordBaseCycleCursor(std::span(candidates), 33u);
	return cursor.has_value() && *cursor == 2u;
}(),
	"ResolveRunewordBaseCycleCursor: returns the selected index when present");

static_assert([] {
	constexpr std::array<std::uint64_t, 3> candidates{ 11u, 22u, 33u };
	return !ResolveRunewordBaseCycleCursor(std::span(candidates), 77u).has_value();
}(),
	"ResolveRunewordBaseCycleCursor: rejects unknown instance keys");

static_assert([] {
	constexpr std::array<std::uint64_t, 1> candidates{ 11u };
	return !ResolveRunewordBaseCycleCursor(std::span(candidates), 0u).has_value();
}(),
	"ResolveRunewordBaseCycleCursor: rejects zero instance key");

static_assert([] {
	constexpr std::array<TooltipResolutionCandidate, 2> candidates{ {
		TooltipResolutionCandidate{ .rowName = "A", .affixToken = 1u, .instanceKey = 100u },
		TooltipResolutionCandidate{ .rowName = "B", .affixToken = 2u, .instanceKey = 200u },
	} };
	const auto result = ResolveTooltipCandidateSelection(std::span(candidates), "");
	return result.matchedIndex.has_value() && *result.matchedIndex == 0u && !result.ambiguous;
}(),
	"ResolveTooltipCandidateSelection: empty selected row falls back to first candidate");

static_assert([] {
	constexpr std::array<TooltipResolutionCandidate, 3> candidates{ {
		TooltipResolutionCandidate{ .rowName = "Stormcloak", .affixToken = 10u, .instanceKey = 100u },
		TooltipResolutionCandidate{ .rowName = "Stormcloak", .affixToken = 10u, .instanceKey = 100u },
		TooltipResolutionCandidate{ .rowName = "Leather", .affixToken = 20u, .instanceKey = 200u },
	} };
	const auto result = ResolveTooltipCandidateSelection(std::span(candidates), "Stormcloak");
	return result.matchedIndex.has_value() && *result.matchedIndex == 0u && !result.ambiguous;
}(),
	"ResolveTooltipCandidateSelection: allows duplicate rows when they resolve to the same instance+affix");

static_assert([] {
	constexpr std::array<TooltipResolutionCandidate, 3> candidates{ {
		TooltipResolutionCandidate{ .rowName = "Stormcloak", .affixToken = 10u, .instanceKey = 100u },
		TooltipResolutionCandidate{ .rowName = "Stormcloak", .affixToken = 10u, .instanceKey = 101u },
		TooltipResolutionCandidate{ .rowName = "Leather", .affixToken = 20u, .instanceKey = 200u },
	} };
	const auto result = ResolveTooltipCandidateSelection(std::span(candidates), "Stormcloak");
	return result.ambiguous && !result.matchedIndex.has_value();
}(),
	"ResolveTooltipCandidateSelection: marks mixed candidates with the same row as ambiguous");
