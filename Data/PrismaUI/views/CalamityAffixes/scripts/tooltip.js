function getDefaultTooltipLayout() {
  if (window.innerWidth <= 900) {
    return { ...tooltipDefaultsMobile };
  }

  const ratio = getTooltipViewportRatio();
  return {
    right: Math.max(8, Math.round(tooltipReferenceDesktop.right * ratio)),
    top: Math.max(8, Math.round(tooltipReferenceDesktop.top * ratio)),
    fontScale: tooltipReferenceDesktop.fontScale
  };
}

function tooltipBounds() {
  const margin = 8;
  const maxRight = Math.max(margin, window.innerWidth - 80);
  const maxTop = Math.max(margin, window.innerHeight - 80);
  return { margin, maxRight, maxTop };
}

function normalizeTooltipLayout(raw) {
  const defaults = getDefaultTooltipLayout();
  const { margin, maxRight, maxTop } = tooltipBounds();
  const source = raw && typeof raw === "object" ? raw : {};

  let right = Number(source.right);
  if (!Number.isFinite(right)) {
    right = defaults.right;
  }

  let top = Number(source.top);
  if (!Number.isFinite(top)) {
    top = defaults.top;
  }

  let fontScale = Number(source.fontScale);
  if (!Number.isFinite(fontScale)) {
    const permille = Number(source.fontPermille);
    if (Number.isFinite(permille) && permille > 0) {
      fontScale = permille / 1000;
    }
  }
  if (!Number.isFinite(fontScale)) {
    fontScale = defaults.fontScale;
  }

  return {
    right: clamp(Math.round(right), margin, maxRight),
    top: clamp(Math.round(top), margin, maxTop),
    fontScale: clamp(fontScale, 0.7, 1.8)
  };
}

function getTooltipMaxLogicalHeight() {
  const availableVisualHeight = Math.max(120, window.innerHeight - 16);
  return Math.max(
    120,
    Math.min(560, Math.floor(availableVisualHeight / getTooltipAutoScale()))
  );
}

function fitTooltipLayoutToViewport(rawLayout, rect) {
  const normalized = normalizeTooltipLayout(rawLayout);
  const margin = 8;
  const rectWidth = Math.max(0, Number(rect?.width) || 0);
  const rectHeight = Math.max(0, Number(rect?.height) || 0);
  if (rectWidth === 0 || rectHeight === 0) {
    return normalized;
  }

  const maxRight = Math.max(
    margin,
    Math.floor(window.innerWidth - margin - rectWidth)
  );
  const maxTop = Math.max(
    margin,
    Math.floor(window.innerHeight - margin - rectHeight)
  );
  return {
    ...normalized,
    right: clamp(normalized.right, margin, maxRight),
    top: clamp(normalized.top, margin, maxTop)
  };
}

function measureTooltipRect() {
  const computedDisplay = window.getComputedStyle(tooltipPanel).display;
  const wasHidden = computedDisplay === "none";
  const previousDisplay = tooltipPanel.style.display;
  const previousVisibility = tooltipPanel.style.visibility;
  if (wasHidden) {
    tooltipPanel.style.visibility = "hidden";
    tooltipPanel.style.display = "block";
  }

  const rect = tooltipPanel.getBoundingClientRect();
  if (wasHidden) {
    tooltipPanel.style.display = previousDisplay;
    tooltipPanel.style.visibility = previousVisibility;
  }
  return rect;
}

function setTooltipSummaryText() {
  if (!tooltipUiSummary) return;
  const percent = Math.round(tooltipLayout.fontScale * 100);
  tooltipUiSummary.textContent = t(
    `Text ${percent}% · Right ${tooltipLayout.right}px / Top ${tooltipLayout.top}px`,
    `글자 ${percent}% · 우측 ${tooltipLayout.right}px / 상단 ${tooltipLayout.top}px`
  );
}

