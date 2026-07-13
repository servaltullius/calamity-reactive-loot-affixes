#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import unittest
from pathlib import Path


class PrismaFailsoftContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        cls.lifecycle = (
            cls.repo_root
            / "skse"
            / "CalamityAffixes"
            / "src"
            / "PrismaTooltip.ViewLifecycle.inl"
        ).read_text(encoding="utf-8")
        cls.shared_state = (
            cls.repo_root
            / "skse"
            / "CalamityAffixes"
            / "src"
            / "PrismaTooltip.SharedState.inl"
        ).read_text(encoding="utf-8")
        cls.core = (
            cls.repo_root / "skse" / "CalamityAffixes" / "src" / "PrismaTooltip.cpp"
        ).read_text(encoding="utf-8")
        cls.handlers = (
            cls.repo_root
            / "skse"
            / "CalamityAffixes"
            / "src"
            / "EventBridge.Triggers.ModCallback.Handlers.cpp"
        ).read_text(encoding="utf-8")
        cls.event_names = (
            cls.repo_root
            / "skse"
            / "CalamityAffixes"
            / "include"
            / "CalamityAffixes"
            / "EventNames.h"
        ).read_text(encoding="utf-8")
        cls.mode_control = (
            cls.repo_root
            / "Data"
            / "Scripts"
            / "Source"
            / "CalamityAffixes_ModeControl.psc"
        ).read_text(encoding="utf-8")
        cls.mcm_path = (
            cls.repo_root / "Data" / "MCM" / "Config" / "CalamityAffixes" / "config.json"
        )
        cls.mcm = json.loads(cls.mcm_path.read_text(encoding="utf-8"))

    def test_view_creation_is_validated_before_use(self) -> None:
        guard = "if (!candidate || !g_api->IsValid(candidate))"
        self.assertIn(guard, self.lifecycle)
        self.assertLess(
            self.lifecycle.index(guard),
            self.lifecycle.index("g_api->SetOrder(g_view, kViewOrder)"),
        )
        self.assertIn("ResetPrismaViewForRetry", self.lifecycle)
        self.assertIn("g_view = 0;", self.lifecycle)
        self.assertIn("g_api->Destroy(expiredView)", self.lifecycle)
        self.assertLess(
            self.lifecycle.index(
                "ResetPrismaViewForRetry(PrismaAvailabilityStatus::kDomReadyTimedOut"
            ),
            self.lifecycle.index("g_api->Destroy(expiredView)"),
        )

    def test_dom_identity_cache_reset_and_backoff_are_preserved(self) -> None:
        for marker in (
            "kPrismaRetryInterval",
            "kPrismaDomReadyTimeout",
            "g_activePrismaView",
            "g_domReadyView",
            "g_prismaViewReadinessMutex",
            "g_pendingDomReadyViews",
            "g_nextPrismaAttemptAt",
        ):
            self.assertIn(marker, self.shared_state)
        for marker in (
            "g_domReadyView.load(std::memory_order_acquire) == g_view",
            "RecordPrismaViewDomReady",
            "ActivatePrismaViewReadiness",
            "DeactivatePrismaViewReadiness",
            "g_lastTooltip.clear()",
            "g_viewCache = {}",
            "g_runewordCache.baseListJson.clear()",
            "g_runewordCache.recipeListJson.clear()",
            "g_runewordCache.panelStateJson.clear()",
        ):
            self.assertIn(marker, self.lifecycle)
        self.assertIn(
            "g_domReadyView.load(std::memory_order_acquire) == activeView",
            self.core,
        )
        callback = re.search(
            r"void RecordPrismaViewDomReady\(.*?\n\t\}",
            self.lifecycle,
            re.DOTALL,
        )
        self.assertIsNotNone(callback)
        assert callback is not None
        self.assertNotIn("g_view", callback.group(0))
        self.assertIn("g_activePrismaView", callback.group(0))
        self.assertIn("g_prismaViewReadinessMutex", callback.group(0))

    def test_status_check_does_not_force_destructive_retry(self) -> None:
        report_status = re.search(
            r"void ReportStatus\(\).*?\n\t\}",
            self.core,
            re.DOTALL,
        )
        self.assertIsNotNone(report_status)
        assert report_status is not None
        self.assertIn("EnsurePrisma(false)", report_status.group(0))
        self.assertNotIn("EnsurePrisma(true)", report_status.group(0))
        self.assertIn(
            "g_prismaStatus.load() != PrismaAvailabilityStatus::kViewLoading",
            self.lifecycle,
        )

    def test_panel_callbacks_do_not_claim_unverified_success(self) -> None:
        self.assertNotIn("Calamity: Prisma panel toggled", self.handlers)
        self.assertNotIn("Calamity: Prisma panel ON", self.handlers)
        ui_handler = re.search(
            r"bool EventBridge::HandleUiModCallback\(.*?\n\t\}",
            self.handlers,
            re.DOTALL,
        )
        self.assertIsNotNone(ui_handler)
        assert ui_handler is not None
        self.assertIn("PrismaTooltip::ReportStatus()", ui_handler.group(0))
        self.assertIn("CalamityAffixes_UI_Status", self.event_names)
        self.assertIn('Emit("CalamityAffixes_UI_Status"', self.mode_control)

    def test_mcm_exposes_status_runeword_status_and_safe_recovery(self) -> None:
        prisma_page = next(
            page
            for page in self.mcm["pages"]
            if page["pageDisplayName"].startswith("Prisma UI")
        )
        functions = {
            entry.get("action", {}).get("function")
            for entry in prisma_page["content"]
            if isinstance(entry, dict)
        }
        self.assertIn("CheckPrismaUiStatus", functions)
        self.assertIn("RunewordStatus", functions)
        self.assertIn("RecoverMissingCurrency", functions)

        self.assertIn("CalamityAffixes_MCM_RecoverMiscCurrency", self.event_names)
        self.assertIn(
            'Emit("CalamityAffixes_MCM_RecoverMiscCurrency"',
            self.mode_control,
        )
        recovery_handler = re.search(
            r"if \(a_eventName == kMcmRecoverMiscCurrencyEvent\) \{(?P<body>.*?)\n\t\t\}",
            self.handlers,
            re.DOTALL,
        )
        self.assertIsNotNone(recovery_handler)
        assert recovery_handler is not None
        body = recovery_handler.group("body")
        self.assertIn("RecoverMiscCurrency()", body)
        self.assertNotIn("GrantRecoveryPack", body)


if __name__ == "__main__":
    unittest.main()
