import importlib.util
import json
import re
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("gen_mcm_config", REPO / "tools" / "gen_mcm_config.py")
gen = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(gen)


class GenMcmConfigTests(unittest.TestCase):
    def setUp(self) -> None:
        self.source = json.loads(gen.SOURCE.read_text(encoding="utf-8"))
        self.config = gen.build(self.source)

    def test_committed_config_is_current(self) -> None:
        self.assertEqual(gen.render(self.config), gen.OUTPUT.read_text(encoding="utf-8"))

    def test_each_page_has_one_hidden_toggle_per_language(self) -> None:
        for page in self.config["pages"]:
            toggles = [c for c in page["content"] if c["type"] == "hiddenToggle"]
            self.assertEqual([t["groupControl"] for t in toggles], [1, 2, 3], page["pageDisplayName"])
            self.assertEqual(
                [t["valueOptions"]["propertyName"] for t in toggles],
                ["McmShowEnglish", "McmShowKorean", "McmShowBoth"],
            )

    def test_localized_controls_come_in_three_skipped_copies(self) -> None:
        for page in self.config["pages"]:
            for control in page["content"]:
                if control["type"] in ("hiddenToggle", "empty"):
                    continue
                self.assertIn(control.get("groupCondition"), (1, 2, 3), control)
                self.assertEqual(control.get("groupBehavior"), "skip", control)
            ids = [c.get("id") for c in page["content"] if c.get("id")]
            for control_id in set(ids):
                self.assertEqual(ids.count(control_id), 3, control_id)

    def test_single_language_copies_have_no_other_script(self) -> None:
        hangul = re.compile("[가-힣]")
        for page in self.config["pages"]:
            for control in page["content"]:
                if control["type"] == "enum":
                    continue
                if control.get("groupCondition") == 1:
                    for field in ("text", "help"):
                        self.assertIsNone(hangul.search(control.get(field, "")), (control.get("id"), field))
                if control.get("groupCondition") == 2:
                    self.assertNotIn(" / ", control.get("help", ""), control.get("id"))

    def test_bilingual_help_fits_the_info_box(self) -> None:
        for page in self.config["pages"]:
            for control in page["content"]:
                if control.get("groupCondition") == 3 and "help" in control:
                    self.assertLessEqual(len(control["help"]), gen.MAX_BOTH_HELP_CHARS, control.get("id"))

    def test_every_help_fits_the_info_box(self) -> None:
        for page in self.config["pages"]:
            for control in page["content"]:
                if "help" in control:
                    self.assertLessEqual(gen.help_width(control["help"]), gen.MAX_HELP_WIDTH, control.get("id"))

    def test_script_declares_the_group_properties(self) -> None:
        script = (REPO / "Data" / "Scripts" / "Source" / "CalamityAffixes_MCMConfig.psc").read_text(encoding="utf-8")
        for _, _, prop in gen.LANGUAGES:
            self.assertRegex(script, rf"bool Property {prop} = (true|false) Auto")
        self.assertIn("ForcePageReset()", script)


if __name__ == "__main__":
    unittest.main()
