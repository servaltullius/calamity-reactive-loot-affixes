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
  if (regularAffixCount < 2 || !Array.isArray(state?.reforgeLockCandidates)) {
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
    : "runeword.reforge";
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
        "Select a working base to view and expand its regular affix slots.",
        "작업 베이스를 선택하면 일반 어픽스 슬롯을 확인하고 확장할 수 있습니다."
      );
    case "requires_first_affix":
      return t(
        "Create the first affix with standard reforge before expanding slots.",
        "기본 재련으로 첫 어픽스를 만든 뒤 슬롯을 확장할 수 있습니다."
      );
    case "max_slots":
      return t(
        "All regular affix slots are unlocked.",
        "모든 일반 어픽스 슬롯이 열렸습니다."
      );
    case "invalid_layout":
      return t(
        "This base has an affix layout that cannot be expanded.",
        "현재 베이스의 어픽스 구성은 확장할 수 없습니다."
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
  const regularAffixCount = regularAffixCountKnown ? state.regularAffixCount : 0;
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
  const pending = Boolean(pendingState);
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
    t("Prefix", "접두"),
    t("Suffix 1", "접미 1"),
    t("Suffix 2", "접미 2")
  ];
  const ariaValueText = !hasValidBase
    ? t("No base selected", "선택된 베이스 없음")
    : displayCount === null
      ? t("Regular affix slots are synchronizing", "일반 어픽스 슬롯 동기화 중")
      : t(
          `${displayCount} of ${maxRegularAffixCount} regular affix slots active`,
          `일반 어픽스 슬롯 ${maxRegularAffixCount}개 중 ${displayCount}개 활성`
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
      expandButtonLabel = t("Select Base", "베이스 선택 필요");
    } else if (unavailableReason === "requires_first_affix") {
      expandButtonLabel = t("Create First Affix First", "첫 어픽스 먼저 생성");
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
      "Affix slot expansion response timed out. Check the current base state before trying again.",
      "어픽스 슬롯 확장 응답 시간이 초과되었습니다. 다시 시도하기 전에 현재 베이스 상태를 확인하세요."
    ));
    schedulePanelRender(panelRenderSection.runewordPanelState);
  }, affixExpandPendingTimeoutMs);
  return true;
}

