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

// The tooltip cannot be scrolled (the item menu keeps the mouse), so it may use
// the whole screen height; text that still does not fit is shrunk a little
// (fitTooltipTextToHeight) instead of being cut off at the bottom.
// While the view is hidden or still sizing, the engine can report a window
// height of 0, which would cap the tooltip at the 120px floor until the text
// changed. Below this height the window size is not trusted: the tooltip gets
// no cap and the next frame measures again. (A safety net; the cut-off tall
// tooltip seen in game turned out to be a repaint problem, see below.)
const tooltipMinTrustedViewportHeight = 240;

function hasTrustedTooltipViewport() {
  return Number(window.innerHeight) >= tooltipMinTrustedViewportHeight;
}

function getTooltipMaxLogicalHeight() {
  const availableVisualHeight = Math.max(120, window.innerHeight - 16);
  return Math.max(120, Math.floor(availableVisualHeight / getTooltipAutoScale()));
}

const tooltipMinFitScale = 0.7;
let tooltipFitKey = "";
let tooltipFitScale = 1;
let tooltipFitRecheckPending = false;
let tooltipRepaintKey = "";

function tooltipOverflows() {
  return tooltipPanel.scrollHeight > tooltipPanel.clientHeight + 1;
}

function setTooltipFitScale(fit) {
  tooltipFitScale = fit;
  document.documentElement.style.setProperty("--tooltip-fit-scale", fit.toFixed(2));
}

// Shrinks the text only as much as needed. The same text at the same size and
// screen keeps the scale it already found: the tooltip is redrawn every few
// hundred ms, and starting over from 100% each time made it flicker.
function fitTooltipTextToHeight() {
  const key = `${tooltipText.textContent}\u0000${tooltipLayout.fontScale}\u0000${window.innerWidth}x${window.innerHeight}`;
  if (key !== tooltipFitKey) {
    tooltipFitKey = key;
    setTooltipFitScale(1);
  }
  let fit = tooltipFitScale;
  while (fit > tooltipMinFitScale && tooltipOverflows()) {
    fit = Math.max(tooltipMinFitScale, Math.round((fit - 0.05) * 100) / 100);
    setTooltipFitScale(fit);
  }
  return fit;
}

// A hidden tooltip forgets its fit, so the next one starts again from 100%
// (a stale measurement can only ever shrink the text, never grow it back).
function resetTooltipFit() {
  tooltipFitKey = "";
  tooltipRepaintKey = "";
}

