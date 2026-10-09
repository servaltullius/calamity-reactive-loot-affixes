function resolveReforgeCandidateName(candidate, language) {
  const en = typeof candidate?.displayNameEn === "string"
    ? candidate.displayNameEn.trim()
    : "";
  const ko = typeof candidate?.displayNameKo === "string"
    ? candidate.displayNameKo.trim()
    : "";
  if (language === "ko") {
    return ko || en || "알 수 없는 어픽스";
  }
  return en || ko || "Unknown affix";
}

function resolveReforgeLockCandidates(state) {
  const regularAffixCount = Number.isFinite(Number(state?.regularAffixCount))
    ? Math.max(0, Math.trunc(Number(state.regularAffixCount)))
    : 0;
  if (regularAffixCount < 1 || !Array.isArray(state?.reforgeLockCandidates)) {
    return [];
  }
  return state.reforgeLockCandidates;
}

function resolveActiveReforgeLockCandidate(state) {
  if (!reforgeLockTokenState) {
    return null;
  }

  const selectedBaseKey = resolveSelectedRunewordBaseKey();
  if (!selectedBaseKey || selectedBaseKey !== reforgeLockBaseKeyState) {
    return null;
  }

  return resolveReforgeLockCandidates(state).find(
    (candidate) => candidate?.affixToken === reforgeLockTokenState
  ) || null;
}

function clearReforgeLockSelection(scheduleRender = true) {
  const changed = Boolean(reforgeLockTokenState || reforgeLockBaseKeyState);
  reforgeLockTokenState = "";
  reforgeLockBaseKeyState = "";
  if (changed && scheduleRender) {
    schedulePanelRender(panelRenderSection.runewordPanelState);
  }
  return changed;
}

function selectReforgeLockCandidate(affixToken, scheduleRender = true) {
  const token = typeof affixToken === "string" ? affixToken.trim() : "";
  if (!token) {
    clearReforgeLockSelection(scheduleRender);
    return true;
  }

  const selectedBaseKey = resolveSelectedRunewordBaseKey();
  const candidate = resolveReforgeLockCandidates(runewordPanelState).find(
    (entry) => entry?.affixToken === token
  );
  if (!selectedBaseKey || !runewordPanelState.hasBase || !candidate) {
    return false;
  }

  const changed = token !== reforgeLockTokenState || selectedBaseKey !== reforgeLockBaseKeyState;
  reforgeLockTokenState = token;
  reforgeLockBaseKeyState = selectedBaseKey;
  if (changed && scheduleRender) {
    schedulePanelRender(panelRenderSection.runewordPanelState);
  }
  return true;
}

function reconcileReforgeLockSelection(scheduleRender = true) {
  if (!reforgeLockTokenState) {
    if (reforgeLockBaseKeyState) {
      reforgeLockBaseKeyState = "";
    }
    return false;
  }

  if (resolveActiveReforgeLockCandidate(runewordPanelState)) {
    return false;
  }
  return clearReforgeLockSelection(scheduleRender);
}

function buildReforgeCommand(state = runewordPanelState) {
  const lockedCandidate = resolveActiveReforgeLockCandidate(state);
  return lockedCandidate
    ? `${lockedReforgeCommandPrefix}${reforgeLockBaseKeyState}:${lockedCandidate.affixToken}`
    : "";
}

function buildAffixExpandCommand(selectedBaseKey, expectedRegularAffixCount) {
  const baseKey = normalizePositiveUint64DecimalString(selectedBaseKey);
  if (
    !baseKey ||
    !Number.isSafeInteger(expectedRegularAffixCount) ||
    (expectedRegularAffixCount !== 1 && expectedRegularAffixCount !== 2)
  ) {
    return "";
  }
  return `${affixExpandCommandPrefix}${baseKey}:${expectedRegularAffixCount}`;
}

function resolveAffixExpandUnavailableText(reason, state) {
  switch (reason) {
    case "no_base":
      return t(
        "Pick an item to see and expand its affix slots.",
        "장비를 고르면 어픽스 슬롯을 확인하고 늘릴 수 있습니다."
      );
    case "requires_first_affix":
      return t(
        "Identify this base with a Scroll of Identification before expanding slots.",
        "확인 스크롤로 어픽스를 부여한 뒤 슬롯을 확장할 수 있습니다."
      );
    case "max_slots":
      return t(
        "All affix slots are unlocked.",
        "모든 어픽스 슬롯이 열렸습니다."
      );
    case "invalid_layout":
      return t(
        "This item's affix layout can't be expanded.",
        "이 장비의 어픽스 구성은 늘릴 수 없습니다."
      );
    case "suffix_slots_disabled":
      return t(
        "Suffix slot expansion is disabled by the current configuration.",
        "현재 설정에서 접미 어픽스 슬롯 확장이 비활성화되어 있습니다."
      );
    case "insufficient_orbs": {
      const cost = state.expandAffixCost;
      const owned = state.reforgeOrbsOwned;
      if (
        Number.isSafeInteger(cost) &&
        cost > 0 &&
        Number.isSafeInteger(owned) &&
        owned >= 0 &&
        owned < cost
      ) {
        const shortfall = cost - owned;
        return t(
          `You need ${shortfall} more Reforge Orb${shortfall === 1 ? "" : "s"}.`,
          `재련 오브가 ${shortfall}개 부족합니다.`
        );
      }
      return t(
        "You do not have enough Reforge Orbs.",
        "재련 오브가 부족합니다."
      );
    }
    default:
      return t(
        "Affix slot expansion is currently unavailable.",
        "현재 어픽스 슬롯 확장을 사용할 수 없습니다."
      );
  }
}

