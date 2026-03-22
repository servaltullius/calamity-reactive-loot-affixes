#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path


class GenPrefixDocTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        module_path = cls.repo_root / "tools" / "gen_prefix_doc.py"
        spec = importlib.util.spec_from_file_location("gen_prefix_doc", module_path)
        if spec is None or spec.loader is None:
            raise RuntimeError(f"Unable to load module: {module_path}")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        cls.gen_prefix_doc = module

    def test_categorize_skips_internal_entries(self) -> None:
        categories = self.gen_prefix_doc.categorize(
            [
                {"id": "storm_call", "nameKo": "폭풍 소환"},
                {"id": "internal_hidden", "nameKo": "INTERNAL: Hidden"},
            ]
        )

        flattened_ids = [
            entry["id"]
            for grouped in categories.values()
            for _, entry in grouped
        ]

        self.assertIn("storm_call", flattened_ids)
        self.assertNotIn("internal_hidden", flattened_ids)

    def test_format_entry_uses_mind_over_matter_damage_to_magicka_pct(self) -> None:
        entry = {
            "id": "mage_armor_t1",
            "nameKo": "마법사의 갑옷 I: 물리 피해 10%를 매지카로 전환",
            "nameEn": "Mage Armor I: Redirect 10% of physical hit damage to Magicka",
            "kid": {"type": "Armor"},
            "runtime": {
                "trigger": "IncomingHit",
                "procChancePercent": 100.0,
                "action": {
                    "type": "MindOverMatter",
                    "damageToMagickaPct": 10.0,
                    "maxRedirectPerHit": 35.0,
                },
            },
        }

        rendered = self.gen_prefix_doc.format_entry(0, entry)

        self.assertIn("한글 표시: 마법사의 갑옷 I: 물리 피해 10%를 매지카로 전환", rendered)
        self.assertIn("영문 표시: Mage Armor I: Redirect 10% of physical hit damage to Magicka", rendered)
        self.assertNotIn("물리 피해 0%", rendered)

    def test_format_entry_uses_spawn_trap_arm_delay_and_ttl(self) -> None:
        entry = {
            "id": "plague_spore",
            "nameKo": "역병 포자: 적중 시 20% 확률로 0.6초 후 포자 폭발(반경 150, 독 피해 3/s, 4초). 1.5초마다 발동.",
            "nameEn": "Plague Spore: 20% chance on hit to trigger a spore burst after 0.6s (radius 150, poison 3/s, 4s). Triggers every 1.5s.",
            "kid": {"type": "Weapon"},
            "runtime": {
                "trigger": "Hit",
                "procChancePercent": 20.0,
                "icdSeconds": 1.5,
                "action": {
                    "type": "SpawnTrap",
                    "radius": 160.0,
                    "armDelaySeconds": 0.6,
                    "ttlSeconds": 3.0,
                    "maxActive": 1,
                    "maxTriggers": 1,
                    "requireWeaponHit": True,
                    "spellEditorId": "CAFF_SPEL_DOT_BLOOM_POISON",
                },
            },
        }

        rendered = self.gen_prefix_doc.format_entry(0, entry)

        self.assertIn("한글 표시: 역병 포자: 적중 시 20% 확률로 0.6초 후 포자 폭발(반경 150, 독 피해 3/s, 4초). 1.5초마다 발동.", rendered)
        self.assertNotIn("0초 후, 0초 유지", rendered)

    def test_format_entry_expands_fractional_corpse_health_pct(self) -> None:
        entry = {
            "id": "ember_pyre",
            "nameKo": "잔불 장작: 처치 시 18% 확률로 시체 폭발 (반경 320, 최대체력 8% + 14 화염). 1초마다 발동.",
            "nameEn": "Ember Pyre: 18% chance on kill to explode corpses (radius 320, 8% corpse max health + 14 fire). Triggers every 1s.",
            "kid": {"type": "Weapon"},
            "runtime": {
                "trigger": "Kill",
                "procChancePercent": 18.0,
                "icdSeconds": 1.0,
                "action": {
                    "type": "CorpseExplosion",
                    "radius": 320.0,
                    "flatDamage": 14.0,
                    "pctOfCorpseMaxHealth": 0.08,
                    "maxChainDepth": 2,
                },
            },
        }

        rendered = self.gen_prefix_doc.format_entry(0, entry)

        self.assertIn("한글 표시: 잔불 장작: 처치 시 18% 확률로 시체 폭발 (반경 320, 최대체력 8% + 14 화염). 1초마다 발동.", rendered)
        self.assertNotIn("시체 최대체력 0.08%", rendered)

    def test_format_entry_uses_exact_ingame_display_text_as_primary_copy(self) -> None:
        entry = {
            "id": "storm_call",
            "nameKo": "폭풍 소환: 38% 확률로 번개 피해 10 + 적중의 10%. 1.5초마다 발동.",
            "nameEn": "Storm Call: 38% chance to deal Shock damage 10 + 10% of hit. Triggers every 1.5s.",
            "kid": {"type": "Weapon"},
            "runtime": {
                "trigger": "Hit",
                "procChancePercent": 38.0,
                "luckyHitChancePercent": 38.0,
                "icdSeconds": 1.5,
                "action": {
                    "type": "CastSpell",
                    "spellEditorId": "CAFF_SPEL_ARC_LIGHTNING",
                    "applyTo": "Target",
                },
            },
        }

        rendered = self.gen_prefix_doc.format_entry(0, entry)

        self.assertIn("한글 표시: 폭풍 소환: 38% 확률로 번개 피해 10 + 적중의 10%. 1.5초마다 발동.", rendered)
        self.assertIn("영문 표시: Storm Call: 38% chance to deal Shock damage 10 + 10% of hit. Triggers every 1.5s.", rendered)
        self.assertNotIn("발동:", rendered)


if __name__ == "__main__":
    unittest.main()
