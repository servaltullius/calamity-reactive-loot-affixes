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

// C++ pushes fixed English feedback strings; map the known ones through the
// language mode so the status line is not the only untranslated text on screen.
const engineFeedbackLocalizations = {
  "Prisma panel opened.": ["Panel opened.", "패널을 열었습니다."],
  "Prisma panel closed.": ["Panel closed.", "패널을 닫았습니다."],
  "Runeword base selected.": ["Runeword base selected.", "룬워드 베이스를 선택했습니다."],
  "Runeword recipe selected.": ["Runeword recipe selected.", "룬워드 레시피를 선택했습니다."],
  "Runeword -> transmute requested": ["Transmute requested.", "변환을 요청했습니다."],
  "Runeword: already complete": ["Runeword already complete.", "이미 완성된 룬워드입니다."],
  "Failed to send command event.": ["Failed to send command event.", "명령 이벤트 전송에 실패했습니다."],
  "Invalid base selection key.": ["Invalid base selection.", "잘못된 베이스 선택입니다."],
  "Invalid recipe selection key.": ["Invalid recipe selection.", "잘못된 레시피 선택입니다."],
  "Failed to select runeword base.": ["Failed to select runeword base.", "룬워드 베이스 선택에 실패했습니다."],
  "Failed to select runeword recipe.": ["Failed to select runeword recipe.", "룬워드 레시피 선택에 실패했습니다."],
  "Runeword system unavailable.": ["Runeword system unavailable.", "룬워드 시스템을 사용할 수 없습니다."],
  "Reforge system unavailable.": ["Reforge system unavailable.", "재련 시스템을 사용할 수 없습니다."],
  "Reset system unavailable.": ["Reset system unavailable.", "초기화 시스템을 사용할 수 없습니다."],
  "Currency system unavailable.": ["Currency system unavailable.", "화폐 시스템을 사용할 수 없습니다."]
};

function localizeEngineFeedback(value) {
  const entry = engineFeedbackLocalizations[value];
  return entry ? t(entry[0], entry[1]) : value;
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

