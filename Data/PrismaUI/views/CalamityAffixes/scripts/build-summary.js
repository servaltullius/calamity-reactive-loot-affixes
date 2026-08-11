const equippedBuildGroupOrder = Object.freeze(["offense", "defense", "kill", "passive"]);
const equippedBuildGroupsById = Object.freeze({
  offense: {
    title: equippedBuildOffenseTitle,
    count: equippedBuildOffenseCount,
    list: equippedBuildOffenseList
  },
  defense: {
    title: equippedBuildDefenseTitle,
    count: equippedBuildDefenseCount,
    list: equippedBuildDefenseList
  },
  kill: {
    title: equippedBuildKillTitle,
    count: equippedBuildKillCount,
    list: equippedBuildKillList
  },
  passive: {
    title: equippedBuildPassiveTitle,
    count: equippedBuildPassiveCount,
    list: equippedBuildPassiveList
  }
});

function normalizeEquippedBuildChance(raw) {
  const value = Number(raw);
  if (!Number.isFinite(value)) {
    return 0;
  }
  return Math.max(0, Math.min(100, value));
}

function normalizeEquippedBuildSuffixState(raw, slotKind) {
  if (slotKind !== "suffix") {
    return "none";
  }

  const value = typeof raw === "string" ? raw.trim().toLowerCase() : "";
  if (value === "highest" || value === "winner" || value === "best") {
    return "highest";
  }
  if (value === "suppressed" || value === "inactive") {
    return "suppressed";
  }
  if (value === "stacking" || value === "additive") {
    return "stacking";
  }
  return "none";
}

function normalizeEquippedBuildEntries(raw) {
  if (!Array.isArray(raw)) {
    return [];
  }

  const entries = [];
  const seenTokens = new Set();
  for (const candidate of raw) {
    const token = typeof candidate?.token === "string" ? candidate.token.trim() : "";
    const group = typeof candidate?.group === "string" ? candidate.group.trim().toLowerCase() : "";
    const slotKind = typeof candidate?.slotKind === "string"
      ? candidate.slotKind.trim().toLowerCase()
      : "";
    const equippedCount = parseNonNegativeInteger(candidate?.equippedCount, 0);
    if (
      !/^[0-9]+$/.test(token) ||
      token === "0" ||
      seenTokens.has(token) ||
      !equippedBuildGroupOrder.includes(group) ||
      !["prefix", "suffix", "runeword"].includes(slotKind) ||
      equippedCount <= 0
    ) {
      continue;
    }

    seenTokens.add(token);
    entries.push({
      token,
      displayNameEn: typeof candidate.displayNameEn === "string"
        ? candidate.displayNameEn.trim()
        : "",
      displayNameKo: typeof candidate.displayNameKo === "string"
        ? candidate.displayNameKo.trim()
        : "",
      group,
      triggerKey: typeof candidate.triggerKey === "string"
        ? candidate.triggerKey.trim().toLowerCase()
        : "",
      slotKind,
      suffixState: normalizeEquippedBuildSuffixState(candidate.suffixState, slotKind),
      equippedCount,
      hasPassiveContribution: Boolean(candidate.hasPassiveContribution),
      passiveContributionActive: Boolean(candidate.passiveContributionActive),
      passiveSpellDisabled: Boolean(candidate.passiveSpellDisabled),
      hasProcRoll: Boolean(candidate.hasProcRoll),
      procRollChancePct: normalizeEquippedBuildChance(candidate.procRollChancePct),
      hasLuckyHitGate: Boolean(candidate.hasLuckyHitGate),
      luckyHitGateChancePct: normalizeEquippedBuildChance(candidate.luckyHitGateChancePct)
    });
  }
  return entries;
}

function normalizeEquippedBuildState(raw) {
  const received = Boolean(raw && typeof raw === "object" && !Array.isArray(raw));
  const data = received ? raw : {};
  return {
    received,
    ready: Boolean(data.ready),
    runtimeEnabled: Boolean(data.runtimeEnabled),
    equippedAffixSlots: parseNonNegativeInteger(data.equippedAffixSlots, 0),
    entries: normalizeEquippedBuildEntries(data.entries)
  };
}