// The first measurement can run before the engine has laid the new text out;
// check again once it has been drawn and finish shrinking if it still overflows.
function scheduleTooltipFitRecheck() {
  if (tooltipFitRecheckPending) return;
  tooltipFitRecheckPending = true;
  const recheck = () => {
    tooltipFitRecheckPending = false;
    if (window.getComputedStyle(tooltipPanel).display === "none") return;
    // When the tooltip's text or size changed, make the engine repaint all of
    // it once: the part that grew could keep showing what was there before.
    // Only on a change (the tooltip is re-laid out every render), and undone
    // on the next frame so the nudge is actually drawn.
    const repaintKey = `${tooltipFitKey}\u0000${tooltipPanel.offsetHeight}`;
    if (repaintKey !== tooltipRepaintKey) {
      tooltipRepaintKey = repaintKey;
      tooltipPanel.style.opacity = "0.999";
      const undo = () => {
        tooltipPanel.style.opacity = "";
      };
      if (typeof window.requestAnimationFrame === "function") {
        window.requestAnimationFrame(undo);
      } else {
        window.setTimeout(undo, 16);
      }
    }
    // The height cap was set (or skipped) from a window size that may have been
    // stale; set it again from the size the drawn frame reports.
    const expectedCap = hasTrustedTooltipViewport() ? `${getTooltipMaxLogicalHeight()}px` : "none";
    const capStale = tooltipPanel.style.maxHeight !== expectedCap;
    // At the smallest scale there is nothing left to shrink; do not loop every frame.
    const canShrink = tooltipFitScale > tooltipMinFitScale && tooltipOverflows();
    if (capStale || canShrink) {
      applyTooltipLayout();
    }
  };
  if (typeof window.requestAnimationFrame === "function") {
    window.requestAnimationFrame(() => window.setTimeout(recheck, 0));
  } else {
    window.setTimeout(recheck, 16);
  }
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

// The fitted position only moves the tooltip on screen. A tall tooltip must not
// overwrite the position the player chose, or every later tooltip opens at the
// clamped spot and the next save stores it. A drag or nudge passes
// commitFitted so the saved value is where the player saw the tooltip.
function applyTooltipLayout(commitFitted = false) {
  tooltipLayout = normalizeTooltipLayout(tooltipLayout);
  document.documentElement.style.setProperty("--tooltip-font-scale", tooltipLayout.fontScale.toFixed(3));
  const trustedViewport = hasTrustedTooltipViewport();
  tooltipPanel.style.maxHeight = trustedViewport ? `${getTooltipMaxLogicalHeight()}px` : "none";
  tooltipPanel.style.right = `${tooltipLayout.right}px`;
  tooltipPanel.style.top = `${tooltipLayout.top}px`;
  if (window.getComputedStyle(tooltipPanel).display !== "none") {
    if (trustedViewport) {
      fitTooltipTextToHeight();
    }
    scheduleTooltipFitRecheck();
  }
  const fitted = fitTooltipLayoutToViewport(
    tooltipLayout,
    measureTooltipRect()
  );
  tooltipRenderedLayout = fitted;
  if (commitFitted) {
    tooltipLayout = fitted;
  }
  tooltipPanel.style.right = `${fitted.right}px`;
  tooltipPanel.style.top = `${fitted.top}px`;
  quickLaunch.style.right = `${fitted.right}px`;
  const quickOffset = window.innerWidth <= 900 ? 25 : 35;
  const quickTop = clamp(fitted.top - quickOffset, 8, Math.max(8, window.innerHeight - 56));
  quickLaunch.style.top = `${Math.round(quickTop)}px`;
  setTooltipSummaryText();
}

function persistTooltipLayout() {
  const right = Math.round(tooltipLayout.right);
  const top = Math.round(tooltipLayout.top);
  const fontPermille = Math.round(tooltipLayout.fontScale * 1000);
  sendCommand(`ui.tooltip.save:${right},${top},${fontPermille}`);
}

function applyAndPersistTooltipLayout(feedbackMessage, commitFitted = false) {
  applyTooltipLayout(commitFitted);
  persistTooltipLayout();
  if (feedbackMessage) {
    showActionFeedback(feedbackMessage);
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
      showActionFeedback(t("Invalid tooltip font command.", "툴팁 글자 명령이 올바르지 않습니다."));
      return true;
    }

    tooltipLayout = {
      ...tooltipLayout,
      fontScale: tooltipLayout.fontScale + delta
    };
    tooltipLayoutLoaded = true;
    applyAndPersistTooltipLayout();
    showActionFeedback(
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
      showActionFeedback(t("Invalid tooltip position command.", "툴팁 위치 명령이 올바르지 않습니다."));
      return true;
    }

    tooltipLayout = {
      ...tooltipLayout,
      right: tooltipLayout.right - deltaX,
      top: tooltipLayout.top + deltaY
    };
    tooltipLayoutLoaded = true;
    applyAndPersistTooltipLayout("", true);
    showActionFeedback(
      t(
        `Tooltip moved (right ${tooltipLayout.right}px, top ${tooltipLayout.top}px).`,
        `툴팁 이동됨 (우측 ${tooltipLayout.right}px, 상단 ${tooltipLayout.top}px).`
      )
    );
    return true;
  }

  showActionFeedback(t("Unknown tooltip command.", "알 수 없는 툴팁 명령입니다."));
  return true;
}

