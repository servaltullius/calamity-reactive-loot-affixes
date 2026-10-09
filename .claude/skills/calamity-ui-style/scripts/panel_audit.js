#!/usr/bin/env node
// Layout audit for the Calamity Prisma panel in headless Chromium.
//
// Fills the panel with game-sized data (all runeword recipes from
// affixes/*.json, long base names, a full equipped build), then for every
// size x language x tab counts:
//   CLIPPED        overflow hidden/clip and the content is larger than the box
//   TRUNCATED      same, but by design (text-overflow: ellipsis / line-clamp);
//                  informational -- check the full text is reachable (title, detail)
//   NESTED-SCROLL  a scrolling box inside another scrolling box (the 2.0.1 miss)
//   NO-BAR         a scrolling box whose scrollbar takes no width
//   UNREACHABLE    a button/input/text that stays <90% visible after scrollIntoView
//
// Chromium is an approximation: Prisma renders with Ultralight (WebKit). A clean
// run here is necessary, not sufficient -- confirm in game.
//
// Usage (from the repo root):
//   node .claude/skills/calamity-ui-style/scripts/panel_audit.js \
//     [--view Data/PrismaUI/views/CalamityAffixes] [--ref v2.2.8] \
//     [--sizes 1320x860,1900x1060,1100x720] [--langs en,ko,both] \
//     [--tabs affix,runeword,advanced] [--out <dir>] [--shots] [--compare v2.2.8]
// --ref <git ref>      audit that ref's view (git archive) instead of the work tree.
// --compare <git ref>  audit both and report only findings the work tree adds.
//                      Use this after a change: the released panel is not clean
//                      (recipe list and action summary scroll inside the
//                      workspace on purpose), so absolute counts mislead.
// Exit code 1 when a non-TRUNCATED finding (or, with --compare, a new one) exists.

"use strict";

const fs = require("fs");
const http = require("http");
const os = require("os");
const path = require("path");
const { execFileSync } = require("child_process");

function parseArgs(argv) {
  const args = {
    view: "Data/PrismaUI/views/CalamityAffixes",
    ref: "",
    sizes: "1320x860,1900x1060,1100x720",
    langs: "en,ko,both",
    tabs: "affix,runeword,advanced",
    out: "",
    compare: "",
    shots: false
  };
  for (let i = 0; i < argv.length; i += 1) {
    const key = argv[i].replace(/^--/, "");
    if (key === "shots") {
      args.shots = true;
    } else if (key in args) {
      args[key] = argv[i + 1];
      i += 1;
    } else {
      throw new Error(`unknown option ${argv[i]}`);
    }
  }
  return args;
}

function loadPlaywright() {
  try {
    return require("playwright");
  } catch (_) {}
  const npxRoot = path.join(os.homedir(), ".npm", "_npx");
  const candidates = fs.existsSync(npxRoot)
    ? fs.readdirSync(npxRoot)
        .map((dir) => path.join(npxRoot, dir, "node_modules", "playwright"))
        .filter((dir) => fs.existsSync(dir))
    : [];
  for (const candidate of candidates) {
    try {
      return require(candidate);
    } catch (_) {}
  }
  throw new Error("playwright not found (npm i -g playwright, or run any `npx playwright` once)");
}

function findChromium() {
  const root = path.join(os.homedir(), ".cache", "ms-playwright");
  if (!fs.existsSync(root)) return undefined;
  const dirs = fs.readdirSync(root).filter((d) => /^chromium-\d+$/.test(d)).sort().reverse();
  for (const dir of dirs) {
    const exe = path.join(root, dir, "chrome-linux64", "chrome");
    if (fs.existsSync(exe)) return exe;
  }
  return undefined;
}

function resolveView(view, ref, outDir) {
  if (!ref) return path.resolve(view);
  const target = path.join(outDir, `ref-${ref.replace(/[^\w.-]/g, "_")}`);
  fs.rmSync(target, { recursive: true, force: true });
  fs.mkdirSync(target, { recursive: true });
  const tar = execFileSync("git", ["archive", ref, view], { maxBuffer: 1 << 28 });
  execFileSync("tar", ["-x", "-C", target], { input: tar });
  return path.join(target, view);
}