function resolveEquippedBuildViewState(state = equippedBuildState) {
  if (!state || !state.received) {
    return "syncing";
  }
  if (!state.ready) {
    return "syncing";
  }
  if (!state.runtimeEnabled) {
    return "runtime-disabled";
  }
  return state.entries.length > 0 ? "ready" : "empty";
}

function formatEquippedBuildChance(value) {
  const rounded = Math.round(normalizeEquippedBuildChance(value) * 10) / 10;
  return Number.isInteger(rounded) ? String(rounded) : rounded.toFixed(1);
}

function resolveEquippedBuildName(entry) {
  const en = entry.displayNameEn || entry.displayNameKo || "Unknown affix";
  const ko = entry.displayNameKo || entry.displayNameEn || "알 수 없는 어픽스";
  return t(en, ko);
}

function resolveEquippedBuildSlotBadge(entry) {
  switch (entry.slotKind) {
    case "prefix":
      return { text: "P", label: t("Prefix", "접두") };
    case "suffix":
      return { text: "S", label: t("Suffix", "접미") };
    case "runeword":
      return { text: "RW", label: t("Runeword", "룬워드") };
    default:
      return null;
  }
}

function resolveEquippedBuildSuffixBadge(entry) {
  switch (entry.suffixState) {
    case "highest":
      return {
        text: t("Highest tier selected", "최고 티어 선택"),
        className: "highest"
      };
    case "suppressed":
      return {
        text: t("Suppressed by higher tier", "상위 티어로 비활성"),
        className: "suppressed"
      };
    case "stacking":
      return {
        text: t("Independent suffix", "독립 접미"),
        className: "stacking"
      };
    default:
      return null;
  }
}

function resolveEquippedBuildPassiveFacetBadge(entry, facetGroupId) {
  if (facetGroupId !== "passive" || !entry.hasPassiveContribution) {
    return null;
  }

  if (entry.suffixState === "suppressed") {
    return entry.passiveContributionActive
      ? {
          text: t(
            "Other passive contribution active",
            "다른 패시브 기여 적용"
          ),
          className: "passive-active"
        }
      : null;
  }

  if (entry.passiveSpellDisabled) {
    return entry.passiveContributionActive
      ? {
          text: t(
            "Stat passive active · spell off",
            "능력치 패시브 적용 · 주문 꺼짐"
          ),
          className: "passive-partial"
        }
      : {
          text: t(
            "Passive spell disabled by runtime setting",
            "런타임 설정으로 패시브 주문 꺼짐"
          ),
          className: "passive-disabled"
        };
  }

  return entry.passiveContributionActive
    ? {
        text: t("Passive active", "패시브 적용"),
        className: "passive-active"
      }
    : null;
}

function resolveEquippedBuildTriggerBadge(entry) {
  switch (entry.triggerKey) {
    case "incominghit":
      return t("On hit taken", "피격 시");
    case "dotapply":
      return t("On DoT apply", "지속 피해 적용 시");
    case "kill":
      return t("On kill", "처치 시");
    case "lowhealth":
      return t("At low health", "낮은 생명력에서");
    case "passive":
      return t("Passive", "패시브");
    case "hit":
      return t("On hit", "타격 시");
    default:
      return "";
  }
}

function appendEquippedBuildBadge(parent, text, className = "", ariaLabel = "") {
  const badge = document.createElement("span");
  badge.className = `ebBadge${className ? ` ${className}` : ""}`;
  badge.textContent = text;
  if (ariaLabel) {
    badge.setAttribute("aria-label", ariaLabel);
    badge.title = ariaLabel;
  }
  parent.appendChild(badge);
  return badge;
}