function applyTooltipLayout() {
  tooltipLayout = normalizeTooltipLayout(tooltipLayout);
  document.documentElement.style.setProperty("--tooltip-font-scale", tooltipLayout.fontScale.toFixed(3));
  tooltipPanel.style.maxHeight = `${getTooltipMaxLogicalHeight()}px`;
  tooltipPanel.style.right = `${tooltipLayout.right}px`;
  tooltipPanel.style.top = `${tooltipLayout.top}px`;
  tooltipLayout = fitTooltipLayoutToViewport(
    tooltipLayout,
    measureTooltipRect()
  );
  tooltipPanel.style.right = `${tooltipLayout.right}px`;
  tooltipPanel.style.top = `${tooltipLayout.top}px`;
  quickLaunch.style.right = `${tooltipLayout.right}px`;
  const quickOffset = window.innerWidth <= 900 ? 25 : 35;
  const quickTop = clamp(tooltipLayout.top - quickOffset, 8, Math.max(8, window.innerHeight - 56));
  quickLaunch.style.top = `${Math.round(quickTop)}px`;
  setTooltipSummaryText();
}

function persistTooltipLayout() {
  const right = Math.round(tooltipLayout.right);
  const top = Math.round(tooltipLayout.top);
  const fontPermille = Math.round(tooltipLayout.fontScale * 1000);
  sendCommand(`ui.tooltip.save:${right},${top},${fontPermille}`);
}

function applyAndPersistTooltipLayout(feedbackMessage) {
  applyTooltipLayout();
  persistTooltipLayout();
  if (feedbackMessage) {
    setActionFeedback(feedbackMessage);
  }
}

function handleTooltipUiCommand(command) {
  if (typeof command !== "string" || !command.startsWith("ui.tooltip.")) {
    return false;
  }

  if (command === "ui.tooltip.reset") {
    tooltipLayout = normalizeTooltipLayout(getDefaultTooltipLayout());
    tooltipLayoutLoaded = true;
    applyAndPersistTooltipLayout(t("Tooltip UI reset.", "툴팁 UI를 초기화했습니다."));
    return true;
  }

  if (command.startsWith("ui.tooltip.font:")) {
    const delta = Number(command.slice("ui.tooltip.font:".length));
    if (!Number.isFinite(delta)) {
      setActionFeedback(t("Invalid tooltip font command.", "툴팁 글자 명령이 올바르지 않습니다."));
      return true;
    }

    tooltipLayout = {
      ...tooltipLayout,
      fontScale: tooltipLayout.fontScale + delta
    };
    tooltipLayoutLoaded = true;
    applyAndPersistTooltipLayout();
    setActionFeedback(
      t(
        `Tooltip text size ${Math.round(tooltipLayout.fontScale * 100)}%.`,
        `툴팁 글자 크기 ${Math.round(tooltipLayout.fontScale * 100)}%.`
      )
    );
    return true;
  }

  if (command.startsWith("ui.tooltip.nudge:")) {
    const payload = command.slice("ui.tooltip.nudge:".length);
    const [dxText = "0", dyText = "0"] = payload.split(",", 2);
    const deltaX = Number(dxText);
    const deltaY = Number(dyText);
    if (!Number.isFinite(deltaX) || !Number.isFinite(deltaY)) {
      setActionFeedback(t("Invalid tooltip position command.", "툴팁 위치 명령이 올바르지 않습니다."));
      return true;
    }

    tooltipLayout = {
      ...tooltipLayout,
      right: tooltipLayout.right - deltaX,
      top: tooltipLayout.top + deltaY
    };
    tooltipLayoutLoaded = true;
    applyAndPersistTooltipLayout();
    setActionFeedback(
      t(
        `Tooltip moved (right ${tooltipLayout.right}px, top ${tooltipLayout.top}px).`,
        `툴팁 이동됨 (우측 ${tooltipLayout.right}px, 상단 ${tooltipLayout.top}px).`
      )
    );
    return true;
  }

  setActionFeedback(t("Unknown tooltip command.", "알 수 없는 툴팁 명령입니다."));
  return true;
}