// python3 -m http.server sends no cache headers and Chromium heuristically
// caches the CSS, so edits silently go unmeasured. Always no-store.
function serve(root) {
  const types = { ".html": "text/html", ".js": "text/javascript", ".css": "text/css",
    ".png": "image/png", ".svg": "image/svg+xml", ".dds": "application/octet-stream" };
  const server = http.createServer((req, res) => {
    const rel = decodeURIComponent(new URL(req.url, "http://x").pathname).replace(/^\/+/, "");
    const file = path.join(root, rel || "index.html");
    if (!file.startsWith(root) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) {
      res.writeHead(404);
      res.end();
      return;
    }
    res.writeHead(200, {
      "Content-Type": types[path.extname(file)] || "application/octet-stream",
      "Cache-Control": "no-store"
    });
    fs.createReadStream(file).pipe(res);
  });
  return new Promise((resolve) => server.listen(0, "127.0.0.1", () => resolve(server)));
}

function buildFixtures(repoRoot) {
  const affixes = JSON.parse(fs.readFileSync(path.join(repoRoot, "affixes", "affixes.json"), "utf8"))
    .keywords.affixes;
  const contract = JSON.parse(fs.readFileSync(path.join(repoRoot, "affixes", "runeword.contract.json"), "utf8"));
  const byId = new Map(affixes.map((a) => [a.id, a]));
  const runeNames = contract.runewordRuneWeights.map((r) => r.rune);
  const runeToken = (name) => String(1000 + Math.max(0, runeNames.indexOf(name)));
  const afterColon = (s) => String(s || "").replace(/^[^:]*:\s*/, "");

  const recipes = contract.runewordCatalog.map((rw, index) => {
    const affix = byId.get(rw.resultAffixId) || {};
    const nameKo = (/^룬워드 (.+?) \[/.exec(affix.nameKo || "") || [])[1] || rw.name;
    const summaryEn = afterColon(affix.nameEn);
    const summaryKo = afterColon(affix.nameKo);
    return {
      token: String(5000 + index),
      name: nameKo,
      nameEn: rw.name,
      nameKo,
      runes: rw.runes.join("-"),
      runeTokens: rw.runes.map(runeToken),
      summaryKey: "",
      summaryEn,
      summaryKo,
      detailEn: `Runeword ${rw.name} [${rw.runes.join("-")}]: ${summaryEn}\nInternal cooldown applies.`,
      detailKo: `룬워드 ${nameKo} [${rw.runes.join("-")}]: ${summaryKo}\n내부 재사용 대기시간이 있습니다.`,
      summary: summaryKo,
      detail: summaryKo,
      baseKey: String(rw.recommendedBase || "").toLowerCase(),
      selected: false
    };
  });
  // An armor recipe with 3+ runes on a weapon base: exercises the mismatch
  // warning and a multi-rune progress row with one honest set of messages.
  const selectedRecipe = recipes.find((r) => r.baseKey === "armor" && r.runeTokens.length >= 3) || recipes[0];
  selectedRecipe.selected = true;

  const baseNames = [
    ["Ancient Nord War Axe of the Blizzard", "고대 노르드 전투 도끼"],
    ["Daedric Greatsword", "데이드릭 대검"],
    ["Ebony Mail", "흑단 갑옷"],
    ["Dragonplate Gauntlets of Peerless Archery", "용판금 건틀릿"],
    ["Glass Bow of Exhaustion", "유리 활"],
    ["Stalhrim Heavy Helmet", "스탈림 중갑 투구"],
    ["Elven Shield", "엘프 방패"],
    ["Nightingale Boots", "나이팅게일 장화"]
  ];
  const inventory = baseNames.map(([en, ko], i) => ({
    key: String(9000 + i),
    name: `${en} / ${ko}`,
    baseName: `${en} / ${ko}`,
    runewordNameEn: i === 2 ? "Enigma" : "",
    runewordNameKo: i === 2 ? "수수께끼" : "",
    selected: i === 0
  }));

  const groupOf = (a) => a.slot === "suffix" ? "passive"
    : a.runtime?.trigger === "IncomingHit" || a.runtime?.trigger === "LowHealth" ? "defense"
    : a.runtime?.trigger === "Kill" ? "kill" : "offense";
  const buildPicks = [
    ...affixes.filter((a) => a.slot === "prefix" && !a.id.startsWith("runeword_")).slice(0, 7),
    ...affixes.filter((a) => a.slot === "suffix").filter((_, i) => i % 9 === 0).slice(0, 7)
  ];
  const equippedBuild = {
    ready: true,
    runtimeEnabled: true,
    equippedAffixSlots: 6,
    entries: buildPicks.map((a, i) => ({
      token: String(7000 + i),
      displayNameEn: a.nameEn,
      displayNameKo: a.nameKo,
      group: groupOf(a),
      triggerKey: String(a.runtime?.trigger || "").toLowerCase(),
      slotKind: a.slot,
      suffixState: a.slot === "suffix" ? (i % 4 === 0 ? "suppressed" : "active") : "",
      equippedCount: 1 + (i % 2),
      suffixTierRank: 1 + (i % 3),
      suffixFamilyRankPoints: 1 + (i % 3),
      effectiveSuffixTierRank: 1 + (i % 3),
      effectiveSuffixDisplayNameEn: a.slot === "suffix" ? a.nameEn : "",
      effectiveSuffixDisplayNameKo: a.slot === "suffix" ? a.nameKo : "",
      hasPassiveContribution: a.slot === "suffix",
      passiveContributionActive: a.slot === "suffix",
      passiveSpellDisabled: false,
      hasProcRoll: a.slot === "prefix",
      procRollChancePct: a.runtime?.procChancePercent || 0,
      procRollStackCount: 1,
      castOnCritSelectionLimited: false,
      hasNormalWeaponHitProcRoll: i % 3 === 0,
      normalWeaponHitProcChancePct: 35,
      normalWeaponHitProcStackCount: 1,
      hasLuckyHitGate: i % 5 === 0,
      luckyHitGateChancePct: 30
    }))
  };

  const runeInventory = runeNames.map((name, i) => ({ runeToken: runeToken(name), runeName: name, owned: (i * 7) % 5 }));
  const ownedOf = (name) => runeInventory.find((r) => r.runeName === name)?.owned || 0;
  const selectedRunes = selectedRecipe.runes.split("-");
  const panelState = {
    hasBase: true,
    hasRecipe: true,
    isComplete: false,
    recipeName: selectedRecipe.nameKo,
    recipeNameEn: selectedRecipe.nameEn,
    recipeNameKo: selectedRecipe.nameKo,
    recipeToken: selectedRecipe.token,
    insertedRunes: 1,
    totalRunes: selectedRunes.length,
    nextRuneName: selectedRunes[1] || selectedRunes[0],
    nextRuneOwned: ownedOf(selectedRunes[1] || selectedRunes[0]),
    canInsert: true,
    missingSummary: "",
    baseCompatibilityWarning: true,
    // Same wording as EventBridge.Loot.Runeword.PanelState.cpp.
    baseCompatibilityMessageEn: "Base mismatch: recommended base is Armor. The selected base is incompatible; transmute remains allowed.",
    baseCompatibilityMessageKo: "베이스 불일치: 이 룬워드는 갑옷에 맞춰져 있습니다. 현재 베이스와 호환되지 않지만 변환은 계속할 수 있습니다.",
    requiredRunes: [...new Set(selectedRunes)].map((name) => ({
      name, required: selectedRunes.filter((r) => r === name).length, owned: ownedOf(name)
    })),
    runeInventoryKnown: true,
    runeInventoryExpectedCount: runeInventory.length,
    runeInventory,
    // A runeword holding the head slot plus two suffixes: shows Remove Runeword.
    regularAffixCount: 2,
    maxRegularAffixCount: 3,
    affixHead: "runeword",
    affixSlotCount: 3,
    canRemoveRuneword: true,
    transmuteRemovesPrefixEn: "",
    transmuteRemovesPrefixKo: "",
    expandAffixCost: 3,
    canExpandAffix: true,
    expandAffixUnavailableReason: "",
    reforgeOrbsKnown: true,
    reforgeOrbsOwned: 128,
    identifyScrollsKnown: true,
    identifyScrollsOwned: 42,
    scouringOrbsKnown: true,
    scouringOrbsOwned: 17,
    exchangeReforgeScrollCost: 10,
    exchangeScourScrollCost: 15,
    pityKnown: true,
    runewordFragmentFailStreak: 37,
    runewordFragmentFailStreakThreshold: 60,
    reforgeOrbFailStreak: 12,
    reforgeOrbFailStreakThreshold: 40,
    standardReforgeCost: 1,
    lockedReforgeCost: 2,
    selectedReforgesLeft: 4,
    selectedReforgesPerItem: 6,
    reforgeLockCandidates: buildPicks.filter((a) => a.slot === "suffix").slice(0, 2).map((a, i) => ({
      affixToken: String(7000 + i), displayNameEn: a.nameEn, displayNameKo: a.nameKo, slotKind: a.slot
    })),
    equippedBuild,
    debugTools: false
  };
  return { recipes, inventory, panelState, preview: selectedRecipe.detailKo };
}

// Runs inside the page. Keep it self-contained.
function auditInPage() {
  const panel = document.getElementById("controlPanel");
  const findings = [];
  const label = (el) => {
    const id = el.id ? `#${el.id}` : "";
    const cls = typeof el.className === "string" && el.className.trim()
      ? `.${el.className.trim().split(/\s+/).slice(0, 2).join(".")}` : "";
    const text = (el.textContent || "").trim().replace(/\s+/g, " ").slice(0, 40);
    return `${el.tagName.toLowerCase()}${id}${cls}${text ? ` "${text}"` : ""}`;
  };
  const isRendered = (el) => {
    if (el.closest("[hidden]")) return false;
    const closedDetails = el.closest("details:not([open])");
    if (closedDetails && !el.closest("summary")) return false;
    const rect = el.getBoundingClientRect();
    if (rect.width < 2 || rect.height < 2) return false;
    for (let node = el; node && node !== document.body; node = node.parentElement) {
      const s = getComputedStyle(node);
      if (s.display === "none" || s.visibility === "hidden") return false;
    }
    const s = getComputedStyle(el);
    // sr-only pattern: absolute + clip
    if (s.position === "absolute" && (s.clip !== "auto" || s.clipPath !== "none")) return false;
    return true;
  };
  const scrolls = (s, axis) => /(auto|scroll)/.test(axis === "y" ? s.overflowY : s.overflowX);
  const clips = (s, axis) => /(hidden|clip)/.test(axis === "y" ? s.overflowY : s.overflowX);

  const all = [...panel.querySelectorAll("*")].filter(isRendered);
  const scrollers = new Set();
  for (const el of all) {
    const s = getComputedStyle(el);
    const overY = el.scrollHeight > el.clientHeight + 1;
    const overX = el.scrollWidth > el.clientWidth + 1;
    if ((overY && clips(s, "y")) || (overX && clips(s, "x"))) {
      const byDesign = s.textOverflow === "ellipsis" || (s.webkitLineClamp && s.webkitLineClamp !== "none");
      const kind = byDesign ? "TRUNCATED" : "CLIPPED";
      findings.push({ kind, el: label(el), detail: `${el.scrollWidth}x${el.scrollHeight} in ${el.clientWidth}x${el.clientHeight}` });
    }
    if ((overY && scrolls(s, "y")) || (overX && scrolls(s, "x"))) {
      scrollers.add(el);
      if (overY && el.offsetWidth - el.clientWidth - parseFloat(s.borderLeftWidth) - parseFloat(s.borderRightWidth) < 1) {
        findings.push({ kind: "NO-BAR", el: label(el), detail: "scrolls but scrollbar has no width" });
      }
    }
  }
  for (const el of scrollers) {
    for (let up = el.parentElement; up && up !== panel.parentElement; up = up.parentElement) {
      if (scrollers.has(up)) {
        findings.push({ kind: "NESTED-SCROLL", el: label(el), detail: `inside ${label(up)}` });
        break;
      }
    }
  }
  const targets = all.filter((el) => el.matches("button, input, select, [role=tab], [role=option], label") ||
    (el.children.length === 0 && (el.textContent || "").trim().length > 0));
  for (const el of targets) {
    el.scrollIntoView({ block: "nearest", inline: "nearest" });
    const r = el.getBoundingClientRect();
    let top = r.top, bottom = r.bottom, left = r.left, right = r.right;
    for (let up = el.parentElement; up; up = up.parentElement) {
      const s = getComputedStyle(up);
      if (up === panel || scrolls(s, "y") || clips(s, "y") || scrolls(s, "x") || clips(s, "x")) {
        const c = up.getBoundingClientRect();
        top = Math.max(top, c.top); bottom = Math.min(bottom, c.bottom);
        left = Math.max(left, c.left); right = Math.min(right, c.right);
      }
      if (up === panel) break;
    }
    const area = r.width * r.height;
    const seen = Math.max(0, bottom - top) * Math.max(0, right - left);
    if (area > 0 && seen / area < 0.9) {
      findings.push({ kind: "UNREACHABLE", el: label(el), detail: `${Math.round((100 * seen) / area)}% visible` });
    }
  }
  for (const el of scrollers) el.scrollTop = 0;
  // An element reported as both CLIPPED and UNREACHABLE is one problem; keep both,
  // they read differently. Deduplicate exact repeats only.
  const seenKeys = new Set();
  return findings.filter((f) => {
    const key = `${f.kind}|${f.el}|${f.detail}`;
    if (seenKeys.has(key)) return false;
    seenKeys.add(key);
    return true;
  });
}

async function runAudit(browser, viewDir, fixtures, args, shotDir) {
  const server = await serve(viewDir);
  const port = server.address().port;
  const tabButton = { affix: "mainAffixTab", runeword: "mainRunewordTab", advanced: "mainAdvancedTab" };
  const report = [];
  try {
    for (const size of args.sizes.split(",")) {
      const [width, height] = size.split("x").map(Number);
      for (const lang of args.langs.split(",")) {
        const page = await browser.newPage({ viewport: { width: Math.max(1920, width + 80), height: Math.max(1080, height + 80) } });
        const errors = [];
        page.on("pageerror", (e) => errors.push(String(e)));
        await page.goto(`http://127.0.0.1:${port}/index.html?lang=${lang}`);
        await page.evaluate(({ fx, lang, width, height }) => {
          setUiLanguage(lang);
          setControlPanel("open");
          setPanelLayout(JSON.stringify({ left: 40, top: 40, width, height }));
          setInventoryItems(JSON.stringify(fx.inventory));
          setRecipeItems(JSON.stringify(fx.recipes));
          setRunewordPanelState(JSON.stringify(fx.panelState));
          setRunewordAffixPreview(fx.preview);
        }, { fx: fixtures, lang, width, height });
        await page.waitForTimeout(300);
        for (const tab of args.tabs.split(",")) {
          await page.click(`#${tabButton[tab]}`);
          await page.waitForTimeout(200);
          const findings = await page.evaluate(auditInPage);
          if (shotDir) {
            fs.mkdirSync(shotDir, { recursive: true });
            await page.locator("#controlPanel").screenshot({ path: path.join(shotDir, `${size}-${lang}-${tab}.png`) });
          }
          report.push({ size, lang, tab, findings, errors: [...errors] });
        }
        await page.close();
      }
    }
  } finally {
    server.close();
  }
  return report;
}

// Text is left out of the key so a copy change does not read as a new finding.
const findingKey = (r, f) => `${r.size}|${r.lang}|${r.tab}|${f.kind}|${f.el.replace(/ ".*$/, "")}`;

function printReport(report, title) {
  console.log(`== ${title}`);
  let failing = 0;
  for (const r of report) {
    const counts = {};
    for (const f of r.findings) counts[f.kind] = (counts[f.kind] || 0) + 1;
    failing += r.findings.filter((f) => f.kind !== "TRUNCATED").length + r.errors.length;
    const summary = Object.entries(counts).map(([k, v]) => `${k}=${v}`).join(" ") || "clean";
    console.log(`${r.size.padEnd(10)} ${r.lang.padEnd(5)} ${r.tab.padEnd(9)} ${summary}${r.errors.length ? ` JS-ERRORS=${r.errors.length}` : ""}`);
    const shown = r.findings.filter((f) => f.kind !== "TRUNCATED");
    for (const f of shown.slice(0, 6)) console.log(`    ${f.kind.padEnd(13)} ${f.el}  (${f.detail})`);
    if (shown.length > 6) console.log(`    ... ${shown.length - 6} more in report json`);
    for (const e of r.errors.slice(0, 3)) console.log(`    JS-ERROR ${e}`);
  }
  return failing;
}

async function main() {
  const args = parseArgs(process.argv.slice(2));
  const repoRoot = execFileSync("git", ["rev-parse", "--show-toplevel"]).toString().trim();
  const outDir = path.resolve(args.out || path.join(os.tmpdir(), "calamity-panel-audit"));
  fs.mkdirSync(outDir, { recursive: true });
  const fixtures = buildFixtures(repoRoot);
  const { chromium } = loadPlaywright();
  const browser = await chromium.launch({
    executablePath: findChromium(),
    // Playwright hides scrollbars by default, which makes every NO-BAR a false positive.
    ignoreDefaultArgs: ["--hide-scrollbars"]
  });
  try {
    const viewDir = resolveView(args.view, args.ref, outDir);
    const report = await runAudit(browser, viewDir, fixtures, args, args.shots ? path.join(outDir, "shots") : "");
    fs.writeFileSync(path.join(outDir, "report.json"), JSON.stringify(report, null, 2));

    if (!args.compare) {
      const failing = printReport(report, `${args.ref || "work tree"} (${viewDir})`);
      console.log(`\nreport: ${path.join(outDir, "report.json")}${args.shots ? `\nscreenshots: ${path.join(outDir, "shots")}` : ""}`);
      console.log("TRUNCATED is by design and only counted, not listed; see report.json.");
      process.exitCode = failing > 0 ? 1 : 0;
      return;
    }

    const baseDir = resolveView(args.view, args.compare, outDir);
    const base = await runAudit(browser, baseDir, fixtures, args, args.shots ? path.join(outDir, `shots-${args.compare}`) : "");
    fs.writeFileSync(path.join(outDir, `report-${args.compare}.json`), JSON.stringify(base, null, 2));
    const baseCounts = new Map();
    for (const r of base) for (const f of r.findings) baseCounts.set(findingKey(r, f), (baseCounts.get(findingKey(r, f)) || 0) + 1);
    const added = [];
    const remaining = new Map(baseCounts);
    for (const r of report) {
      for (const f of r.findings) {
        const key = findingKey(r, f);
        const left = remaining.get(key) || 0;
        if (left > 0) remaining.set(key, left - 1);
        else added.push({ combo: `${r.size} ${r.lang} ${r.tab}`, ...f });
      }
    }
    const fixed = [...remaining.entries()].filter(([, n]) => n > 0);
    console.log(`== work tree vs ${args.compare}`);
    console.log(`new findings: ${added.length}  (TRUNCATED among them: ${added.filter((f) => f.kind === "TRUNCATED").length})`);
    for (const f of added.slice(0, 30)) console.log(`  + ${f.combo.padEnd(24)} ${f.kind.padEnd(13)} ${f.el}  (${f.detail})`);
    if (added.length > 30) console.log(`  ... ${added.length - 30} more`);
    console.log(`gone since ${args.compare}: ${fixed.reduce((n, [, c]) => n + c, 0)}`);
    for (const [key, n] of fixed.slice(0, 15)) console.log(`  - ${key}${n > 1 ? ` x${n}` : ""}`);
    const jsErrors = report.reduce((n, r) => n + r.errors.length, 0);
    if (jsErrors) console.log(`JS errors in work tree: ${jsErrors} (see report.json)`);
    console.log(`\nreports: ${outDir}`);
    process.exitCode = added.some((f) => f.kind !== "TRUNCATED") || jsErrors ? 1 : 0;
  } finally {
    await browser.close();
  }
}

main().catch((e) => {
  console.error(e);
  process.exitCode = 2;
});
