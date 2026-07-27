function normalizeUiLang(raw) {
  const value = String(raw ?? "").trim().toLowerCase();
  if (value === "en" || value === "english" || value === "0") return "en";
  if (value === "ko" || value === "korean" || value === "kr" || value === "1") return "ko";
  if (
    value === "both" ||
    value === "bilingual" ||
    value === "en+ko" ||
    value === "ko+en" ||
    value === "2"
  ) {
    return "both";
  }
  return "both";
}

function resolveUiLang() {
  try {
    const url = new URL(window.location.href);
    const forced = normalizeUiLang(url.searchParams.get("lang"));
    if (forced === "en" || forced === "ko" || forced === "both") {
      return forced;
    }
  } catch (_) {}

  return "both";
}

let uiLang = resolveUiLang();

function t(en, ko) {
  if (uiLang === "en") return en;
  if (uiLang === "ko") return ko;
  return `${en} / ${ko}`;
}

function updatePanelHotkeyHints() {
  const key = panelHotkeyTextState || "";
  const label = t(
    `Open/Close Panel: ${key}`.trim(),
    `패널 열기/닫기: ${key}`.trim()
  );
  if (quickOpenRuneword) {
    quickOpenRuneword.textContent = label;
  }
  if (tooltipRunewordHint) {
    tooltipRunewordHint.textContent = label;
  }
}