function createEquippedBuildEntry(entry, facetGroupId) {
  const item = document.createElement("li");
  if (entry.suffixState === "suppressed") {
    item.className = entry.passiveContributionActive
      ? "ebEntry suppressed-partial"
      : "ebEntry suppressed";
  } else {
    item.className = "ebEntry";
  }

  const top = document.createElement("div");
  top.className = "ebEntryTop";

  const name = document.createElement("div");
  name.className = "ebEntryName";
  name.textContent = resolveEquippedBuildName(entry);
  top.appendChild(name);

  const identityBadges = document.createElement("div");
  identityBadges.className = "ebBadgeRow";
  appendEquippedBuildBadge(
    identityBadges,
    `×${entry.equippedCount}`,
    "count",
    t(`Equipped count ${entry.equippedCount}`, `장착 수 ${entry.equippedCount}`)
  );
  const slotBadge = resolveEquippedBuildSlotBadge(entry);
  if (slotBadge) {
    appendEquippedBuildBadge(identityBadges, slotBadge.text, "slot", slotBadge.label);
  }
  top.appendChild(identityBadges);
  item.appendChild(top);

  const effectBadges = document.createElement("div");
  effectBadges.className = "ebBadgeRow ebEffectBadges";

  const triggerBadge = resolveEquippedBuildTriggerBadge(entry);
  if (triggerBadge) {
    appendEquippedBuildBadge(effectBadges, triggerBadge, "trigger");
  }
  const suffixBadge = resolveEquippedBuildSuffixBadge(entry);
  if (suffixBadge) {
    appendEquippedBuildBadge(effectBadges, suffixBadge.text, suffixBadge.className);
  }
  if (
    facetGroupId !== "passive" &&
    entry.group !== "passive" &&
    entry.hasPassiveContribution &&
    entry.passiveContributionActive
  ) {
    appendEquippedBuildBadge(
      effectBadges,
      t("Passive also active", "패시브도 적용"),
      "hybrid"
    );
  }
  const passiveFacetBadge = resolveEquippedBuildPassiveFacetBadge(entry, facetGroupId);
  if (passiveFacetBadge) {
    appendEquippedBuildBadge(
      effectBadges,
      passiveFacetBadge.text,
      passiveFacetBadge.className
    );
  }
  if (entry.hasProcRoll) {
    const chance = formatEquippedBuildChance(entry.procRollChancePct);
    appendEquippedBuildBadge(
      effectBadges,
      t(`Conditional proc roll ${chance}%`, `조건부 발동 굴림 ${chance}%`),
      "proc"
    );
  }
  if (entry.hasProcRoll && entry.equippedCount > 1) {
    appendEquippedBuildBadge(
      effectBadges,
      t("Shared single roll", "중복 1회 판정"),
      "shared-roll"
    );
  }
  if (entry.hasLuckyHitGate) {
    const chance = formatEquippedBuildChance(entry.luckyHitGateChancePct);
    appendEquippedBuildBadge(
      effectBadges,
      t(`Lucky Hit gate ${chance}%`, `행운 적중 관문 ${chance}%`),
      "lucky"
    );
  }
  if (!entry.hasProcRoll && !entry.hasLuckyHitGate && entry.group !== "passive") {
    appendEquippedBuildBadge(effectBadges, t("No random proc roll", "무작위 발동 굴림 없음"), "conditional");
  }
  item.appendChild(effectBadges);

  return item;
}

function renderEquippedBuildGroup(groupId, entries) {
  const elements = equippedBuildGroupsById[groupId];
  if (!elements || !elements.list || !elements.count) {
    return;
  }

  clearChildren(elements.list);
  const slotCount = entries.reduce((sum, entry) => sum + entry.equippedCount, 0);
  const countLabel = t(
    `${slotCount} effect copies shown in this group`,
    `이 그룹에 표시된 효과 복사본 ${slotCount}개`
  );
  elements.count.textContent = String(slotCount);
  elements.count.setAttribute("aria-label", countLabel);
  elements.count.title = countLabel;

  if (entries.length === 0) {
    const empty = document.createElement("li");
    empty.className = "ebGroupEmpty";
    empty.textContent = t("None", "없음");
    elements.list.appendChild(empty);
    return;
  }

  for (const entry of entries) {
    elements.list.appendChild(createEquippedBuildEntry(entry, groupId));
  }
}

