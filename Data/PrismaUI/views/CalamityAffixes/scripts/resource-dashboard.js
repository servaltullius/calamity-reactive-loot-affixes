function normalizeResourceDashboardSnapshot(data) {
  let runeInventoryKnown = false;
  let runeInventoryExpectedCount = 0;
  let runes = [];

  const expectedCount = data?.runeInventoryExpectedCount;
  if (
    data?.runeInventoryKnown === true &&
    Array.isArray(data?.runeInventory) &&
    typeof expectedCount === "number" &&
    Number.isSafeInteger(expectedCount) &&
    expectedCount > 0 &&
    data.runeInventory.length === expectedCount
  ) {
    const seenTokens = new Set();
    const normalizedRunes = [];
    let valid = true;
    let totalOwned = 0;

    for (const entry of data.runeInventory) {
      const runeToken = normalizePositiveUint64DecimalString(entry?.runeToken);
      const runeName = typeof entry?.runeName === "string" ? entry.runeName.trim() : "";
      const owned = entry?.owned;
      if (
        !runeToken ||
        !runeName ||
        typeof owned !== "number" ||
        !Number.isSafeInteger(owned) ||
        owned < 0 ||
        seenTokens.has(runeToken) ||
        !Number.isSafeInteger(totalOwned + owned)
      ) {
        valid = false;
        break;
      }

      seenTokens.add(runeToken);
      totalOwned += owned;
      normalizedRunes.push({ runeToken, runeName, owned });
    }

    if (valid && normalizedRunes.length === data.runeInventory.length) {
      runeInventoryKnown = true;
      runeInventoryExpectedCount = expectedCount;
      runes = normalizedRunes;
    }
  }

  const reforgeOrbsOwned = data?.reforgeOrbsOwned;
  const reforgeOrbsKnown = data?.reforgeOrbsKnown === true &&
    typeof reforgeOrbsOwned === "number" &&
    Number.isSafeInteger(reforgeOrbsOwned) &&
    reforgeOrbsOwned >= 0;

  const fragmentStreak = data?.runewordFragmentFailStreak;
  const fragmentThreshold = data?.runewordFragmentFailStreakThreshold;
  const orbStreak = data?.reforgeOrbFailStreak;
  const orbThreshold = data?.reforgeOrbFailStreakThreshold;
  const pityKnown = data?.pityKnown === true &&
    typeof fragmentStreak === "number" &&
    Number.isSafeInteger(fragmentStreak) &&
    fragmentStreak >= 0 &&
    typeof fragmentThreshold === "number" &&
    Number.isSafeInteger(fragmentThreshold) &&
    fragmentThreshold > 0 &&
    fragmentStreak <= fragmentThreshold &&
    typeof orbStreak === "number" &&
    Number.isSafeInteger(orbStreak) &&
    orbStreak >= 0 &&
    typeof orbThreshold === "number" &&
    Number.isSafeInteger(orbThreshold) &&
    orbThreshold > 0 &&
    orbStreak <= orbThreshold;

  const normalized = {
    received: true,
    runeInventoryKnown,
    runeInventoryExpectedCount,
    runes,
    reforgeOrbsKnown,
    reforgeOrbsOwned: reforgeOrbsKnown ? reforgeOrbsOwned : 0,
    pityKnown,
    runewordFragmentFailStreak: pityKnown ? fragmentStreak : 0,
    runewordFragmentFailStreakThreshold: pityKnown ? fragmentThreshold : 0,
    reforgeOrbFailStreak: pityKnown ? orbStreak : 0,
    reforgeOrbFailStreakThreshold: pityKnown ? orbThreshold : 0
  };

  return {
    state: normalized,
    signature: JSON.stringify(normalized)
  };
}

function applyResourceDashboardSnapshot(data) {
  const next = normalizeResourceDashboardSnapshot(data);
  if (next.signature === resourceDashboardSignatureState) {
    return false;
  }

  resourceDashboardSignatureState = next.signature;
  resourceDashboardState = next.state;
  schedulePanelRender(panelRenderSection.resourceDashboard);
  return true;
}

