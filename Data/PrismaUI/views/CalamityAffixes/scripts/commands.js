function focusWorkingBaseChooserOption() {
  if (!workingBaseDetails?.open || !inventoryBaseList) {
    return;
  }

  const option = inventoryBaseList.querySelector('[role="option"][aria-selected="true"]') ||
    inventoryBaseList.querySelector('[role="option"]');
  if (!option) {
    runewordBaseChooserSummary?.focus();
    return;
  }

  option.focus({ preventScroll: true });
}

function closeWorkingBaseChooser(restoreFocus = false) {
  if (!workingBaseDetails?.open) {
    return false;
  }

  workingBaseDetails.open = false;
  runewordBaseChooserSummary?.setAttribute("aria-expanded", "false");
  if (restoreFocus) {
    runewordBaseChooserSummary?.focus();
  }
  return true;
}

function handleWorkingBaseChooserToggle() {
  if (!workingBaseDetails || !runewordBaseChooserSummary) {
    return;
  }

  runewordBaseChooserSummary.setAttribute(
    "aria-expanded",
    workingBaseDetails.open ? "true" : "false"
  );
  if (workingBaseDetails.open) {
    requestAnimationFrame(focusWorkingBaseChooserOption);
  }
}

function handleWorkingBaseOutsidePointerDown(event) {
  if (
    workingBaseDetails?.open &&
    event.target &&
    !workingBaseDetails.contains(event.target)
  ) {
    closeWorkingBaseChooser(false);
  }
}

function sendCommand(command) {
  if (typeof window.calamityCommand === "function") {
    window.calamityCommand(command);
    return;
  }

  setActionFeedback(t("Command bridge unavailable.", "명령 브리지가 연결되지 않았습니다."));
}

function handlePanelKeydown(event) {
  if (!controlPanelOpen) return;
  const key = event.key;
  const code = event.code;
  const keyCode = event.keyCode || event.which;
  const isEscape =
    key === "Escape" ||
    key === "Esc" ||
    code === "Escape" ||
    keyCode === 27;
  if (isEscape) {
    event.preventDefault();
    event.stopPropagation();
    if (closeWorkingBaseChooser(true)) {
      return;
    }
    sendCommand("ui.close");
  }
}

function shouldInvalidatePreviewForCommand(command) {
  if (previewInvalidatingCommands.has(command)) {
    return true;
  }

  return previewInvalidatingCommandPrefixes.some((prefix) => command.startsWith(prefix));
}

function clearRunewordResetConfirmation() {
  runewordResetArmedUntil = 0;
  runewordResetArmedBaseKey = "";
  if (!runewordResetButton) {
    return;
  }
  runewordResetButton.classList.remove("armed");
  runewordResetButton.textContent = t("Reset Selected Base", "선택 베이스 초기화");
  const actionState = resolveRunewordPanelActionState(runewordPanelState);
  runewordResetButton.title = actionState.resetHint;
  runewordResetButton.setAttribute("aria-label", actionState.resetHint);
}

function resolveSelectedRunewordBaseKey() {
  const selected = inventoryItemsState.find((item) => item && item.selected);
  return selected && typeof selected.key === "string" ? selected.key : "";
}

function armRunewordResetConfirmation(button) {
  const now = Date.now();
  const selectedBaseKey = resolveSelectedRunewordBaseKey();
  if (runewordResetArmedUntil > now) {
    const confirmsSameBase =
      selectedBaseKey.length > 0 && selectedBaseKey === runewordResetArmedBaseKey;
    clearRunewordResetConfirmation();
    if (confirmsSameBase) {
      return false;
    }
  }

  if (!selectedBaseKey) {
    setActionFeedback(t(
      "Select the equipped base again before resetting it.",
      "초기화할 착용 베이스를 다시 선택하세요."
    ));
    return true;
  }

  runewordResetArmedUntil = now + runewordResetConfirmWindowMs;
  runewordResetArmedBaseKey = selectedBaseKey;
  button.classList.add("armed");
  button.textContent = t("Confirm Full Reset", "전체 초기화 확인");
  const warning = t(
    "Click again within 6 seconds. All affixes and runeword progress will be removed; materials are not refunded.",
    "6초 안에 다시 누르세요. 모든 어픽스와 룬워드 진행도가 삭제되며 재료는 환불되지 않습니다."
  );
  button.title = warning;
  button.setAttribute("aria-label", warning);
  setActionFeedback(warning);
  window.setTimeout(() => {
    if (runewordResetArmedUntil !== 0 && Date.now() >= runewordResetArmedUntil) {
      clearRunewordResetConfirmation();
    }
  }, runewordResetConfirmWindowMs + 50);
  return true;
}

function resolvePreviewPendingStateForCommand(command) {
  if (previewInvalidatingCommandPrefixes.some((prefix) => command.startsWith(prefix))) {
    return true;
  }

  return Boolean(selectedItemNameState);
}

function resolveListboxNavigationIndex(currentIndex, key, itemCount) {
  if (itemCount <= 0 || currentIndex < 0 || currentIndex >= itemCount) {
    return -1;
  }
  switch (key) {
    case "ArrowDown":
    case "ArrowRight":
      return Math.min(itemCount - 1, currentIndex + 1);
    case "ArrowUp":
    case "ArrowLeft":
      return Math.max(0, currentIndex - 1);
    case "Home":
      return 0;
    case "End":
      return itemCount - 1;
    default:
      return -1;
  }
}