function renderEquippedBuildStateMessage(viewState) {
  if (!equippedBuildStatus) {
    return;
  }
  clearChildren(equippedBuildStatus);
  equippedBuildStatus.className = "ebStatus";

  if (viewState === "runtime-disabled") {
    appendEmptyState(
      equippedBuildStatus,
      t("Calamity effects are disabled", "칼래미티 효과가 비활성화되었습니다"),
      t(
        "Enable runtime effects in MCM to rebuild the equipped-effect summary.",
        "MCM에서 런타임 효과를 활성화하면 장착 효과 요약을 다시 구성합니다."
      )
    );
    return;
  }
  if (viewState === "empty") {
    appendEmptyState(
      equippedBuildStatus,
      t("No Calamity affixes equipped", "장착된 칼래미티 어픽스가 없습니다"),
      t(
        "Equip an item with an affix or runeword to see its build role here.",
        "어픽스나 룬워드가 있는 아이템을 착용하면 여기에서 빌드 역할을 확인할 수 있습니다."
      )
    );
    return;
  }
  if (viewState === "syncing") {
    appendEmptyState(
      equippedBuildStatus,
      t("Synchronizing equipped effects…", "장착 효과 동기화 중…"),
      t(
        "Waiting for the game's equipped-affix cache.",
        "게임의 장착 어픽스 캐시를 기다리고 있습니다."
      )
    );
    return;
  }

  equippedBuildStatus.classList.add("ready");
  equippedBuildStatus.textContent = t(
    `${equippedBuildState.equippedAffixSlots} equipped affix slots synchronized.`,
    `장착 어픽스 슬롯 ${equippedBuildState.equippedAffixSlots}개가 동기화되었습니다.`
  );
}

function updateEquippedBuildStaticText() {
  if (equippedBuildTitle) {
    equippedBuildTitle.textContent = t("Equipped Build", "장착 빌드");
  }
  if (equippedBuildLead) {
    equippedBuildLead.textContent = t(
      "Read-only summary of Calamity effects on equipped items.",
      "착용 아이템의 칼래미티 효과를 읽기 전용으로 요약합니다."
    );
  }
  if (equippedBuildChanceHint) {
    equippedBuildChanceHint.textContent = t(
      "Shown chance is a condition-qualified roll after current modifiers. ICDs, proc budgets, action preconditions, and Lucky Hit still apply separately. ×N is equipped-copy count, not a guaranteed stack multiplier; duplicate proc entries share one roll, and tiered suffix families apply only the highest tier. Hybrid effects may appear in both their trigger group and Passives without increasing the equipped-slot total.",
      "표시 확률은 현재 보정 적용 후 조건부 굴림입니다. ICD, 발동 예산, 행동 선행 조건, 행운 적중은 별도로 적용됩니다. ×N은 장착 개수이며 보장된 중첩 배수가 아닙니다. 중복 발동 항목은 한 번만 판정하고, 단계형 접미 계열은 최고 티어만 적용됩니다. 하이브리드 효과는 장착 슬롯 총계를 늘리지 않고 발동 그룹과 패시브 그룹에 함께 표시될 수 있습니다."
    );
  }

  const groupLabels = {
    offense: t("Attack Procs", "공격 발동"),
    defense: t("Hit Taken & Survival", "피격·생존"),
    kill: t("Kill Effects", "처치 효과"),
    passive: t("Passives", "패시브")
  };
  for (const groupId of equippedBuildGroupOrder) {
    const title = equippedBuildGroupsById[groupId]?.title;
    if (title) {
      title.textContent = groupLabels[groupId];
    }
  }
}

function renderEquippedBuildSummary() {
  if (!equippedBuildStatus || !equippedBuildGroups) {
    return;
  }

  const viewState = resolveEquippedBuildViewState();
  const ready = viewState === "ready";
  equippedBuildGroups.hidden = !ready;
  equippedBuildGroups.setAttribute("aria-busy", viewState === "syncing" ? "true" : "false");
  if (equippedBuildChanceHint) {
    equippedBuildChanceHint.hidden = !ready;
  }

  if (equippedBuildSlotCount) {
    equippedBuildSlotCount.textContent = ready
      ? t(
          `${equippedBuildState.equippedAffixSlots} slots`,
          `슬롯 ${equippedBuildState.equippedAffixSlots}개`
        )
      : "—";
  }

  renderEquippedBuildStateMessage(viewState);
  if (!ready) {
    return;
  }

  for (const groupId of equippedBuildGroupOrder) {
    const groupEntries = equippedBuildState.entries.filter(
      (entry) =>
        entry.group === groupId ||
        (
          groupId === "passive" &&
          entry.group !== "passive" &&
          entry.hasPassiveContribution
        )
    );
    renderEquippedBuildGroup(
      groupId,
      groupEntries
    );
  }
}