function resolveAffixSlotProgressState(
  state,
  selectedBaseKey = resolveSelectedRunewordBaseKey(),
  pendingState = affixExpandPendingState
) {
  const hasBase = Boolean(state?.hasBase);
  const baseKey = normalizePositiveUint64DecimalString(selectedBaseKey);
  const hasValidBase = hasBase && Boolean(baseKey);
  const regularAffixCountKnown = state?.regularAffixCountKnown === true &&
    Number.isSafeInteger(state?.regularAffixCount) &&
    state.regularAffixCount >= 0;
  // Filled slots of the three: a runeword holding the head slot counts as
  // one. Older DLLs send no affixSlotCount; then only regular affixes count.
  const affixHead = typeof state?.affixHead === "string" ? state.affixHead : "";
  const runewordHead = affixHead === "runeword";
  const regularAffixCount = regularAffixCountKnown
    ? (Number.isSafeInteger(state?.affixSlotCount) ? state.affixSlotCount : state.regularAffixCount)
    : 0;
  const maxRegularAffixCountKnown = state?.maxRegularAffixCountKnown === true &&
    state?.maxRegularAffixCount === 3;
  const maxRegularAffixCount = 3;
  const expandAffixCost = Number.isSafeInteger(state?.expandAffixCost) &&
    state.expandAffixCost > 0
    ? state.expandAffixCost
    : null;
  const reforgeOrbsOwned = Number.isSafeInteger(state?.reforgeOrbsOwned) &&
    state.reforgeOrbsOwned >= 0
    ? state.reforgeOrbsOwned
    : null;
  const reforgeOrbsKnown = state?.reforgeOrbsKnown === true && reforgeOrbsOwned !== null;
  const expectedRegularAffixCount = regularAffixCount === 1 || regularAffixCount === 2
    ? regularAffixCount
    : null;
  const expandCommand = expectedRegularAffixCount === null
    ? ""
    : buildAffixExpandCommand(baseKey, expectedRegularAffixCount);
  const pending = Boolean(pendingState || affixCraftPendingState);
  const hasEnoughOrbs = expandAffixCost !== null &&
    reforgeOrbsKnown &&
    reforgeOrbsOwned >= expandAffixCost;
  const structurallyReady = hasBase &&
    Boolean(baseKey) &&
    regularAffixCountKnown &&
    maxRegularAffixCountKnown &&
    expectedRegularAffixCount !== null &&
    Boolean(expandCommand) &&
    expandAffixCost !== null;
  const expandEnabled = state?.canExpandAffix === true &&
    structurallyReady &&
    hasEnoughOrbs &&
    !pending;

  let unavailableReason = normalizeAffixExpandUnavailableReason(
    state?.expandAffixUnavailableReason
  );
  if (!hasBase || !baseKey) {
    unavailableReason = "no_base";
  } else if (!regularAffixCountKnown || !maxRegularAffixCountKnown) {
    unavailableReason = "unavailable";
  } else if (regularAffixCount === 0) {
    unavailableReason = "requires_first_affix";
  } else if (regularAffixCount >= maxRegularAffixCount) {
    unavailableReason = "max_slots";
  } else if (expandAffixCost === null) {
    unavailableReason = "unavailable";
  } else if (
    unavailableReason === "invalid_layout" ||
    unavailableReason === "suffix_slots_disabled"
  ) {
    // Preserve the server-authoritative structural reason even when the
    // inventory snapshot is unavailable or the player also lacks currency.
  } else if (!reforgeOrbsKnown) {
    unavailableReason = "unavailable";
  } else if (!hasEnoughOrbs) {
    unavailableReason = "insufficient_orbs";
  }

  const displayCount = regularAffixCountKnown ? regularAffixCount : null;
  const countText = !hasValidBase
    ? `—/${maxRegularAffixCount}`
    : displayCount === null
      ? `…/${maxRegularAffixCount}`
      : `${displayCount}/${maxRegularAffixCount}`;
  const nextSlotNumber = expectedRegularAffixCount === null
    ? null
    : expectedRegularAffixCount + 1;
  const slotLabels = [
    runewordHead ? t("Runeword", "룬워드") : t("Prefix", "접두"),
    t("Suffix 1", "접미 1"),
    t("Suffix 2", "접미 2")
  ];
  const ariaValueText = !hasValidBase
    ? t("No item selected", "선택한 장비 없음")
    : displayCount === null
      ? t("Loading affix slots", "어픽스 슬롯 불러오는 중")
      : t(
          `${displayCount} of ${maxRegularAffixCount} affix slots filled`,
          `어픽스 슬롯 ${maxRegularAffixCount}칸 중 ${displayCount}칸 사용`
        );

  let progressMeta = "";
  let expandButtonLabel = "";
  let expandHint = "";
  if (pending) {
    progressMeta = t(
      "Unlocking one suffix slot while preserving current affixes and runeword…",
      "현재 어픽스와 룬워드를 유지하며 접미 슬롯을 여는 중…"
    );
    expandButtonLabel = t(
      `Unlocking Slot ${nextSlotNumber || ""}…`.trim(),
      `${nextSlotNumber || ""}번 슬롯 여는 중…`.trim()
    );
    expandHint = progressMeta;
  } else if (expandEnabled) {
    progressMeta = t(
      "Next: add one suffix while preserving current affixes and runeword.",
      "다음: 현재 어픽스와 룬워드를 유지하고 접미 1개를 추가합니다."
    );
    expandButtonLabel = t(
      `Unlock Slot ${nextSlotNumber} · ${expandAffixCost} Orb${expandAffixCost === 1 ? "" : "s"}`,
      `${nextSlotNumber}번 슬롯 열기 · 오브 ${expandAffixCost}개`
    );
    expandHint = t(
      `Consume ${expandAffixCost} Reforge Orb${expandAffixCost === 1 ? "" : "s"} to add one suffix. Existing regular affixes and any completed runeword are preserved.`,
      `재련 오브 ${expandAffixCost}개를 소모해 접미 1개를 추가합니다. 기존 일반 어픽스와 완성된 룬워드는 유지됩니다.`
    );
  } else {
    progressMeta = resolveAffixExpandUnavailableText(unavailableReason, {
      expandAffixCost,
      reforgeOrbsOwned
    });
    if (unavailableReason === "no_base") {
      expandButtonLabel = t("Select Item", "장비 선택 필요");
    } else if (unavailableReason === "requires_first_affix") {
      expandButtonLabel = t("Identify First", "확인 스크롤 먼저 사용");
    } else if (unavailableReason === "max_slots") {
      expandButtonLabel = t("Max Slots 3/3", "최대 슬롯 3/3");
    } else if (
      unavailableReason === "insufficient_orbs" &&
      nextSlotNumber !== null &&
      expandAffixCost !== null
    ) {
      expandButtonLabel = t(
        `Unlock Slot ${nextSlotNumber} · ${expandAffixCost} Orb${expandAffixCost === 1 ? "" : "s"}`,
        `${nextSlotNumber}번 슬롯 열기 · 오브 ${expandAffixCost}개`
      );
    } else {
      expandButtonLabel = t("Slot Expansion Unavailable", "슬롯 확장 사용 불가");
    }
    expandHint = progressMeta;
  }

  return {
    hasValidBase,
    affixHead,
    regularAffixCount,
    regularAffixCountKnown,
    maxRegularAffixCount,
    countText,
    slotLabels,
    ariaValueText,
    nextSlotNumber,
    progressMeta,
    unavailableReason,
    expandAffixCost,
    expandCommand,
    expandEnabled,
    expandPending: pending,
    expandButtonLabel,
    expandHint
  };
}