function resolveRunewordPanelActionState(state) {
  const hasBase = Boolean(state.hasBase);
  const hasRecipe = Boolean(state.hasRecipe);
  const isComplete = Boolean(state.isComplete);
  const affixExpandPending = Boolean(affixExpandPendingState);
  const canTransmute = Boolean(state.canInsert) &&
    hasBase &&
    hasRecipe &&
    !isComplete &&
    !affixExpandPending;
  const baseCompatibilityWarning = Boolean(state.baseCompatibilityWarning);
  const baseCompatibilityMessage = baseCompatibilityWarning
    ? t(
        typeof state.baseCompatibilityMessageEn === "string" ? state.baseCompatibilityMessageEn : "Selected base mismatch.",
        typeof state.baseCompatibilityMessageKo === "string" ? state.baseCompatibilityMessageKo : "선택한 베이스가 권장 타입과 다릅니다."
      )
    : "";

  let buttonLabel = t("Transmute", "변환");
  let buttonHint = "";

  if (isComplete) {
    buttonLabel = t("Complete", "완료");
    buttonHint = t(
      "This base already has a completed runeword.",
      "이 베이스에는 이미 룬워드가 완성되어 있습니다."
    );
  } else if (!hasBase) {
    buttonHint = t(
      "Select an equipped base first.",
      "착용 베이스를 먼저 선택하세요."
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
      "Fragments are missing — check the rune list above.",
      "룬조각이 부족합니다 — 위 룬 목록을 확인하세요."
    );
  } else if (!canTransmute) {
    buttonHint = t(
      "Transmute is not available yet.",
      "아직 변환할 수 없습니다."
    );
  } else {
    buttonHint = t(
      "Transmute consumes all required fragments and applies the runeword.",
      "변환 시 필요한 룬조각을 모두 소모하고 룬워드를 적용합니다."
    );
  }

  if (baseCompatibilityMessage) {
    buttonHint = buttonHint
      ? `${baseCompatibilityMessage}
${buttonHint}`
      : baseCompatibilityMessage;
  }

  const regularAffixCount = Number.isFinite(Number(state.regularAffixCount))
    ? Math.max(0, Math.trunc(Number(state.regularAffixCount)))
    : 0;
  const standardReforgeCost = Number.isFinite(Number(state.standardReforgeCost)) && Number(state.standardReforgeCost) > 0
    ? Math.trunc(Number(state.standardReforgeCost))
    : 1;
  const lockedReforgeCost = Number.isFinite(Number(state.lockedReforgeCost)) && Number(state.lockedReforgeCost) > 0
    ? Math.trunc(Number(state.lockedReforgeCost))
    : 2;
  const hasKnownReforgeOrbCount = state.reforgeOrbsOwned !== null &&
    state.reforgeOrbsOwned !== undefined &&
    Number.isFinite(Number(state.reforgeOrbsOwned)) &&
    Number(state.reforgeOrbsOwned) >= 0;
  const reforgeOrbsOwned = hasKnownReforgeOrbCount
    ? Math.trunc(Number(state.reforgeOrbsOwned))
    : null;
  const affixSlotState = resolveAffixSlotProgressState(state);
  const reforgeLockCandidates = resolveReforgeLockCandidates(state);
  const lockedReforgeCandidate = resolveActiveReforgeLockCandidate(state);
  const reforgeCost = lockedReforgeCandidate ? lockedReforgeCost : standardReforgeCost;
  const hasEnoughReforgeOrbs = reforgeOrbsOwned === null || reforgeOrbsOwned >= reforgeCost;
  const reforgeEnabled = hasBase && hasEnoughReforgeOrbs && !affixSlotState.expandPending;
  const reforgeCommand = buildReforgeCommand(state);

  const lockedNameEn = lockedReforgeCandidate
    ? resolveReforgeCandidateName(lockedReforgeCandidate, "en")
    : "";
  const lockedNameKo = lockedReforgeCandidate
    ? resolveReforgeCandidateName(lockedReforgeCandidate, "ko")
    : "";
  let reforgeHint = "";
  if (!hasBase) {
    reforgeHint = t("Select a base first.", "베이스를 먼저 선택하세요.");
  } else if (lockedReforgeCandidate) {
    reforgeHint = t(
      `Consume ${reforgeCost} Reforge Orbs, keep ${lockedNameEn}, and reroll the remaining regular affixes. The current regular-affix count and any completed runeword are preserved.`,
      `재련 오브 ${reforgeCost}개를 소모해 ${lockedNameKo} 어픽스를 유지하고 나머지 일반 어픽스를 재굴림합니다. 현재 일반 어픽스 개수와 완성된 룬워드는 유지됩니다.`
    );
  } else if (isComplete) {
    reforgeHint = t(
      `Consume ${reforgeCost} Reforge Orb${reforgeCost === 1 ? "" : "s"} and reroll only the regular affixes on the selected base. The completed runeword and current regular-affix count are preserved; a base with none gains one.`,
      `재련 오브 ${reforgeCost}개를 소모해 선택 베이스의 일반 어픽스만 재굴림합니다. 완성된 룬워드는 유지됩니다. 현재 일반 어픽스 개수도 유지되며, 없으면 1개가 생깁니다.`
    );
  } else {
    reforgeHint = t(
      `Consume ${reforgeCost} Reforge Orb${reforgeCost === 1 ? "" : "s"} and reroll the same number of regular affixes on the selected base. A base with none gains one; any completed runeword is preserved.`,
      `재련 오브 ${reforgeCost}개를 소모해 선택 베이스의 일반 어픽스를 같은 개수로 재굴림합니다. 일반 어픽스가 없으면 1개가 생기며, 완성된 룬워드는 유지됩니다.`
    );
  }

  if (hasBase && reforgeOrbsOwned !== null) {
    const inventoryLine = t(
      `Reforge Orbs owned: ${reforgeOrbsOwned}.`,
      `보유 재련 오브: ${reforgeOrbsOwned}개.`
    );
    reforgeHint = `${reforgeHint}\n${inventoryLine}`;
    if (!hasEnoughReforgeOrbs) {
      const shortfall = reforgeCost - reforgeOrbsOwned;
      reforgeHint = `${reforgeHint}\n${t(
        `You need ${shortfall} more Reforge Orb${shortfall === 1 ? "" : "s"}.`,
        `재련 오브가 ${shortfall}개 더 필요합니다.`
      )}`;
    }
  }

  const reforgeButtonLabel = lockedReforgeCandidate
    ? t(
        `Protect & Reforge (${reforgeCost} Orb${reforgeCost === 1 ? "" : "s"})`,
        `잠금 재련 (오브 ${reforgeCost}개)`
      )
    : affixSlotState.regularAffixCountKnown && affixSlotState.regularAffixCount === 0
      ? t(
          `Create First Affix (${reforgeCost} Orb${reforgeCost === 1 ? "" : "s"})`,
          `첫 어픽스 생성 (오브 ${reforgeCost}개)`
        )
    : t(
        `Reforge (${reforgeCost} Orb${reforgeCost === 1 ? "" : "s"})`,
        `재련 (오브 ${reforgeCost}개)`
      );
  const reforgeSummary = lockedReforgeCandidate
    ? t(
        `Protected: ${lockedNameEn} · ${reforgeCost} Orbs`,
        `잠금: ${lockedNameKo} · 오브 ${reforgeCost}개`
      )
    : t(
        `Reforge: reroll all · ${reforgeCost} Orb${reforgeCost === 1 ? "" : "s"}`,
        `재련: 모두 재굴림 · 오브 ${reforgeCost}개`
      );

  let reforgeLockHint = "";
  if (!hasBase) {
    reforgeLockHint = t(
      "Select an equipped base to configure affix protection.",
      "어픽스 잠금을 설정하려면 착용 베이스를 선택하세요."
    );
  } else if (regularAffixCount === 0) {
    reforgeLockHint = t(
      "This base has no regular affix yet. Standard reforge adds one; there is nothing to lock.",
      "이 베이스에는 아직 일반 어픽스가 없습니다. 기본 재련으로 1개가 생기며, 잠글 대상은 없습니다."
    );
  } else if (regularAffixCount === 1) {
    reforgeLockHint = t(
      "At least two regular affixes are needed to lock one. Standard reforge rerolls the current affix.",
      "어픽스 하나를 잠그려면 일반 어픽스가 최소 2개 필요합니다. 기본 재련은 현재 어픽스를 재굴림합니다."
    );
  } else if (reforgeLockCandidates.length === 0) {
    reforgeLockHint = t(
      "No eligible regular affix can be locked. Standard reforge remains available.",
      "잠글 수 있는 일반 어픽스가 없습니다. 기본 재련은 계속 사용할 수 있습니다."
    );
  } else {
    reforgeLockHint = t(
      "Only regular affixes can be locked. A completed runeword is preserved automatically and is never a lock candidate.",
      "일반 어픽스만 잠글 수 있습니다. 완성된 룬워드는 자동으로 유지되며 잠금 후보에서 제외됩니다."
    );
  }

  let reforgeCostSummary = hasBase
    ? t(
        `Cost: ${reforgeCost} Reforge Orb${reforgeCost === 1 ? "" : "s"}`,
        `비용: 재련 오브 ${reforgeCost}개`
      )
    : t("Select a base to see the cost.", "비용을 확인하려면 베이스를 선택하세요.");
  if (hasBase && reforgeOrbsOwned !== null) {
    reforgeCostSummary += t(
      ` · Owned: ${reforgeOrbsOwned}`,
      ` · 보유: ${reforgeOrbsOwned}개`
    );
    if (!hasEnoughReforgeOrbs) {
      const shortfall = reforgeCost - reforgeOrbsOwned;
      reforgeCostSummary += t(
        ` · Need ${shortfall} more`,
        ` · ${shortfall}개 부족`
      );
    }
  }

  const resetEnabled = hasBase && !affixSlotState.expandPending;
  const resetHint = resetEnabled ?
    t(
      "Remove all Calamity affixes, runeword progress, and instance state from the selected base. No material refund.",
      "선택 베이스의 모든 Calamity 어픽스, 룬워드 진행도, 인스턴스 상태를 제거합니다. 재료는 환불되지 않습니다."
    ) :
    t(
      "Select a base first.",
      "베이스를 먼저 선택하세요."
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
    reforgeEnabled,
    reforgeHint,
    reforgeButtonLabel,
    reforgeSummary,
    reforgeLockHint,
    reforgeCostSummary,
    reforgeCommand,
    reforgeCost,
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
    runewordFlowHint.textContent = t(
      "Start by selecting one equipped base.",
      "먼저 착용 중인 베이스 아이템 하나를 선택하세요."
    );
    return;
  }

  if (!hasRecipe) {
    runewordFlowHint.textContent = t(
      "Base locked in. Now use the center recipe explorer to find the runeword that fits it.",
      "베이스를 골랐습니다. 이제 중앙 레시피 탐색기에서 어울리는 룬워드를 찾으세요."
    );
    return;
  }

  if (actionState.baseCompatibilityWarning) {
    runewordFlowHint.textContent = actionState.baseCompatibilityMessage;
    return;
  }

  if (isComplete) {
    runewordFlowHint.textContent = t(
      "This base already has a completed runeword. Reforge rerolls only its regular affixes; the runeword stays.",
      "이 베이스에는 이미 룬워드가 완성되어 있습니다. 재련해도 일반 어픽스만 바뀌고 룬워드는 유지됩니다."
    );
    return;
  }

  if (canTransmute) {
    runewordFlowHint.textContent = t(
      "Everything is ready. Use the review area to transmute and apply the runeword.",
      "준비가 끝났습니다. 검토 영역에서 변환을 눌러 룬워드를 적용하세요."
    );
    return;
  }

  if (state?.missingSummary) {
    runewordFlowHint.textContent = t(
      "Fragments are missing — details in step 3 on the right.",
      "룬조각이 부족합니다 — 상세는 우측 3단계에서 확인하세요."
    );
    return;
  }

  runewordFlowHint.textContent = t(
    "Review the selected recipe on the right, then transmute when available.",
    "오른쪽 검토 영역에서 선택한 레시피를 확인한 뒤, 가능해지면 변환하세요."
  );
}

