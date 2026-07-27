import json
import re
import unittest
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SUFFIX_SPEC = ROOT / "affixes" / "modules" / "keywords.affixes.suffixes.json"
AFFIX_PARSING_SOURCE = (
    ROOT / "skse" / "CalamityAffixes" / "src" / "EventBridge.Config.AffixParsing.cpp"
)

EXPECTED_CHANCE_BY_TIER = {1: 6, 2: 3, 3: 1}
EXPECTED_ASSASSIN_UI = {
    "suffix_assassin_t1": {
        "nameKo": "약간의 암살자: 치명타 확률 +10%, 치명타 피해 +10%",
        "nameEn": "of Minor Assassin: Critical Chance +10%, Critical Damage +10%",
        "critDamageBonusPct": 10,
    },
    "suffix_assassin_t2": {
        "nameKo": "암살자: 치명타 확률 +20%, 치명타 피해 +20%",
        "nameEn": "of the Assassin: Critical Chance +20%, Critical Damage +20%",
        "critDamageBonusPct": 20,
    },
    "suffix_assassin_t3": {
        "nameKo": "위대한 암살자: 치명타 확률 +30%, 치명타 피해 +30%",
        "nameEn": "of Grand Assassin: Critical Chance +30%, Critical Damage +30%",
        "critDamageBonusPct": 30,
    },
}


class SuffixFamilyContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.entries = json.loads(SUFFIX_SPEC.read_text(encoding="utf-8"))
        cls.loader_source = AFFIX_PARSING_SOURCE.read_text(encoding="utf-8")

    def test_all_suffix_families_have_three_weighted_tiers(self) -> None:
        by_family = defaultdict(list)
        for entry in self.entries:
            by_family[entry["family"]].append(entry)

        self.assertEqual(66, len(self.entries))
        self.assertEqual(22, len(by_family))

        for family, entries in by_family.items():
            tiers = {}
            for entry in entries:
                match = re.search(r"_t([1-3])$", entry["id"])
                self.assertIsNotNone(match, entry["id"])
                tier = int(match.group(1))
                self.assertNotIn(tier, tiers, family)
                tiers[tier] = entry

            self.assertEqual({1, 2, 3}, set(tiers), family)
            for tier, expected_chance in EXPECTED_CHANCE_BY_TIER.items():
                entry = tiers[tier]
                self.assertEqual("suffix", entry["slot"], entry["id"])
                self.assertEqual(expected_chance, entry["kid"]["chance"], entry["id"])
                self.assertNotIn(
                    "lootWeight",
                    entry.get("runtime", {}),
                    f"{entry['id']} should use the centralized kid.chance fallback",
                )

    def test_loader_keeps_kid_chance_as_suffix_runtime_loot_weight(self) -> None:
        """The kid.chance fallback must land before the suffix runtime gate.

        ApplyAffixRuntimeGateFromJson returns early for suffixes, so anything
        that runs after it never executes for a suffix affix. lootWeight is
        derived in ApplyAffixKidLootFromJson, and it only reaches suffixes
        because that call precedes the gate. Swapping the two would silently
        leave every suffix at lootWeight 0.

        This used to pin the literal `a_out.lootWeight = a_outKidChancePct;`,
        which broke the moment the expression moved into
        detail::ResolveAffixKidLootWeight -- while the invariant it claimed to
        guard was untouched. Pin the call order instead: that IS the invariant,
        and the value logic is covered by static_assert in
        tests/test_affix_parsing_policy.cpp.
        """
        weight_call = "ApplyAffixKidLootFromJson(a, out, kidChancePct);"
        gate_call = "ApplyAffixRuntimeGateFromJson(runtime, action, kidChancePct, type, out)"
        suffix_early_return = "if (a_out.slot == AffixSlot::kSuffix) {"

        self.assertIn(weight_call, self.loader_source)
        self.assertIn(gate_call, self.loader_source)
        # The early return is what makes the ordering load-bearing rather than
        # merely conventional.
        self.assertIn(suffix_early_return, self.loader_source)
        self.assertLess(
            self.loader_source.index(weight_call),
            self.loader_source.index(gate_call),
            "kid.chance fallback must be applied before the suffix runtime gate returns",
        )

    def test_assassin_suffix_discloses_critical_chance_and_damage(self) -> None:
        by_id = {entry["id"]: entry for entry in self.entries}

        for affix_id, expected in EXPECTED_ASSASSIN_UI.items():
            entry = by_id[affix_id]
            self.assertEqual(expected["nameKo"], entry["name"], affix_id)
            self.assertEqual(expected["nameKo"], entry["nameKo"], affix_id)
            self.assertEqual(expected["nameEn"], entry["nameEn"], affix_id)
            self.assertEqual(
                expected["critDamageBonusPct"],
                entry["runtime"]["action"]["critDamageBonusPct"],
                affix_id,
            )


if __name__ == "__main__":
    unittest.main()