function clearAffixExpandPending(scheduleRender = true) {
  if (!affixExpandPendingState) {
    return false;
  }
  affixExpandPendingState = null;
  affixExpandPendingNonce += 1;
  if (scheduleRender) {
    schedulePanelRender(panelRenderSection.runewordPanelState);
  }
  return true;
}

function beginAffixExpandPending(command) {
  if (affixExpandPendingState) {
    return false;
  }

  const actionState = resolveAffixSlotProgressState(runewordPanelState);
  if (!actionState.expandEnabled || !actionState.expandCommand || command !== actionState.expandCommand) {
    return false;
  }

  const pendingNonce = ++affixExpandPendingNonce;
  affixExpandPendingState = {
    nonce: pendingNonce,
    command,
    expectedRegularAffixCount: actionState.regularAffixCount
  };
  clearReforgeLockSelection(false);
  if (affixExpandButton) {
    affixExpandButton.disabled = true;
  }
  if (runewordReforgeButton) {
    runewordReforgeButton.disabled = true;
  }
  if (runewordInsertButton) {
    runewordInsertButton.disabled = true;
  }
  if (runewordResetButton) {
    runewordResetButton.disabled = true;
  }
  closeWorkingBaseChooser(false);
  schedulePanelRender(panelRenderSection.runewordPanelState);
  window.setTimeout(() => {
    if (affixExpandPendingState?.nonce !== pendingNonce) {
      return;
    }
    clearAffixExpandPending(false);
    setActionFeedback(t(
      "Affix slot expansion timed out. Check the item before trying again.",
      "어픽스 슬롯 확장 응답이 없습니다. 장비 상태를 확인한 뒤 다시 시도하세요."
    ));
    schedulePanelRender(panelRenderSection.runewordPanelState);
  }, affixExpandPendingTimeoutMs);
  return true;
}