function handleListboxKeydown(event) {
  if (!(event.target instanceof Element)) {
    return;
  }

  const option = event.target.closest('[role="option"]');
  if (!option) {
    return;
  }
  const listbox = option.parentElement;
  if (listbox !== inventoryBaseList && listbox !== recipeList && listbox !== runewordReforgeLockList) {
    return;
  }

  const options = Array.from(listbox.children).filter(
    (child) => child instanceof Element &&
      child.getAttribute("role") === "option" &&
      !child.disabled
  );
  const currentIndex = options.indexOf(option);
  const nextIndex = resolveListboxNavigationIndex(
    currentIndex,
    event.key,
    options.length
  );
  if (nextIndex < 0) {
    return;
  }

  event.preventDefault();
  event.stopPropagation();
  option.tabIndex = -1;
  const nextOption = options[nextIndex];
  nextOption.tabIndex = 0;
  nextOption.focus();
  nextOption.scrollIntoView({ block: "nearest", inline: "nearest" });
}

function handleReforgeLockOptionClick(event) {
  const target = event.target;
  if (!(target instanceof Element) || !runewordReforgeLockList) {
    return false;
  }

  const option = target.closest(`[${reforgeLockTokenAttribute}]`);
  if (!option || option.disabled || !runewordReforgeLockList.contains(option)) {
    return false;
  }

  if (runewordResetArmedUntil !== 0) {
    clearRunewordResetConfirmation();
  }
  const token = option.getAttribute(reforgeLockTokenAttribute) || "";
  return selectReforgeLockCandidate(token);
}

function dispatchPanelCommand(button) {
  const command = button.getAttribute(panelCommandAttribute);
  if (!command) {
    return false;
  }

  if (handleTooltipUiCommand(command)) {
    return true;
  }

  if (command.startsWith("affix.identify:") || command.startsWith("affix.reforge:") || command.startsWith("affix.scour:")) {
    const action = resolveRunewordPanelActionState(runewordPanelState);
    const allowed = (action.identifyEnabled && command === action.identifyCommand) ||
      (action.reforgeEnabled && command === action.reforgeCommand) ||
      (action.scourEnabled && command === action.scourCommand);
    if (!allowed || affixCraftPendingState) return true;
    if (command.startsWith("affix.scour:")) {
      const signature = `${command}:${resolveReforgeLockCandidates(runewordPanelState).map(x => x.affixToken).join(",")}`;
      if (!scourConfirmation || scourConfirmation.signature !== signature || Date.now() > scourConfirmation.until) {
        scourConfirmation = { signature, until: Date.now() + 6000 };
        setActionFeedback(t(
          "Click Scour again within 6 seconds to reroll every regular affix at once. Slot count and runeword stay; current affixes and their growth are lost.",
          "6초 안에 정제를 다시 누르면 일반 어픽스 전부를 한 번에 다시 굴립니다. 슬롯 수와 룬워드는 유지되며, 현재 어픽스와 그 성장 상태는 사라집니다."
        ));
        return true;
      }
    }
    scourConfirmation = null;
    const pending = { command };
    affixCraftPendingState = pending;
    schedulePanelRender(panelRenderSection.runewordPanelState);
    window.setTimeout(() => {
      if (affixCraftPendingState !== pending) return;
      affixCraftPendingState = null;
      setActionFeedback(t("Crafting response timed out. Refresh the base before retrying.", "제작 응답 시간이 초과되었습니다. 베이스를 다시 확인하세요."));
      schedulePanelRender(panelRenderSection.runewordPanelState);
    }, 8000);
  } else {
    scourConfirmation = null;
  }

  if (command === "runeword.reset") {
    if (armRunewordResetConfirmation(button)) {
      return true;
    }
  } else if (runewordResetArmedUntil !== 0) {
    clearRunewordResetConfirmation();
  }

  const openTab = button.getAttribute(panelOpenTabAttribute);
  if (openTab) {
    pendingOpenMainTab = openTab;
  }

  if (command.startsWith(recipeSelectionCommandPrefix)) {
    beginOptimisticRecipeSelection(
      command.slice(recipeSelectionCommandPrefix.length)
    );
  }

  if (command.startsWith(affixExpandCommandPrefix)) {
    if (!beginAffixExpandPending(command)) {
      if (!affixExpandPendingState) {
        setActionFeedback(t(
          "The working base state changed. Review it before expanding a slot.",
          "작업 베이스 상태가 바뀌었습니다. 슬롯을 확장하기 전에 다시 확인하세요."
        ));
        schedulePanelRender(panelRenderSection.runewordPanelState);
      }
      return true;
    }
  }

  if (shouldInvalidatePreviewForCommand(command)) {
    invalidateRunewordAffixPreview(resolvePreviewPendingStateForCommand(command));
  }

  const selectedWorkingBaseOption =
    workingBaseDetails?.open && inventoryBaseList?.contains(button);
  sendCommand(command);
  if (selectedWorkingBaseOption) {
    closeWorkingBaseChooser(true);
  }
  return true;
}

function handleDelegatedPanelCommandClick(event) {
  if (handleReforgeLockOptionClick(event)) {
    event.preventDefault();
    return;
  }

  const target = event.target;
  if (!(target instanceof Element)) {
    return;
  }

  const button = target.closest(`[${panelCommandAttribute}]`);
  if (!button || button.disabled) {
    return;
  }

  if (dispatchPanelCommand(button)) {
    event.preventDefault();
  }
}