function setResourceProgress(progress, valueNode, stateNode, received, known, streak, threshold, kind) {
  if (!progress || !valueNode || !stateNode) {
    return;
  }

  if (!known) {
    progress.setAttribute("max", "1");
    progress.removeAttribute("value");
    progress.setAttribute("aria-busy", received ? "false" : "true");
    progress.setAttribute(
      "aria-label",
      !received
        ? kind === "fragment"
          ? t("Rune fragment pity is synchronizing", "룬 조각 피티 동기화 중")
          : t("Reforge Orb pity is synchronizing", "재련 오브 피티 동기화 중")
        : kind === "fragment"
          ? t("Rune fragment pity is unavailable", "룬 조각 피티 사용 불가")
          : t("Reforge Orb pity is unavailable", "재련 오브 피티 사용 불가")
    );
    progress.removeAttribute("aria-valuetext");
    valueNode.textContent = "— / —";
    stateNode.textContent = received
      ? t(
          "Pity data is unavailable; no count is assumed.",
          "피티 데이터를 사용할 수 없어 수치를 추정하지 않습니다."
        )
      : t(
          "Synchronizing eligible ordinary drop rolls.",
          "적격 일반 드랍 판정을 동기화 중입니다."
        );
    return;
  }

  progress.setAttribute("max", String(threshold));
  const boundedStreak = Math.min(streak, threshold);
  const guaranteeReady = streak >= threshold;
  progress.setAttribute("value", String(boundedStreak));
  progress.setAttribute("aria-busy", "false");
  valueNode.textContent = `${streak} / ${threshold}`;

  const label = kind === "fragment"
    ? t("Rune fragment pity", "룬 조각 피티")
    : t("Reforge Orb pity", "재련 오브 피티");
  const valueText = guaranteeReady
    ? t(
        `${streak} eligible misses recorded; the next eligible ordinary drop roll is guaranteed.`,
        `적격 실패 ${streak}회 기록; 다음 적격 일반 드랍 판정에서 확정 지급됩니다.`
      )
    : t(
        `${streak} of ${threshold} eligible ordinary drop misses recorded.`,
        `적격 일반 드랍 실패 ${streak}/${threshold}회가 기록되었습니다.`
      );
  progress.setAttribute("aria-label", label);
  progress.setAttribute("aria-valuetext", valueText);
  stateNode.textContent = guaranteeReady
    ? t(
        "Next eligible ordinary drop roll: guaranteed.",
        "다음 적격 일반 드랍 판정: 확정 지급."
      )
    : t(
        "Eligible ordinary drop misses recorded.",
        "적격 일반 드랍 실패가 기록됩니다."
      );
}

function renderResourceRuneList(state, totalOwned) {
  if (!resourceRuneList || !resourceRuneDetailsSummary) {
    return;
  }

  clearChildren(resourceRuneList);
  if (!state.runeInventoryKnown) {
    resourceRuneDetailsSummary.textContent = state.received
      ? t("Rune Fragment Inventory · Unavailable", "룬 조각 보유량 · 사용 불가")
      : t("Rune Fragment Inventory · Synchronizing", "룬 조각 보유량 · 동기화 중");
    const unknown = document.createElement("li");
    unknown.className = "rdRuneEmpty";
    unknown.textContent = state.received
      ? t(
          "Rune inventory is unavailable because the received snapshot was not valid.",
          "수신한 룬 보유량 스냅샷이 유효하지 않아 표시하지 않습니다."
        )
      : t(
          "Rune inventory is synchronizing; no count is assumed.",
          "룬 보유량을 동기화 중이며 수치를 추정하지 않습니다."
        );
    resourceRuneList.appendChild(unknown);
    return;
  }

  resourceRuneDetailsSummary.textContent = t(
    `Rune Fragment Inventory · ${totalOwned} total`,
    `룬 조각 보유량 · 총 ${totalOwned}개`
  );
  const displayRunes = [...state.runes].sort((left, right) =>
    left.runeName.localeCompare(right.runeName)
  );
  for (const rune of displayRunes) {
    const item = document.createElement("li");
    item.className = rune.owned > 0 ? "rdRuneItem" : "rdRuneItem empty";
    item.setAttribute(
      "aria-label",
      t(`${rune.runeName}: ${rune.owned} owned`, `${rune.runeName}: ${rune.owned}개 보유`)
    );

    const name = document.createElement("span");
    name.className = "rdRuneName";
    name.textContent = rune.runeName;
    item.appendChild(name);

    const owned = document.createElement("strong");
    owned.className = "rdRuneOwned";
    owned.textContent = String(rune.owned);
    item.appendChild(owned);
    resourceRuneList.appendChild(item);
  }
}