function resolveRunewordPanelActionState(state) {
  const hasBase = Boolean(state.hasBase);
  const hasRecipe = Boolean(state.hasRecipe);
  const isComplete = Boolean(state.isComplete);
  const affixExpandPending = Boolean(affixExpandPendingState || affixCraftPendingState);
  const canTransmute = Boolean(state.canInsert) &&
    hasBase &&
    hasRecipe &&
    !isComplete &&
    !affixExpandPending;
  const baseCompatibilityWarning = Boolean(state.baseCompatibilityWarning);
  const baseCompatibilityMessage = baseCompatibilityWarning
    ? t(
        typeof state.baseCompatibilityMessageEn === "string" ? state.baseCompatibilityMessageEn : "This item isn't the recommended base.",
        typeof state.baseCompatibilityMessageKo === "string" ? state.baseCompatibilityMessageKo : "선택한 장비가 권장 베이스와 다릅니다."
      )
    : "";

  let buttonLabel = t("Transmute", "변환");
  let buttonHint = "";

  if (isComplete) {
    buttonLabel = t("Complete", "완료");
    buttonHint = t(
      "This item already has a runeword.",
      "이 장비에는 이미 룬워드가 있습니다."
    );
  } else if (!hasBase) {
    buttonHint = t(
      "Select an item first.",
      "장비를 먼저 고르세요."
    );
  } else if (!hasRecipe) {
    buttonHint = t(
      "Select a runeword recipe first.",
      "룬워드 레시피를 먼저 선택하세요."
    );
  } else if (affixExpandPending) {
    buttonHint = t(
      "Wait for affix slot expansion to finish.",
      "어픽스 슬롯 확장이 끝날 때까지 기다려 주세요."
    );
  } else if (!canTransmute && state.missingSummary) {
    buttonHint = t(
      "Not enough rune fragments.",
      "룬 조각이 부족합니다."
    );
  } else if (!canTransmute) {
    buttonHint = t(
      "Transmute is not available yet.",
      "아직 변환할 수 없습니다."
    );
  } else {
    buttonHint = t(
      "Uses up all the required rune fragments.",
      "필요한 룬 조각을 모두 사용합니다."
    );
  }

  // The runeword takes the head slot, so a transmute removes the prefix there.
  const transmuteRemovesPrefix = Boolean(state.transmuteRemovesPrefixEn || state.transmuteRemovesPrefixKo);
  const transmuteConfirmText = canTransmute && transmuteRemovesPrefix
    ? t(
        `The runeword takes the prefix slot: ${state.transmuteRemovesPrefixEn} will be removed. Suffixes stay.`,
        `룬워드가 접두 칸을 차지합니다. 접두 '${state.transmuteRemovesPrefixKo || state.transmuteRemovesPrefixEn}'이(가) 사라집니다. 접미는 유지됩니다.`
      )
    : "";
  if (transmuteConfirmText) {
    buttonHint = `${transmuteConfirmText}
${buttonHint}`;
  }

  // The base mismatch sentence lives in the review box only (with its badge);
  // repeating it in the header, recipe chip and this note said it four times.

  const regularAffixCount = Number.isSafeInteger(state.regularAffixCount) ? state.regularAffixCount : 0;
  const standardReforgeCost = 2;
  const lockedReforgeCost = 2;
  const affixSlotState = resolveAffixSlotProgressState(state);
  const reforgeLockCandidates = resolveReforgeLockCandidates(state);
  const lockedReforgeCandidate = resolveActiveReforgeLockCandidate(state);
  const reforgeCost = 2;
  // Per-item limit from the runtime; null means an older DLL with no limit.
  const reforgeLimit = Number.isSafeInteger(state.selectedReforgesPerItem) && state.selectedReforgesPerItem > 0
    ? state.selectedReforgesPerItem
    : 0;
  const reforgesLeft = reforgeLimit > 0 && Number.isSafeInteger(state.selectedReforgesLeft)
    ? Math.min(Math.max(state.selectedReforgesLeft, 0), reforgeLimit)
    : null;
  const reforgesSpent = reforgesLeft === 0;
  const reforgeOrbsOwned = state.reforgeOrbsKnown === true && Number.isSafeInteger(state.reforgeOrbsOwned) ? state.reforgeOrbsOwned : null;
  const reforgeCommand = buildReforgeCommand(state);
  const baseKey = normalizePositiveUint64DecimalString(resolveSelectedRunewordBaseKey());
  const ready = hasBase && Boolean(baseKey) && state.regularAffixCountKnown === true && !affixSlotState.expandPending;
  const reforgeEnabled = ready && Boolean(lockedReforgeCandidate) && reforgeOrbsOwned !== null &&
    reforgeOrbsOwned >= reforgeCost && !reforgesSpent;
  const identifyEnabled = ready && regularAffixCount === 0 && state.identifyScrollsKnown === true && state.identifyScrollsOwned >= 1;
  const scourEnabled = ready && regularAffixCount > 0 && state.scouringOrbsKnown === true && state.scouringOrbsOwned >= 1;
  const identifyCommand = baseKey ? `affix.identify:${baseKey}` : "";
  const scourCommand = baseKey ? `affix.scour:${baseKey}` : "";
  const runewordHead = affixSlotState.affixHead === "runeword";
  const hasRuneword = runewordHead || affixSlotState.affixHead === "legacy";
  const removeRunewordEnabled = ready && hasRuneword && state.canRemoveRuneword === true &&
    state.scouringOrbsKnown === true && state.scouringOrbsOwned >= 1;
  const removeRunewordCommand = baseKey && hasRuneword ? `runeword.remove:${baseKey}` : "";
  // Scroll trades need no base; costs come from the runtime (0 = unavailable).
  const scrollsOwned = state.identifyScrollsKnown === true && Number.isSafeInteger(state.identifyScrollsOwned)
    ? state.identifyScrollsOwned
    : null;
  const exchangeReforgeCost = Number.isSafeInteger(state.exchangeReforgeScrollCost) ? state.exchangeReforgeScrollCost : 0;
  const exchangeScourCost = Number.isSafeInteger(state.exchangeScourScrollCost) ? state.exchangeScourScrollCost : 0;
  const exchangeReforgeEnabled = !affixSlotState.expandPending && exchangeReforgeCost > 0 &&
    scrollsOwned !== null && scrollsOwned >= exchangeReforgeCost;
  const exchangeScourEnabled = !affixSlotState.expandPending && exchangeScourCost > 0 &&
    scrollsOwned !== null && scrollsOwned >= exchangeScourCost;
  const selectedName = lockedReforgeCandidate ? t(resolveReforgeCandidateName(lockedReforgeCandidate, "en"), resolveReforgeCandidateName(lockedReforgeCandidate, "ko")) : "";
  const reforgeHint = reforgeLimit > 0
    ? t(
      `Spend 2 Reforge Orbs to replace only the selected regular affix. Other affixes, slot count, and runeword progress stay. Each item allows ${reforgeLimit}; a Scouring Orb gives them back.`,
      `재련 오브 2개로 선택한 일반 어픽스 하나만 바꿉니다. 나머지 어픽스·슬롯 수·룬워드 성장 상태는 유지됩니다. 장비마다 ${reforgeLimit}번까지이며, 정제 오브를 쓰면 다시 ${reforgeLimit}번이 됩니다.`
    )
    : t(
      "Spend 2 Reforge Orbs to replace only the selected regular affix. Other affixes, slot count, and runeword progress stay. No attempt limit.",
      "재련 오브 2개로 선택한 일반 어픽스 하나만 바꿉니다. 나머지 어픽스·슬롯 수·룬워드 성장 상태는 유지됩니다. 횟수 제한은 없습니다."
    );
  const reforgeButtonLabel = t("Reforge Selected (2 Orbs)", "선택 어픽스 재련 (오브 2개)");
  const reforgeSummary = selectedName ? t(`Replace: ${selectedName}`, `교체 대상: ${selectedName}`) : t("Choose an affix to replace", "바꿀 어픽스 선택");
  const reforgeLockHint = regularAffixCount === 0
    ? (runewordHead
      ? t("Identify first: 1–2 suffixes. Expand missing slots later.", "확인 스크롤로 접미 1~2개를 부여하세요. 부족한 슬롯은 확장할 수 있습니다.")
      : t("Identify first: 1–3 regular affixes. Expand missing slots later.", "확인 스크롤로 일반 어픽스 1~3개를 부여하세요. 부족한 슬롯은 확장할 수 있습니다."))
    : t("Choose the one affix to reroll. All other effects are preserved.", "다시 굴릴 어픽스 하나를 선택하세요. 다른 효과는 유지됩니다.");
  const reforgeCostSummary = reforgesSpent
    ? t(
      `No reforges left on this item. Scour it to get ${reforgeLimit} back.`,
      `이 장비는 재련을 모두 썼습니다. 정제하면 다시 ${reforgeLimit}번 할 수 있습니다.`
    )
    : reforgesLeft !== null
      ? t(
        `Cost: 2 Reforge Orbs · Owned: ${reforgeOrbsOwned ?? "?"} · Left on this item: ${reforgesLeft}/${reforgeLimit}`,
        `비용: 재련 오브 2개 · 보유: ${reforgeOrbsOwned ?? "?"}개 · 이 장비 남은 재련: ${reforgesLeft}/${reforgeLimit}`
      )
      : t(`Cost: 2 Reforge Orbs · Owned: ${reforgeOrbsOwned ?? "?"}`, `비용: 재련 오브 2개 · 보유: ${reforgeOrbsOwned ?? "?"}개`);

  const resetEnabled = hasBase && state.debugTools === true && !affixSlotState.expandPending;
  const resetHint = resetEnabled ?
    t(
      "Removes every Calamity affix and runeword from the selected item. Materials are not refunded.",
      "선택한 장비의 칼라미티 어픽스와 룬워드를 모두 지웁니다. 재료는 돌려받지 못합니다."
    ) :
    !hasBase ?
    t(
      "Select an item first.",
      "장비를 먼저 고르세요."
    ) :
    state.debugTools !== true ?
    t(
      "Reset is a debug tool. Enable Debug Notifications in MCM, or use a Scouring Orb to reroll regular affixes.",
      "초기화는 디버그 도구입니다. MCM에서 디버그 알림을 켜거나, 정제 오브로 일반 어픽스를 다시 굴리세요."
    ) :
    t(
      "Wait for the current action to finish.",
      "진행 중인 작업이 끝날 때까지 기다리세요."
    );

  return {
    hasBase,
    hasRecipe,
    isComplete,
    canTransmute,
    baseCompatibilityWarning,
    baseCompatibilityMessage,
    buttonLabel,
    buttonHint,
    transmuteConfirmText,
    identifyEnabled, scourEnabled, identifyCommand, scourCommand,
    runewordHead, hasRuneword, removeRunewordEnabled, removeRunewordCommand,
    exchangeReforgeCost, exchangeScourCost, exchangeReforgeEnabled, exchangeScourEnabled,
    reforgeEnabled,
    reforgeHint,
    reforgeButtonLabel,
    reforgeSummary,
    reforgeLockHint,
    reforgeCostSummary,
    reforgeCommand,
    reforgeCost,
    reforgeLimit,
    reforgesLeft,
    reforgeOrbsOwned,
    regularAffixCount,
    standardReforgeCost,
    lockedReforgeCost,
    reforgeLockCandidates,
    lockedReforgeCandidate,
    affixSlotState,
    resetEnabled,
    resetHint
  };
}