function renderAffixSlotProgress(actionState) {
  const slotState = actionState?.affixSlotState;
  if (!slotState || !affixSlotProgress || !affixSlotProgressTrack) {
    return;
  }

  if (affixSlotProgressTitle) {
    affixSlotProgressTitle.textContent = t("Regular Affix Slots", "일반 어픽스 슬롯");
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
    t("Affix protection for reforge", "재련 어픽스 잠금")
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
      t("No base selected", "선택된 베이스 없음"),
      t(
        "Select an equipped base before configuring affix protection.",
        "어픽스 잠금을 설정하기 전에 착용 베이스를 선택하세요."
      )
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

  const noLockSelected = !actionState.lockedReforgeCandidate;
  appendOption(
    "",
    t(
      `No lock — reroll all · ${actionState.standardReforgeCost} Orb${actionState.standardReforgeCost === 1 ? "" : "s"}`,
      `잠금 없음 — 모두 재굴림 · 오브 ${actionState.standardReforgeCost}개`
    ),
    t(
      `No affix protection. Reroll all regular affixes for ${actionState.standardReforgeCost} Reforge Orb${actionState.standardReforgeCost === 1 ? "" : "s"}.`,
      `어픽스를 잠그지 않습니다. 재련 오브 ${actionState.standardReforgeCost}개로 모든 일반 어픽스를 재굴림합니다.`
    ),
    noLockSelected
  );

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
        `[${marker}] ${nameEn} · ${actionState.lockedReforgeCost} Orbs`,
        `[${slotKo}] ${nameKo} · 오브 ${actionState.lockedReforgeCost}개`
      ),
      t(
        `Lock ${slotEn} affix ${nameEn}. Reforge cost: ${actionState.lockedReforgeCost} Orbs.`,
        `${slotKo} 어픽스 ${nameKo} 잠금. 재련 비용: 오브 ${actionState.lockedReforgeCost}개.`
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
      const recipeName = typeof selectedRecipe?.name === "string" ? selectedRecipe.name : t("Unknown", "알 수 없음");
      const runeOrder = typeof selectedRecipe?.runes === "string" ? selectedRecipe.runes.trim() : "";
      runewordContextRecipeName.textContent = runeOrder ? `${recipeName} [${runeOrder}]` : recipeName;
    } else {
      runewordContextRecipeName.textContent = t("No recipe selected", "선택된 레시피 없음");
    }
  }

  if (runewordContextRecipeMeta) {
    if (!hasBase) {
      runewordContextRecipeMeta.textContent = t(
        "Pick a base first so the recipe explorer has a stable context.",
        "먼저 베이스를 골라야 레시피 탐색기를 안정적으로 사용할 수 있습니다."
      );
    } else if (!selectedRecipe) {
      runewordContextRecipeMeta.textContent = t(
        "Search the center explorer and select the recipe you want to review.",
        "중앙 탐색기에서 검토할 레시피를 선택하세요."
      );
    } else if (actionState.baseCompatibilityWarning) {
      runewordContextRecipeMeta.textContent = actionState.baseCompatibilityMessage;
    } else if (state.missingSummary) {
      runewordContextRecipeMeta.textContent = `${t("Missing fragments", "부족한 룬조각")}: ${state.missingSummary}`;
    } else if (isComplete) {
      runewordContextRecipeMeta.textContent = t(
        "This base already has a completed runeword. Reforge changes only its regular affixes.",
        "이 베이스에는 이미 룬워드가 완성되어 있습니다. 재련은 일반 어픽스만 변경합니다."
      );
    } else if (canTransmute) {
      runewordContextRecipeMeta.textContent = t(
        "Everything is ready. Review the details and transmute when you are ready.",
        "준비가 끝났습니다. 세부 정보를 확인한 뒤 변환하세요."
      );
    } else {
      runewordContextRecipeMeta.textContent = t(
        "Review the selected recipe and finish the missing requirements.",
        "선택한 레시피를 검토하고 남은 요구 조건을 채우세요."
      );
    }
  }

  if (runewordCubeGrid) {
    clearChildren(runewordCubeGrid);
    const totalCells = 12;
    let filled = 0;

    const addCell = (className, title, name, counts) => {
      if (filled >= totalCells) return;
      const cell = document.createElement("div");
      cell.className = `rwCell ${className || ""}`.trim();

      if (title) {
        const el = document.createElement("div");
        el.className = "rwCellTitle";
        el.textContent = title;
        cell.appendChild(el);
      }

      if (name) {
        const el = document.createElement("div");
        el.className = "rwCellName";
        el.textContent = name;
        cell.appendChild(el);
      }

      if (counts) {
        const el = document.createElement("div");
        el.className = "rwCellCounts";
        el.textContent = counts;
        cell.appendChild(el);
      }

      runewordCubeGrid.appendChild(cell);
      filled += 1;
    };

    addCell(
      hasBase ? "base" : "base empty",
      t("Base", "베이스"),
      hasBase ? resolveSelectedWorkingBase()?.name || t("Selected", "선택됨") : t("None", "없음"),
      hasBase ? t("Equipped", "착용") : ""
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
          missing ? t("Rune (Missing)", "룬(부족)") : t("Rune", "룬"),
          `${name} x${required}`,
          `${t("Owned", "보유")}: ${owned}/${required}`
        );
      }
    } else if (hasRecipe && requiredRunes.length === 0) {
      addCell("empty", t("Runes", "룬"), t("No data", "정보 없음"), "");
    }

    while (filled < totalCells) {
      addCell("empty", "", "", "");
    }
  }

  clearChildren(runewordPanelStatus);

  if (!hasBase) {
    runewordPanelStatus.textContent = t(
      "Select an equipped base first.",
      "착용 베이스를 먼저 선택하세요."
    );
  } else if (!hasRecipe) {
    runewordPanelStatus.textContent = t(
      "Select a runeword recipe.",
      "룬워드 레시피를 선택하세요."
    );
  } else {
    const recipeName = state.recipeName || t("Unknown", "알 수 없음");
    const inserted = Number(state.insertedRunes) || 0;
    const total = Number(state.totalRunes) || 0;

    const header = document.createElement("div");
    header.className = "rwStatusHeader";

    const left = document.createElement("div");
    left.textContent = `${t("Recipe", "레시피")}: ${recipeName}`;

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

    header.appendChild(left);
    header.appendChild(badge);
    runewordPanelStatus.appendChild(header);

    const meta = document.createElement("div");
    meta.className = "rwMetaLine";
    if (isComplete) {
      meta.textContent = t(
        "This base already has a runeword.",
        "이 베이스에는 이미 룬워드가 적용되어 있습니다."
      );
    } else if (canTransmute) {
      meta.textContent = t(
        "Transmute will consume all required fragments.",
        "변환 시 필요한 룬조각을 모두 소모합니다."
      );
    } else if (state.missingSummary) {
      meta.textContent = `${t("Missing", "부족")}: ${state.missingSummary}`;
    }

    if (meta.textContent) {
      runewordPanelStatus.appendChild(meta);
    }

    if (selectedRecipe) {
      const baseBadge = resolveRecipeBaseBadge(selectedRecipe);

      const baseLine = document.createElement("div");
      baseLine.className = "rwMetaLine";
      baseLine.textContent = `${t("Recommended Base", "권장 베이스")}: ${baseBadge.text}`;
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
      runewordResetButton.textContent = t("Reset Selected Base", "선택 베이스 초기화");
      runewordResetButton.title = actionState.resetHint;
      runewordResetButton.setAttribute("aria-label", actionState.resetHint);
    }
  }
}