function renderResourceDashboard() {
  if (!resourceDashboardSection) {
    return;
  }

  const state = resourceDashboardState;
  const allKnown = state.runeInventoryKnown && state.reforgeOrbsKnown && state.pityKnown;
  const anyKnown = state.runeInventoryKnown || state.reforgeOrbsKnown || state.pityKnown;
  resourceDashboardSection.setAttribute("aria-busy", state.received ? "false" : "true");
  if (resourceOrbDashboardSection) {
    resourceOrbDashboardSection.setAttribute("aria-busy", state.received ? "false" : "true");
  }

  resourceDashboardTitle.textContent = t("Rune Resources & Pity", "룬 자원 및 피티");
  resourceDashboardLead.textContent = t(
    "Current rune fragments and the ordinary rune-drop pity streak.",
    "현재 룬 조각과 일반 룬 드랍 피티 연속 실패를 표시합니다."
  );
  resourceDashboardSync.textContent = !state.received
    ? t("Synchronizing", "동기화 중")
    : allKnown
    ? t("Current", "현재 상태")
    : anyKnown
      ? t("Partially unavailable", "일부 사용 불가")
      : t("Unavailable", "사용 불가");
  resourceDashboardSync.classList.toggle("ready", allKnown);

  resourceRuneTotalLabel.textContent = t("Rune Fragments", "룬 조각");
  resourceRuneTotalMeta.textContent = t("Total owned", "총 보유량");
  resourceRuneKindsLabel.textContent = t("Rune Types", "룬 종류");
  resourceRuneKindsMeta.textContent = t("Types with at least one", "1개 이상 보유");
  resourceReforgeOrbsLabel.textContent = t("Reforge Orbs", "재련 오브");
  resourceReforgeOrbsMeta.textContent = t("Current inventory", "현재 보유량");

  let totalOwned = 0;
  let ownedKinds = 0;
  if (state.runeInventoryKnown) {
    for (const rune of state.runes) {
      totalOwned += rune.owned;
      if (rune.owned > 0) {
        ownedKinds += 1;
      }
    }
    resourceRuneTotal.textContent = String(totalOwned);
    resourceRuneKinds.textContent = `${ownedKinds} / ${state.runeInventoryExpectedCount}`;
  } else {
    resourceRuneTotal.textContent = "—";
    resourceRuneKinds.textContent = "— / —";
  }
  resourceReforgeOrbs.textContent = state.reforgeOrbsKnown
    ? String(state.reforgeOrbsOwned)
    : "—";

  resourceFragmentPityLabel.textContent = t("Rune Fragment Pity", "룬 조각 피티");
  resourceOrbPityLabel.textContent = t("Reforge Orb Pity", "재련 오브 피티");
  setResourceProgress(
    resourceFragmentPityProgress,
    resourceFragmentPityValue,
    resourceFragmentPityState,
    state.received,
    state.pityKnown,
    state.runewordFragmentFailStreak,
    state.runewordFragmentFailStreakThreshold,
    "fragment"
  );
  setResourceProgress(
    resourceOrbPityProgress,
    resourceOrbPityValue,
    resourceOrbPityState,
    state.received,
    state.pityKnown,
    state.reforgeOrbFailStreak,
    state.reforgeOrbFailStreakThreshold,
    "orb"
  );

  resourcePityHint.textContent = t(
    "Only eligible ordinary rune-fragment drop rolls affect this streak; unrelated kills and forced rewards do not. At the threshold, the next eligible ordinary drop roll is guaranteed.",
    "적격 일반 룬 조각 드랍 판정만 이 연속 실패를 바꾸며, 무관한 처치나 강제 지급은 포함되지 않습니다. 기준치에 도달하면 다음 적격 일반 드랍 판정에서 확정 지급됩니다."
  );
  if (resourceOrbPityHint) {
    resourceOrbPityHint.textContent = t(
      "Only eligible ordinary Reforge Orb drop rolls affect this streak; unrelated kills and forced rewards do not. At the threshold, the next eligible ordinary drop roll is guaranteed.",
      "적격 일반 재련 오브 드랍 판정만 이 연속 실패를 바꾸며, 무관한 처치나 강제 지급은 포함되지 않습니다. 기준치에 도달하면 다음 적격 일반 드랍 판정에서 확정 지급됩니다."
    );
  }
  resourceRuneList.setAttribute(
    "aria-label",
    t("Rune fragment inventory", "룬 조각 보유량")
  );
  renderResourceRuneList(state, totalOwned);
}
