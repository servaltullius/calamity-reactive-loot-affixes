import hashlib
import json
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CORE_SPEC = ROOT / "affixes" / "modules" / "keywords.affixes.core.json"


def _sha_lines(values: list[str]) -> str:
    return hashlib.sha256("\n".join(values).encode("utf-8")).hexdigest()


class CoreIdentityWaveTwoTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.entries = json.loads(CORE_SPEC.read_text(encoding="utf-8"))
        cls.by_id = {entry["id"]: entry for entry in cls.entries}

    def test_core_affix_and_record_identity_order_is_unchanged(self) -> None:
        self.assertEqual(83, len(self.entries))
        self.assertEqual(
            "264991fe3ae3249cda7957e70c878d6d7c21db3e67bb776b39ca887326fade19",
            _sha_lines([entry["id"] for entry in self.entries]),
        )
        self.assertEqual(
            "a910701b744f15ec7215f293e2b98bb10b077b2906f6c370ccd124c1678cdca1",
            _sha_lines([entry["editorId"] for entry in self.entries]),
        )

        record_editor_ids: list[str] = []
        for entry in self.entries:
            records = entry.get("records", {})
            if records.get("magicEffect", {}).get("editorId"):
                record_editor_ids.append(records["magicEffect"]["editorId"])
            record_editor_ids.extend(
                record["editorId"]
                for record in records.get("magicEffects", [])
                if record.get("editorId")
            )
            if records.get("spell", {}).get("editorId"):
                record_editor_ids.append(records["spell"]["editorId"])
            record_editor_ids.extend(
                record["editorId"]
                for record in records.get("spells", [])
                if record.get("editorId")
            )

        self.assertEqual(81, len(record_editor_ids))
        self.assertEqual(
            "6496747fc3d6e332b296cd3d7a2e1fd2705f3b4993cec372fdcb396ddef02ee7",
            _sha_lines(record_editor_ids),
        )

        records_payload = json.dumps(
            [entry.get("records") for entry in self.entries],
            ensure_ascii=False,
            sort_keys=True,
            separators=(",", ":"),
        ).encode("utf-8")
        self.assertEqual(
            "e17f11cf71414ec4d1ad9ff887a2662baf5208e26f492a9a81e8b8c3b350cdca",
            hashlib.sha256(records_payload).hexdigest(),
            "wave two must not mutate any shared or owned record payload",
        )

    def test_shadow_stride_is_a_kill_haste_with_three_growth_stages(self) -> None:
        entry = self.by_id["shadow_stride"]
        runtime = entry["runtime"]
        action = runtime["action"]
        self.assertEqual("Kill", runtime["trigger"])
        self.assertEqual(100, runtime["procChancePercent"])
        self.assertEqual(6, runtime["icdSeconds"])
        self.assertNotIn("requireRecentlyKillSeconds", runtime)
        self.assertEqual("CAFF_SPEL_SWAP_JACKPOT_HASTE", action["spellEditorId"])
        self.assertEqual("Self", action["applyTo"])
        self.assertIs(True, action["noHitEffectArt"])
        self.assertEqual(
            {"xpPerProc": 1, "thresholds": [0, 10, 30], "multipliers": [1.0, 1.2, 1.4]},
            action["evolution"],
        )
        self.assertIn("+15%", entry["nameKo"])
        self.assertIn("+18%/+21%", entry["nameKo"])

    def test_nourishing_flame_is_a_reliable_post_kill_leech_window(self) -> None:
        entry = self.by_id["nourishing_flame"]
        runtime = entry["runtime"]
        scaling = runtime["action"]["magnitudeScaling"]
        self.assertEqual("Hit", runtime["trigger"])
        self.assertEqual(100, runtime["procChancePercent"])
        self.assertEqual(5, runtime["requireRecentlyKillSeconds"])
        self.assertEqual(2, runtime["icdSeconds"])
        self.assertEqual(
            {
                "source": "HitPhysicalDealt",
                "mult": 0.10,
                "add": 6,
                "min": 6,
                "max": 50,
                "spellBaseAsMin": True,
            },
            scaling,
        )

    def test_three_debuffs_use_runtime_overrides_without_record_mutation(self) -> None:
        expected = {
            "mana_knot": {
                "lucky": 35,
                "icd": 5,
                "spell": "CAFF_SPEL_CHAOS_CURSE_DRAIN_MAGREGEN",
                "magnitude": 100,
            },
            "shock_weakness": {
                "lucky": 35,
                "icd": 4,
                "spell": "CAFF_SPEL_SHOCK_SHRED",
                "magnitude": 25,
            },
            "death_mark": {
                "lucky": 25,
                "icd": 1,
                "per_target": 10,
                "spell": "CAFF_SPEL_CHAOS_CURSE_SUNDER",
                "magnitude": 200,
            },
        }
        for affix_id, contract in expected.items():
            with self.subTest(affix_id=affix_id):
                runtime = self.by_id[affix_id]["runtime"]
                action = runtime["action"]
                self.assertEqual("Hit", runtime["trigger"])
                self.assertEqual(100, runtime["procChancePercent"])
                self.assertEqual(contract["lucky"], runtime["luckyHitChancePercent"])
                self.assertEqual(1, runtime["luckyHitProcCoefficient"])
                self.assertEqual(contract["icd"], runtime["icdSeconds"])
                self.assertEqual(contract["spell"], action["spellEditorId"])
                self.assertEqual(contract["magnitude"], action["magnitudeOverride"])
                if "per_target" in contract:
                    self.assertEqual(contract["per_target"], runtime["perTargetICDSeconds"])
                self.assertNotIn("requireRecentlyKillSeconds", runtime)

    def test_ice_form_has_a_usable_proc_window_and_keeps_the_root_spell(self) -> None:
        entry = self.by_id["ice_form"]
        runtime = entry["runtime"]
        self.assertEqual(100, runtime["procChancePercent"])
        self.assertEqual(15, runtime["luckyHitChancePercent"])
        self.assertEqual(1, runtime["luckyHitProcCoefficient"])
        self.assertEqual(10, runtime["icdSeconds"])
        self.assertEqual("CAFF_SPEL_TRAP_IRONJAW_SNARE", runtime["action"]["spellEditorId"])

    def test_all_wave_two_display_strings_are_synchronized(self) -> None:
        for affix_id in (
            "shadow_stride",
            "nourishing_flame",
            "mana_knot",
            "shock_weakness",
            "death_mark",
            "ice_form",
        ):
            with self.subTest(affix_id=affix_id):
                entry = self.by_id[affix_id]
                self.assertEqual(entry["name"], entry["nameKo"])
                self.assertTrue(entry["nameEn"])


if __name__ == "__main__":
    unittest.main()