function triggerRunewordStateShift(element, state) {
  if (!element) return;
  const previous = element.dataset.stepState || "";
  if (previous === state) {
    return;
  }

  element.dataset.stepState = state;
  const shiftNonce = String((Number(element.dataset.shiftNonce) || 0) + 1);
  element.dataset.shiftNonce = shiftNonce;
  element.classList.add("rwStateShift");
  window.setTimeout(() => {
    if (element.dataset.shiftNonce === shiftNonce) {
      element.classList.remove("rwStateShift");
    }
  }, 170);
}

function setRunewordStepCardState(element, state) {
  if (!element) return;
  triggerRunewordStateShift(element, state);
  element.classList.toggle("active", state === "active");
  element.classList.toggle("complete", state === "complete");
  element.classList.toggle("muted", state === "muted");
  if (state === "active") {
    element.setAttribute("aria-current", "step");
  } else {
    element.removeAttribute("aria-current");
  }
}

function renderRunewordFlowProgress(actionState, state) {
  const hasBase = Boolean(actionState?.hasBase);
  const hasRecipe = Boolean(actionState?.hasRecipe);
  const isComplete = Boolean(actionState?.isComplete);
  const canTransmute = Boolean(actionState?.canTransmute);

  setRunewordStepCardState(runewordBaseStep, hasBase ? "complete" : "active");
  setRunewordStepCardState(runewordRecipeStep, hasRecipe ? "complete" : hasBase ? "active" : "muted");
  setRunewordStepCardState(
    runewordActionStep,
    isComplete ? "complete" : hasBase && hasRecipe ? "active" : "muted"
  );
  if (runewordInsertButton) {
    runewordInsertButton.classList.toggle("attention", canTransmute);
  }

  if (!runewordFlowHint) {
    return;
  }

  if (!hasBase) {
    runewordFlowHint.textContent = t("Pick an item to work on.", "작업할 장비를 고르세요.");
    return;
  }

  if (!hasRecipe) {
    runewordFlowHint.textContent = t("Pick a recipe.", "레시피를 고르세요.");
    return;
  }

  if (isComplete) {
    runewordFlowHint.textContent = t(
      "This item already has a runeword. Reforging changes only its regular affixes.",
      "이 장비에는 이미 룬워드가 있습니다. 재련은 일반 어픽스만 바꿉니다."
    );
    return;
  }

  if (canTransmute) {
    // The recipe card on the right already says it is ready.
    runewordFlowHint.textContent = "";
    return;
  }

  if (state?.missingSummary) {
    // The recipe card and its badge already list what is missing.
    runewordFlowHint.textContent = "";
    return;
  }

  runewordFlowHint.textContent = "";
}

function renderAffixSlotProgress(actionState) {
  const slotState = actionState?.affixSlotState;
  if (!slotState || !affixSlotProgress || !affixSlotProgressTrack) {
    return;
  }

  if (affixSlotProgressTitle) {
    affixSlotProgressTitle.textContent = t("Affix Slots", "어픽스 슬롯");
  }
  if (affixSlotProgressCount) {
    affixSlotProgressCount.textContent = slotState.countText;
  }
  if (affixSlotProgressMeta) {
    affixSlotProgressMeta.textContent = slotState.progressMeta;
    affixSlotProgressMeta.title = slotState.progressMeta;
  }

  const countForProgress = slotState.regularAffixCountKnown
    ? Math.max(0, Math.min(slotState.maxRegularAffixCount, slotState.regularAffixCount))
    : 0;
  affixSlotProgress.classList.toggle(
    "muted",
    !slotState.hasValidBase || !slotState.regularAffixCountKnown
  );
  affixSlotProgress.classList.toggle("pending", slotState.expandPending);
  affixSlotProgress.setAttribute("aria-busy", slotState.expandPending ? "true" : "false");
  affixSlotProgressTrack.setAttribute(
    "aria-valuemax",
    String(slotState.maxRegularAffixCount)
  );
  if (slotState.hasValidBase && slotState.regularAffixCountKnown) {
    affixSlotProgressTrack.setAttribute("aria-valuenow", String(countForProgress));
  } else {
    affixSlotProgressTrack.removeAttribute("aria-valuenow");
  }
  affixSlotProgressTrack.setAttribute("aria-valuetext", slotState.ariaValueText);

  for (let index = 0; index < affixSlotNodes.length; index += 1) {
    const node = affixSlotNodes[index];
    const active = slotState.hasValidBase && slotState.regularAffixCountKnown && index < countForProgress;
    const next = slotState.hasValidBase &&
      slotState.regularAffixCountKnown &&
      index === countForProgress &&
      countForProgress < slotState.maxRegularAffixCount;
    node.textContent = slotState.slotLabels[index] || "";
    node.classList.toggle("active", active);
    node.classList.toggle("next", !active && next);
    node.dataset.slotState = active ? "active" : next ? "next" : "locked";
    node.setAttribute("aria-hidden", "true");
  }

  if (affixExpandButton) {
    affixExpandButton.disabled = !slotState.expandEnabled;
    affixExpandButton.textContent = slotState.expandButtonLabel;
    affixExpandButton.title = slotState.expandHint;
    affixExpandButton.setAttribute("aria-label", slotState.expandHint);
    if (slotState.expandEnabled && slotState.expandCommand) {
      affixExpandButton.setAttribute(panelCommandAttribute, slotState.expandCommand);
    } else {
      affixExpandButton.removeAttribute(panelCommandAttribute);
    }
  }
}

