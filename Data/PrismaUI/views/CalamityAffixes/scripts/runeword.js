function resolveRunewordPanelActionState(state) {
  const hasBase = Boolean(state.hasBase);
  const hasRecipe = Boolean(state.hasRecipe);
  const isComplete = Boolean(state.isComplete);
  const canTransmute = Boolean(state.canInsert) && hasBase && hasRecipe && !isComplete;
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
  } else if (!canTransmute && state.missingSummary) {
    buttonHint = `${t("Missing fragments", "부족한 룬조각")}: ${state.missingSummary}`;
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

  const reforgeEnabled = hasBase;
  const reforgeHint = reforgeEnabled ?
    (isComplete ?
      t(
        "Consume 1 Reforge Orb and reroll only the regular affixes on the selected base. The completed runeword is preserved.",
        "재련 오브 1개를 소모해 선택 베이스의 일반 어픽스만 재굴림합니다. 완성된 룬워드는 유지됩니다."
      ) :
      t(
        "Consume 1 Reforge Orb and reroll affixes on the selected base.",
        "재련 오브 1개를 소모해 선택 베이스의 어픽스를 재굴림합니다."
      )) :
    t(
      "Select a base first.",
      "베이스를 먼저 선택하세요."
    );

  const resetEnabled = hasBase;
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
    runewordFlowHint.textContent = `${t("Missing fragments", "부족한 룬조각")}: ${state.missingSummary}`;
    return;
  }

  runewordFlowHint.textContent = t(
    "Review the selected recipe on the right, then transmute when available.",
    "오른쪽 검토 영역에서 선택한 레시피를 확인한 뒤, 가능해지면 변환하세요."
  );
}

function renderRunewordPanelState() {
  const state = runewordPanelState || {};
  const actionState = resolveRunewordPanelActionState(state);
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
      hasBase ? selectedItemNameState || t("Selected", "선택됨") : t("None", "없음"),
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

  if (runewordReforgeButton) {
    runewordReforgeButton.disabled = !actionState.reforgeEnabled;
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
