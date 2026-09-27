#!/usr/bin/env python3
from __future__ import annotations

import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
NOTICES = REPO_ROOT / "THIRD_PARTY_NOTICES.txt"
VENDOR_INCLUDE = REPO_ROOT / "skse" / "CalamityAffixes" / "extern" / "vendor" / "include"

# Vendored header directory or file -> the heading its notice uses.
NOTICE_HEADINGS = {
    "fmt": "{fmt}",
    "nlohmann": "nlohmann/json",
    "rapidcsv.h": "rapidcsv",
    "spdlog": "spdlog",
    "xbyak": "Xbyak",
}


class ThirdPartyNoticesTests(unittest.TestCase):
    """The DLL compiles in CommonLibSSE-NG and the vendored libraries, whose
    MIT/BSD licenses require their notices to ship with the binary."""

    def setUp(self) -> None:
        self.text = NOTICES.read_text(encoding="utf-8")

    def test_every_vendored_library_has_a_notice(self) -> None:
        vendored = sorted(p.name for p in VENDOR_INCLUDE.iterdir())
        unknown = [name for name in vendored if name not in NOTICE_HEADINGS]
        self.assertEqual([], unknown, "add a notice (and a NOTICE_HEADINGS entry) for new vendored code")
        for name in vendored:
            self.assertIn(NOTICE_HEADINGS[name], self.text, name)
        self.assertIn("CommonLibSSE-NG", self.text)

    def test_notices_carry_each_license_text_and_copyright(self) -> None:
        self.assertEqual(4, self.text.count("Permission is hereby granted, free of charge"))
        self.assertEqual(2, self.text.count("Redistribution and use in source and binary forms"))
        for holder in (
            "Ryan-rsm-McKenzie",
            "Gabi Melman",
            "Victor Zverovich",
            "Niels Lohmann",
            "Kristofer Berggren",
            "MITSUNARI Shigeo",
        ):
            self.assertIn(holder, self.text)

    def test_the_package_ships_the_notices(self) -> None:
        build_script = (REPO_ROOT / "tools" / "build_mo2_zip.sh").read_text(encoding="utf-8")
        verifier = (REPO_ROOT / "tools" / "verify_mo2_zip.py").read_text(encoding="utf-8")
        self.assertIn('"${repo_root}/THIRD_PARTY_NOTICES.txt"', build_script)
        self.assertIn('"Docs/THIRD_PARTY_NOTICES.txt"', verifier)


if __name__ == "__main__":
    unittest.main()