function renderReforgeLockOptions(actionState) {
  if (!runewordReforgeLockList) {
    return;
  }

  if (runewordReforgeSummary && runewordReforgeSummary.textContent !== actionState.reforgeSummary) {
    runewordReforgeSummary.textContent = actionState.reforgeSummary;
  }
  if (runewordReforgeLockHint && runewordReforgeLockHint.textContent !== actionState.reforgeLockHint) {
    runewordReforgeLockHint.textContent = actionState.reforgeLockHint;
  }
  if (runewordReforgeCostSummary && runewordReforgeCostSummary.textContent !== actionState.reforgeCostSummary) {
    runewordReforgeCostSummary.textContent = actionState.reforgeCostSummary;
  }
  if (runewordReforgeDetails) {
    runewordReforgeDetails.classList.toggle("muted", !actionState.hasBase);
  }
  runewordReforgeLockList.setAttribute(
    "aria-label",
    t("Select the affix to replace", "교체할 어픽스 선택")
  );

  const hadFocus = runewordReforgeLockList.contains(document.activeElement);
  const focusedToken = hadFocus && document.activeElement
    ? document.activeElement.getAttribute(reforgeLockTokenAttribute)
    : null;
  clearChildren(runewordReforgeLockList);

  if (!actionState.hasBase) {
    runewordReforgeLockList.setAttribute("aria-disabled", "true");
    appendEmptyState(
      runewordReforgeLockList,
      t("No item selected", "선택한 장비 없음"),
      t("Select an item first.", "장비를 먼저 고르세요.")
    );
    return;
  }
  runewordReforgeLockList.setAttribute("aria-disabled", "false");

  const renderedOptions = [];
  const appendOption = (token, label, accessibleLabel, selected) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = selected
      ? "cpListItem rwReforgeLockOption selected"
      : "cpListItem rwReforgeLockOption";
    button.textContent = label;
    button.title = accessibleLabel;
    button.setAttribute("role", "option");
    button.setAttribute("aria-label", accessibleLabel);
    button.setAttribute("aria-selected", selected ? "true" : "false");
    button.setAttribute(reforgeLockTokenAttribute, token);
    button.dataset.reforgeLockToken = token;
    button.tabIndex = -1;
    runewordReforgeLockList.appendChild(button);
    renderedOptions.push({ button, token, selected });
  };

  for (const candidate of actionState.reforgeLockCandidates) {
    const isPrefix = candidate.slotKind === "prefix";
    const slotEn = isPrefix ? "Prefix" : "Suffix";
    const slotKo = isPrefix ? "접두" : "접미";
    const marker = isPrefix ? "P" : "S";
    const nameEn = resolveReforgeCandidateName(candidate, "en");
    const nameKo = resolveReforgeCandidateName(candidate, "ko");
    const selected = actionState.lockedReforgeCandidate?.affixToken === candidate.affixToken;
    appendOption(
      candidate.affixToken,
      t(
        `[${marker}] ${nameEn}`,
        `[${slotKo}] ${nameKo}`
      ),
      t(
        `Replace ${slotEn} affix ${nameEn}. Reforge cost: ${actionState.lockedReforgeCost} Orbs.`,
        `${slotKo} 어픽스 ${nameKo} 교체. 재련 비용: 오브 ${actionState.lockedReforgeCost}개.`
      ),
      selected
    );
  }

  const preferredOption = (focusedToken !== null
    ? renderedOptions.find((entry) => entry.token === focusedToken)
    : null) || renderedOptions.find((entry) => entry.selected) || renderedOptions[0];
  if (preferredOption) {
    preferredOption.button.tabIndex = 0;
    if (hadFocus && typeof preferredOption.button.focus === "function") {
      try {
        preferredOption.button.focus({ preventScroll: true });
      } catch (_) {
        preferredOption.button.focus();
      }
    }
  }
}

