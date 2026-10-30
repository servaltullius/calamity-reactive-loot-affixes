import re
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SKSE = REPO / "skse" / "CalamityAffixes"
SRC = SKSE / "src"


class V231RuntimeGuardTests(unittest.TestCase):
    """Fixes from the 2026-10-27 review of v2.3.0 that need the game to run, so
    they are kept by structure here (the pure parts have static_assert tests)."""

    def test_an_affix_that_fails_to_parse_keeps_its_saved_tokens(self) -> None:
        source = (SRC / "EventBridge.Config.AffixParsing.cpp").read_text(encoding="utf-8")
        body = re.search(
            r"void EventBridge::ParseConfiguredAffixesFromJson\(.*?\n\t\}\n", source, re.S
        )
        self.assertIsNotNone(body)
        loop = body.group(0)
        # Every failure inside the per-affix parse returns false instead of
        # skipping the protection with `continue`.
        parse = loop[loop.index("parsed = [&]() -> bool {") : loop.index("}();")]
        self.assertNotIn("continue;", parse)
        self.assertIn("configuredToken = out.token;", parse)
        # The protection runs after the catch, so a JSON exception is covered too.
        after_catch = loop[loop.index("catch (const nlohmann::json::exception& e)") :]
        self.assertIn("if (!parsed) {", after_catch)
        self.assertIn("_unresolvedConfiguredAffixTokens.insert(configuredToken);", after_catch)

    def test_a_low_health_task_from_before_a_load_leaves_newer_damage(self) -> None:
        source = (SRC / "Hooks.Dispatch.cpp").read_text(encoding="utf-8")
        schedule = source[source.index("void SchedulePlayerLowHealthOnlyCheck(") : source.index("void InvalidateDeferredTasks()")]
        self.assertIn("s_lowHealthOnlyGeneration = runtimeGeneration;", schedule)
        task = schedule[schedule.index("tasks->AddTask(") :]
        guard = task.index("if (s_lowHealthOnlyGeneration != runtimeGeneration)")
        self.assertLess(guard, task.index("damage = s_lowHealthOnlyDamage;"))

    def test_status_records_use_the_paused_time_clock(self) -> None:
        private_api = (SKSE / "include" / "CalamityAffixes" / "detail" / "EventBridge.PrivateApi.inl").read_text(encoding="utf-8")
        clock = private_api[private_api.index("static std::int64_t StatusClockNowMs()") :]
        clock = clock[: clock.index("\n\t\t}\n")]
        self.assertIn("StatusClock::NowMs()", clock)
        self.assertNotIn("steady_clock", clock)
        # A running game unfreezes the clock even if a close event was missed.
        self.assertIn("!ui->GameIsPaused()", clock)
        bridge = (SRC / "EventBridge.cpp").read_text(encoding="utf-8")
        self.assertIn("ui->AddEventSink<RE::MenuOpenCloseEvent>(&g_statusPauseMenuSink);", bridge)

    def test_status_code_reads_time_only_through_the_status_clock(self) -> None:
        status = (SRC / "EventBridge.Actions.Status.cpp").read_text(encoding="utf-8")
        self.assertNotIn("steady_clock", status)

    def test_every_editor_id_the_runtime_names_is_in_the_plugin(self) -> None:
        # Two runeword stand-in spells named records that left the data in an
        # earlier rework, so their lookups always returned null (review 2026-10-27).
        esp = (REPO / "Data" / "CalamityAffixes.esp").read_bytes()
        ids = set()
        for path in list(SRC.rglob("*")) + list((SKSE / "include").rglob("*")):
            if path.suffix in (".cpp", ".h", ".inl"):
                ids |= set(re.findall(r'"(CAFF_[A-Z0-9_]+)"', path.read_text(encoding="utf-8", errors="replace")))
        full_ids = sorted(i for i in ids if not i.endswith("_"))  # "CAFF_X_" literals are prefixes
        self.assertGreater(len(full_ids), 40)
        missing = [i for i in full_ids if i.encode() + b"\0" not in esp]
        self.assertEqual(missing, [])

    def test_crafting_refuses_an_item_with_an_affix_that_did_not_load(self) -> None:
        policy = (SRC / "EventBridge.Loot.Runeword.Policy.cpp").read_text(encoding="utf-8")
        check = policy[policy.index("bool EventBridge::HasUnloadedConfiguredAffix(") :]
        check = check[: check.index("\n\t}\n")]
        self.assertIn("_unresolvedConfiguredAffixTokens.contains(token)", check)
        self.assertIn("!_unknownAffixTokenPruningAllowed", check)
        self.assertIn("RunewordApplyBlockReason::kUnloadedAffix", policy)
        reforge = (SRC / "EventBridge.Loot.Runeword.Reforge.cpp").read_text(encoding="utf-8")
        craft = reforge[reforge.index("EventBridge::CraftSelectedAffixes(") : reforge.index("EventBridge::ExchangeCraftingCurrency(")]
        # Before the layout is judged, so Scour and Remove Runeword are refused too.
        self.assertLess(craft.index("HasUnloadedConfiguredAffix(previous)"), craft.index("ResolveAffixHead("))
        expand = (SRC / "EventBridge.Loot.Runeword.Expand.cpp").read_text(encoding="utf-8")
        self.assertIn("HasUnloadedConfiguredAffix(previousSlots)", expand)
        # A new block reason must not fall through to applying the runeword.
        transmute = (SRC / "EventBridge.Loot.Runeword.Transmute.cpp").read_text(encoding="utf-8")
        apply = transmute[transmute.index("bool EventBridge::ApplyRunewordResult(") :]
        self.assertLess(
            apply.index("if (blockReason != RunewordApplyBlockReason::kNone)"),
            apply.index("PromoteTokenToPrimary"),
        )
        # The panel shows the message in the player's language.
        constants = (SKSE / "include" / "CalamityAffixes" / "detail" / "EventBridge.Constants.inl").read_text(encoding="utf-8")
        message = re.search(r'kUnloadedAffixCraftMessage =\s*"([^"]+)"', constants).group(1)
        i18n = (REPO / "Data" / "PrismaUI" / "views" / "CalamityAffixes" / "scripts" / "i18n.js").read_text(encoding="utf-8")
        self.assertIn(f'"{message}": [', i18n)

    def test_low_health_and_archmage_use_the_bar_maximum(self) -> None:
        policy = (SRC / "EventBridge.Triggers.Policy.cpp").read_text(encoding="utf-8")
        status = (SRC / "EventBridge.Actions.Status.cpp").read_text(encoding="utf-8")
        archmage = (SRC / "EventBridge.Actions.Archmage.cpp").read_text(encoding="utf-8")
        for source in (policy, status):
            self.assertNotIn("GetPermanentActorValue(RE::ActorValue::kHealth)", source)
            self.assertIn("ResolveActorValueMax(", source)
        self.assertNotIn("GetPermanentActorValue(RE::ActorValue::kMagicka)", archmage)
        self.assertIn("ResolveActorValueMax(a_caster, RE::ActorValue::kMagicka)", archmage)

    def test_a_load_closes_the_panel_without_waiting_for_a_tick(self) -> None:
        source = (SRC / "PrismaTooltip.cpp").read_text(encoding="utf-8")
        body = source[source.index("void OnPreLoadGame()") :]
        body = body[: body.index("\n\t}\n")]
        self.assertIn("QueueTask(", body)
        self.assertIn("ApplyControlPanelState(false);", body)


if __name__ == "__main__":
    unittest.main()
