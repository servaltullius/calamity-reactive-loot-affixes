import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CORE_SPEC = ROOT / "affixes" / "modules" / "keywords.affixes.core.json"


EXPECTED_FAMILIES = {
    "mage_armor": {"mage_armor_t1", "mage_armor_t2"},
    "scroll_mastery": {
        "scroll_mastery_t1",
        "scroll_mastery_t2",
        "scroll_mastery_t3",
        "scroll_mastery_t4",
    },
    "fire_infusion": {"fire_infusion_50", "fire_infusion_100"},
    "frost_infusion": {"frost_infusion_50", "frost_infusion_100"},
    "shock_infusion": {"shock_infusion_50", "shock_infusion_100"},
    "archmage": {"archmage_t1", "archmage_t2", "archmage_t3", "archmage_t4"},
    "death_pyre": {"death_pyre_t1", "death_pyre_t2", "death_pyre_t3"},
    "conjured_pyre": {"conjured_pyre_t1", "conjured_pyre_t2"},
}


class CoreAffixFamilyContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.entries = json.loads(CORE_SPEC.read_text(encoding="utf-8"))
        cls.by_id = {entry["id"]: entry for entry in cls.entries}

    def test_tiered_prefix_families_are_exclusive_and_keep_intended_rarity(self) -> None:
        for family, expected_ids in EXPECTED_FAMILIES.items():
            actual_ids = {
                entry["id"] for entry in self.entries if entry.get("family") == family
            }
            self.assertEqual(expected_ids, actual_ids, family)

            for affix_id in expected_ids:
                entry = self.by_id[affix_id]
                self.assertEqual("prefix", entry["slot"], affix_id)
                self.assertEqual(
                    entry["kid"]["chance"],
                    entry["runtime"]["lootWeight"],
                    f"{affix_id} must use its legacy KID rarity as runtime loot weight",
                )


if __name__ == "__main__":
    unittest.main()