function renderRunewordPanelState() {
  const state = runewordPanelState || {};
  const actionState = resolveRunewordPanelActionState(state);
  if (affixIdentifyButton) {
    affixIdentifyButton.disabled = !actionState.identifyEnabled;
    affixIdentifyButton.textContent = t("Identify (1 Scroll)", "확인 (스크롤 1개)");
    affixIdentifyButton.setAttribute(panelCommandAttribute, actionState.identifyCommand);
    affixIdentifyButton.title = actionState.runewordHead
      ? t("Only without regular affixes. The runeword holds the prefix slot, so this adds 1 suffix (75%) or 2 (25%).", "일반 어픽스가 없는 장비에만 사용. 룬워드가 접두 칸을 차지하므로 접미 1개(75%) 또는 2개(25%)를 부여합니다.")
      : t("Only without regular affixes. Gain 1/2/3 affixes at 60%/30%/10%.", "일반 어픽스가 없는 장비에만 사용. 1/2/3개를 60%/30%/10% 확률로 부여합니다.");
  }
  if (affixScourButton) {
    affixScourButton.disabled = !actionState.scourEnabled;
    affixScourButton.textContent = t("Scour (1 Orb)", "정제 (정제 오브 1개)");
    affixScourButton.setAttribute(panelCommandAttribute, actionState.scourCommand);
    affixScourButton.title = actionState.reforgeLimit > 0
      ? t(
        `Reroll every regular affix at once, keeping the slot count, and get the item's ${actionState.reforgeLimit} selected reforges back. Runeword preserved.`,
        `슬롯 수를 유지한 채 일반 어픽스 전부를 한 번에 다시 굴리고, 그 장비의 선택 재련 ${actionState.reforgeLimit}회를 되돌려 받습니다. 룬워드는 유지됩니다.`
      )
      : t("Reroll every regular affix at once, keeping the slot count. Runeword preserved.", "슬롯 수를 유지한 채 일반 어픽스 전부를 한 번에 다시 굴립니다. 룬워드는 유지됩니다.");
  }
  if (runewordRemoveButton) {
    runewordRemoveButton.hidden = !actionState.hasRuneword;
    runewordRemoveButton.disabled = !actionState.removeRunewordEnabled;
    runewordRemoveButton.textContent = t("Remove Runeword (1 Scouring Orb)", "룬워드 제거 (정제 오브 1개)");
    runewordRemoveButton.setAttribute(panelCommandAttribute, actionState.removeRunewordCommand);
    runewordRemoveButton.title = actionState.runewordHead
      ? t("Removes the runeword and rolls a new prefix into its slot. Suffixes and reforges left stay.", "룬워드를 지우고 그 칸에 새 접두를 굴립니다. 접미와 남은 재련 횟수는 유지됩니다.")
      : t("Removes the runeword. The prefix it sat on and the suffixes stay.", "룬워드를 지웁니다. 함께 있던 접두와 접미는 유지됩니다.");
  }
  if (resourceExchangeGroup) {
    resourceExchangeGroup.hidden = actionState.exchangeReforgeCost <= 0 && actionState.exchangeScourCost <= 0;
    resourceExchangeLabel.textContent = t("Trade Identify Scrolls (one way)", "확인 스크롤 교환 (되돌릴 수 없음)");
    for (const [button, cost, enabled, command, en, ko] of [
      [exchangeReforgeButton, actionState.exchangeReforgeCost, actionState.exchangeReforgeEnabled,
        "currency.exchange:reforge", "Reforge Orb", "재련 오브"],
      [exchangeScourButton, actionState.exchangeScourCost, actionState.exchangeScourEnabled,
        "currency.exchange:scour", "Scouring Orb", "정제 오브"]
    ]) {
      button.hidden = cost <= 0;
      button.disabled = !enabled;
      button.textContent = t(`${cost} Scrolls → 1 ${en}`, `스크롤 ${cost}개 → ${ko} 1개`);
      button.setAttribute(panelCommandAttribute, enabled ? command : "");
      button.title = t(
        `Trade ${cost} Identify Scrolls for 1 ${en}. This cannot be undone.`,
        `확인 스크롤 ${cost}개를 ${ko} 1개로 바꿉니다. 되돌릴 수 없습니다.`
      );
    }
  }
  if (runewordRecoveryDetails) {
    // Free reset is a debug tool; outside debug it could only ever be disabled.
    runewordRecoveryDetails.hidden = state.debugTools !== true;
  }
  if (debugToolsPanel) {
    // Cheat-adjacent tools stay hidden unless the runtime reports a debug
    // toggle enabled (debug HUD or verbose logging).
    debugToolsPanel.style.display = state.debugTools ? "" : "none";
  }
  const hasBase = actionState.hasBase;
  const hasRecipe = actionState.hasRecipe;
  const isComplete = actionState.isComplete;
  const requiredRunes = Array.isArray(state.requiredRunes) ? state.requiredRunes : [];
  const canTransmute = actionState.canTransmute;
  const selectedRecipe = getSelectedRecipeItem();

  renderRunewordFlowProgress(actionState, state);

  if (runewordContextRecipeName) {
    if (selectedRecipe) {
      const recipeName = resolveRecipeName(selectedRecipe) || t("Unknown", "알 수 없음");
      const runeOrder = typeof selectedRecipe?.runes === "string" ? selectedRecipe.runes.trim() : "";
      runewordContextRecipeName.textContent = runeOrder ? `${recipeName} [${runeOrder}]` : recipeName;
    } else {
      runewordContextRecipeName.textContent = t("No recipe selected", "선택된 레시피 없음");
    }
  }

  if (runewordContextRecipeMeta) {
    if (!hasBase) {
      runewordContextRecipeMeta.textContent = t("Pick an item first.", "먼저 장비를 고르세요.");
    } else if (!selectedRecipe) {
      runewordContextRecipeMeta.textContent = t(
        "Pick a recipe from the list on the left.",
        "왼쪽 목록에서 레시피를 고르세요."
      );
    } else if (state.missingSummary) {
      runewordContextRecipeMeta.textContent = `${t("Missing fragments", "부족한 룬 조각")}: ${state.missingSummary}`;
    } else if (isComplete) {
      runewordContextRecipeMeta.textContent = t(
        "This item already has a runeword. Reforging changes only its regular affixes.",
        "이 장비에는 이미 룬워드가 있습니다. 재련은 일반 어픽스만 바꿉니다."
      );
    } else if (canTransmute) {
      runewordContextRecipeMeta.textContent = t("Ready to transmute.", "변환할 수 있습니다.");
    } else {
      runewordContextRecipeMeta.textContent = t(
        "Meet the remaining requirements to transmute.",
        "남은 조건을 채우면 변환할 수 있습니다."
      );
    }
  }

  if (runewordCubeGrid) {
    clearChildren(runewordCubeGrid);
    const totalCells = 12;
    let filled = 0;

    // Parts are plain text or [en, ko] pairs. A 56px cell cannot hold both
    // languages, so it shows one (tCompact) and keeps both on hover (t).
    const cellText = (part, compact) => Array.isArray(part)
      ? (compact ? tCompact(part[0], part[1]) : t(part[0], part[1]))
      : (part || "");
    const addCell = (className, title, name, counts) => {
      if (filled >= totalCells) return;
      const cell = document.createElement("div");
      cell.className = `rwCell ${className || ""}`.trim();

      if (title) {
        const el = document.createElement("div");
        el.className = "rwCellTitle";
        el.textContent = cellText(title, true);
        cell.appendChild(el);
      }

      if (name) {
        const el = document.createElement("div");
        el.className = "rwCellName";
        el.textContent = cellText(name, true);
        cell.appendChild(el);
      }

      if (counts) {
        const el = document.createElement("div");
        el.className = "rwCellCounts";
        el.textContent = cellText(counts, true);
        cell.appendChild(el);
      }

      cell.title = [title, name, counts].map((part) => cellText(part, false)).filter(Boolean).join("\n");
      runewordCubeGrid.appendChild(cell);
      filled += 1;
    };

    addCell(
      hasBase ? "base" : "base empty",
      ["Base", "베이스"],
      hasBase ? resolveSelectedWorkingBase()?.name || ["Selected", "선택됨"] : ["None", "없음"],
      // Every selectable base is equipped; the cell's room goes to the name.
      ""
    );

    if (hasRecipe && requiredRunes.length > 0) {
      for (const req of requiredRunes) {
        const name = typeof req?.name === "string" ? req.name : "";
        const required = Number.isFinite(Number(req?.required)) ? Number(req.required) : 0;
        const owned = Number.isFinite(Number(req?.owned)) ? Number(req.owned) : 0;
        if (!name || required <= 0) continue;

        const missing = owned < required;
        addCell(
          missing ? "missing" : "ready",
          // Short enough for a 56px cell in every language; the red cell already marks it.
          missing ? ["Missing", "부족"] : ["Rune", "룬"],
          // The count line already says "Have n/required"; the name alone fits one line.
          name,
          // owned/required reads the same in every language and fits even at 3 digits.
          `${owned}/${required}`
        );
      }
    } else if (hasRecipe && requiredRunes.length === 0) {
      addCell("empty", ["Runes", "룬"], ["No data", "정보 없음"], "");
    }

    while (filled < totalCells) {
      addCell("empty", "", "", "");
    }
  }

  clearChildren(runewordPanelStatus);

  if (!hasBase) {
    runewordPanelStatus.textContent = t(
      "Select an item first.",
      "장비를 먼저 고르세요."
    );
  } else if (!hasRecipe) {
    runewordPanelStatus.textContent = t(
      "Select a runeword recipe.",
      "룬워드 레시피를 선택하세요."
    );
  } else {
    const inserted = Number(state.insertedRunes) || 0;
    const total = Number(state.totalRunes) || 0;

    const header = document.createElement("div");
    header.className = "rwStatusHeader";

    // The recipe name already heads the panel ("Selected Recipe"), so this
    // row carries the status sentence next to its badge instead.
    const left = document.createElement("div");

    const badge = document.createElement("div");
    let badgeClass = "rwBadge";
    let badgeText = "";
    if (isComplete) {
      badgeClass += " complete";
      badgeText = t("Complete", "완료");
    } else if (actionState.baseCompatibilityWarning) {
      badgeClass += " warning";
      badgeText = t("Base Mismatch", "베이스 불일치");
    } else if (canTransmute) {
      badgeClass += " ready";
      badgeText = t("Ready", "가능");
    } else {
      badgeClass += " missing";
      badgeText = t("Missing", "부족");
    }
    badge.className = badgeClass;
    badge.textContent = badgeText;

    if (isComplete) {
      left.textContent = t("This item already has a runeword.", "이 장비에는 이미 룬워드가 있습니다.");
    } else if (canTransmute) {
      left.textContent = t("Ready to transmute.", "변환할 수 있습니다.");
    } else if (state.missingSummary) {
      left.textContent = `${t("Missing", "부족")}: ${state.missingSummary}`;
    }

    header.appendChild(left);
    header.appendChild(badge);
    runewordPanelStatus.appendChild(header);

    if (selectedRecipe) {
      const baseBadge = resolveRecipeBaseBadge(selectedRecipe);

      const baseLine = document.createElement("div");
      baseLine.className = "rwMetaLine";
      // The badge text carries its own "Base:" label for the recipe list.
      const baseTypeText = baseBadge.text.replace(/(Base|베이스):\s*/g, "");
      baseLine.textContent = `${t("Recommended Base", "권장 베이스")}: ${baseTypeText}`;
      runewordPanelStatus.appendChild(baseLine);

      if (actionState.baseCompatibilityWarning) {
        const warningLine = document.createElement("div");
        warningLine.className = "rwMetaLine warning";
        warningLine.textContent = actionState.baseCompatibilityMessage;
        runewordPanelStatus.appendChild(warningLine);
      }

      const runeSequence = typeof selectedRecipe?.runes === "string" ? selectedRecipe.runes : "";
      if (runeSequence.trim()) {
        const runeLine = document.createElement("div");
        runeLine.className = "rwMetaLine";
        runeLine.textContent = `${t("Rune Order", "룬 순서")}: ${runeSequence}`;
        runewordPanelStatus.appendChild(runeLine);
      }

    }

    if (total > 0 && !isComplete) {
      const progress = document.createElement("div");
      progress.className = "rwMetaLine";
      progress.textContent = `${t("Progress", "진행도")}: ${inserted}/${total}`;
      runewordPanelStatus.appendChild(progress);
    }
  }

  runewordInsertButton.textContent = actionState.buttonLabel;
  runewordInsertButton.disabled = !canTransmute;
  runewordInsertButton.title = actionState.buttonHint;
  runewordInsertButton.setAttribute("aria-label", actionState.buttonHint);

  if (runewordActionHint) {
    runewordActionHint.textContent = actionState.buttonHint;
  }

  renderAffixSlotProgress(actionState);
  renderReforgeLockOptions(actionState);

  if (runewordReforgeButton) {
    runewordReforgeButton.disabled = !actionState.reforgeEnabled;
    runewordReforgeButton.textContent = actionState.reforgeButtonLabel;
    runewordReforgeButton.setAttribute(panelCommandAttribute, actionState.reforgeCommand);
    runewordReforgeButton.title = actionState.reforgeHint;
    runewordReforgeButton.setAttribute("aria-label", actionState.reforgeHint);
  }
  if (runewordResetButton) {
    runewordResetButton.disabled = !actionState.resetEnabled;
    if (!actionState.resetEnabled || Date.now() >= runewordResetArmedUntil) {
      runewordResetArmedUntil = 0;
      runewordResetButton.classList.remove("armed");
      runewordResetButton.textContent = t("Reset Selected Item", "선택한 장비 초기화");
      runewordResetButton.title = actionState.resetHint;
      runewordResetButton.setAttribute("aria-label", actionState.resetHint);
    }
  }
}
