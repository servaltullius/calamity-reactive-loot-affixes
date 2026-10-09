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

  const isKnownCount = (known, owned) => known === true &&
    typeof owned === "number" &&
    Number.isSafeInteger(owned) &&
    owned >= 0;
  const reforgeOrbsOwned = data?.reforgeOrbsOwned;
  const reforgeOrbsKnown = isKnownCount(data?.reforgeOrbsKnown, reforgeOrbsOwned);
  const identifyScrollsKnown = isKnownCount(data?.identifyScrollsKnown, data?.identifyScrollsOwned);
  const scouringOrbsKnown = isKnownCount(data?.scouringOrbsKnown, data?.scouringOrbsOwned);

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
    identifyScrollsKnown,
    identifyScrollsOwned: identifyScrollsKnown ? data.identifyScrollsOwned : 0,
    scouringOrbsKnown,
    scouringOrbsOwned: scouringOrbsKnown ? data.scouringOrbsOwned : 0,
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
          ? t("Loading rune fragment pity", "룬 조각 천장 불러오는 중")
          : t("Loading Reforge Orb pity", "재련 오브 천장 불러오는 중")
        : kind === "fragment"
          ? t("Rune fragment pity unavailable", "룬 조각 천장 정보 없음")
          : t("Reforge Orb pity unavailable", "재련 오브 천장 정보 없음")
    );
    progress.removeAttribute("aria-valuetext");
    valueNode.textContent = "— / —";
    stateNode.textContent = received
      ? t("Couldn't read pity data.", "천장 정보를 읽지 못했습니다.")
      : t("Loading…", "불러오는 중…");
    stateNode.hidden = false;
    return;
  }

  progress.setAttribute("max", String(threshold));
  const boundedStreak = Math.min(streak, threshold);
  const guaranteeReady = streak >= threshold;
  progress.setAttribute("value", String(boundedStreak));
  progress.setAttribute("aria-busy", "false");
  valueNode.textContent = `${streak} / ${threshold}`;

  const label = kind === "fragment"
    ? t("Rune fragment pity", "룬 조각 천장")
    : t("Reforge Orb pity", "재련 오브 천장");
  const valueText = guaranteeReady
    ? t("Full: the next drop is guaranteed.", "가득 참: 다음 드랍 확정.")
    : t(
        `${streak} of ${threshold}; when full, the next drop is guaranteed.`,
        `${streak}/${threshold}, 다 차면 다음 드랍 확정.`
      );
  progress.setAttribute("aria-label", label);
  progress.setAttribute("aria-valuetext", valueText);
  // How pity works is the same every time, so it lives on hover (and in
  // aria-valuetext); the line under the bar only speaks when something changes.
  stateNode.textContent = guaranteeReady ? t("Next drop guaranteed!", "다음 드랍 확정!") : "";
  stateNode.hidden = !guaranteeReady;
  if (stateNode.parentElement) {
    stateNode.parentElement.title = t(
      "Fills on each miss; when full, the next drop is guaranteed.",
      "못 얻을 때마다 차고, 다 차면 다음 드랍은 확정입니다."
    );
  }
}

function renderResourceRuneList(state, totalOwned) {
  if (!resourceRuneList || !resourceRuneDetailsSummary) {
    return;
  }

  clearChildren(resourceRuneList);
  if (!state.runeInventoryKnown) {
    resourceRuneDetailsSummary.textContent = state.received
      ? t("Rune Fragment Inventory · Unavailable", "룬 조각 보유량 · 읽지 못함")
      : t("Rune Fragment Inventory · Loading", "룬 조각 보유량 · 불러오는 중");
    const unknown = document.createElement("li");
    unknown.className = "rdRuneEmpty";
    unknown.textContent = state.received
      ? t("Couldn't read rune inventory.", "룬 보유량을 읽지 못했습니다.")
      : t("Loading…", "불러오는 중…");
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

  resourceDashboardTitle.textContent = t("Runes & Pity", "룬 조각과 천장");
  // The badge only speaks up when something is off; a "current" chip that
  // is always on says nothing.
  resourceDashboardSync.textContent = !state.received
    ? t("Loading", "불러오는 중")
    : allKnown
    ? ""
    : anyKnown
      ? t("Partly unavailable", "일부 읽지 못함")
      : t("Unavailable", "읽지 못함");
  resourceDashboardSync.hidden = state.received && allKnown;
  resourceDashboardSync.classList.toggle("ready", allKnown);

  resourceRuneTotalLabel.textContent = t("Rune Fragments", "룬 조각");
  resourceRuneTotalMeta.textContent = t("Total owned", "총 보유량");
  resourceRuneKindsLabel.textContent = t("Rune Types", "룬 종류");
  resourceRuneKindsMeta.textContent = t("Collected", "모은 종류");
  if (resourceCraftingTitle) {
    resourceCraftingTitle.textContent = t("Crafting Resources", "제작 재료");
  }
  resourceReforgeOrbsLabel.textContent = t("Reforge Orbs", "재련 오브");
  if (resourceIdentifyScrollsLabel) {
    resourceIdentifyScrollsLabel.textContent = t("Identify Scrolls", "확인 스크롤");
  }
  if (resourceScouringOrbsLabel) {
    resourceScouringOrbsLabel.textContent = t("Scouring Orbs", "정제 오브");
  }
  // Metas are one short line; bilingual text would be cut, so show one
  // language and keep both on hover.
  for (const [meta, en, ko] of [
    [resourceIdentifyScrollsMeta, "First affixes", "최초 부여"],
    [resourceReforgeOrbsMeta, "Reforge · expand", "재련·확장"],
    [resourceScouringOrbsMeta, "Full reroll", "전체 리롤"]
  ]) {
    if (meta) {
      meta.textContent = tCompact(en, ko);
      meta.title = t(en, ko);
    }
  }

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
  if (resourceIdentifyScrolls) {
    resourceIdentifyScrolls.textContent = state.identifyScrollsKnown
      ? String(state.identifyScrollsOwned)
      : "—";
  }
  if (resourceScouringOrbs) {
    resourceScouringOrbs.textContent = state.scouringOrbsKnown
      ? String(state.scouringOrbsOwned)
      : "—";
  }

  resourceFragmentPityLabel.textContent = t("Rune Fragment Pity", "룬 조각 천장");
  resourceOrbPityLabel.textContent = t("Reforge Orb Pity", "재련 오브 천장");
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
    "Goes up by 1 each time a kill that could drop a rune fragment doesn't. When full, the next one is guaranteed.",
    "룬 조각이 나올 수 있는 처치에서 못 얻으면 1씩 오릅니다. 다 차면 다음엔 반드시 나옵니다."
  );
  if (resourceOrbPityHint) {
    resourceOrbPityHint.textContent = t(
      "Goes up by 1 each time a kill that could drop a Reforge Orb doesn't. When full, the next one is guaranteed.",
      "재련 오브가 나올 수 있는 처치에서 못 얻으면 1씩 오릅니다. 다 차면 다음엔 반드시 나옵니다."
    );
  }
  resourceRuneList.setAttribute(
    "aria-label",
    t("Rune fragment inventory", "룬 조각 보유량")
  );
  renderResourceRuneList(state, totalOwned);
}
