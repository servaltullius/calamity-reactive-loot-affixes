import re
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SRC = REPO / "skse" / "CalamityAffixes" / "src"


class PrismaPrewarmTests(unittest.TestCase):
    """The Prisma view is created while a game loads, not on the first inventory
    open, where creating it cost a 2-4 s hitch (measured 2026-10-26)."""

    def test_view_is_created_after_load_and_new_game(self) -> None:
        main = (SRC / "main.cpp").read_text(encoding="utf-8")
        for message in ("kPostLoadGame", "kNewGame"):
            case = re.search(rf"case SKSE::MessagingInterface::{message}:(.*?)break;", main, re.S)
            self.assertIsNotNone(case, message)
            self.assertIn("PrismaTooltip::PrewarmView();", case.group(1), message)

    def test_prewarm_only_queues_view_creation(self) -> None:
        source = (SRC / "PrismaTooltip.cpp").read_text(encoding="utf-8")
        body = re.search(r"void PrewarmView\(\)\s*\{(.*?)\n\t\}", source, re.S)
        self.assertIsNotNone(body)
        self.assertIn("QueueTask(", body.group(1))
        self.assertIn("EnsurePrisma(false)", body.group(1))

    def test_worker_keeps_ticking_while_the_view_loads(self) -> None:
        lifecycle = (SRC / "PrismaTooltip.ViewLifecycle.inl").read_text(encoding="utf-8")
        self.assertIn("g_prismaStatus.load() != PrismaAvailabilityStatus::kViewLoading", lifecycle)


if __name__ == "__main__":
    unittest.main()
