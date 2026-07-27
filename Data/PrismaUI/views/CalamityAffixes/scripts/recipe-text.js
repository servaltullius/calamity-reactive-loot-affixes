      function resolveSelectedItemContextViewModel() {
        const hasSelection = Boolean(selectedItemNameState);
        const sourceText = selectedItemSourceState === "equipped"
          ? t("Equipped item selection", "착용 아이템 선택")
          : t("Current selection", "현재 선택");
        return {
          hasSelection,
          name: selectedItemNameState || t("No base selected", "선택된 베이스 없음"),
          sourceText: hasSelection
            ? sourceText
            : t("Equipped items only", "착용 장비만 대상")
        };
      }

      function renderSelectedItemContext() {
        const viewModel = resolveSelectedItemContextViewModel();
        selectedItemName.textContent = viewModel.name;
        selectedItemSource.textContent = viewModel.sourceText;
        if (affixSelectedItemName) {
          affixSelectedItemName.textContent = viewModel.hasSelection
            ? viewModel.name
            : t("No item selected", "선택된 아이템 없음");
        }
        if (affixSelectedItemMeta) {
          affixSelectedItemMeta.textContent = viewModel.hasSelection
            ? t(
                "This mirrors the currently highlighted inventory item.",
                "현재 인벤토리에서 강조된 아이템을 그대로 보여줍니다."
              )
            : t(
                "Highlight one inventory item to mirror its affix details here.",
                "인벤토리에서 아이템 하나를 가리키면 여기에서 어픽스 상세를 확인할 수 있습니다."
              );
        }
      }

      function resolveInventoryListViewModel() {
        const items = Array.isArray(inventoryItemsState) ? inventoryItemsState : [];
        return {
          items,
          emptyState: items.length === 0
            ? {
                title: t("No compatible equipped base", "호환되는 착용 베이스가 없습니다"),
                body: t(
                  "Equip a weapon, armor, helm, or shield first so Calamity can use it as a runeword base.",
                  "룬워드 베이스로 사용할 무기, 갑옷, 투구, 방패를 먼저 착용하세요."
                ),
                hint: t(
                  "Only currently equipped gear can be selected here.",
                  "여기에서는 현재 착용 중인 장비만 선택할 수 있습니다."
                )
              }
            : null
        };
      }

      function renderInventoryItems() {
        const focusedBaseKey = inventoryBaseList.contains(document.activeElement)
          ? document.activeElement?.dataset?.baseKey || ""
          : "";
        clearChildren(inventoryBaseList);
        const viewModel = resolveInventoryListViewModel();

        if (viewModel.emptyState) {
          appendEmptyState(
            inventoryBaseList,
            viewModel.emptyState.title,
            viewModel.emptyState.body,
            viewModel.emptyState.hint
          );
          return;
        }

        const renderedOptions = [];

        for (const item of viewModel.items) {
          const key = typeof item?.key === "string" ? item.key : "";
          const name = typeof item?.name === "string" ? item.name : "";
          const selected = Boolean(item?.selected);
          if (!key || !name) {
            continue;
          }

          const button = document.createElement("button");
          button.type = "button";
          button.className = selected ? "cpListItem rwRecipeListItem selected" : "cpListItem rwRecipeListItem";
          button.dataset.baseKey = key;
          button.textContent = name;
          button.title = name;
          button.setAttribute("role", "option");
          button.setAttribute("aria-label", name);
          button.setAttribute("aria-selected", selected ? "true" : "false");
          button.tabIndex = -1;
          button.setAttribute(panelCommandAttribute, `runeword.base.select:${key}`);
          inventoryBaseList.appendChild(button);
          renderedOptions.push({ button, key, selected });
        }

        const preferredOption = renderedOptions.find(
          (entry) => entry.key === focusedBaseKey
        ) || renderedOptions.find((entry) => entry.selected) || renderedOptions[0];
        if (preferredOption) {
          preferredOption.button.tabIndex = 0;
        }
      }

      function resolveLocalizedRecipeText(
        item,
        fieldName,
        bilingualSeparator = " / ",
        includeLegacyFallback = true
      ) {
        const enValue = typeof item?.[`${fieldName}En`] === "string"
          ? item[`${fieldName}En`].trim()
          : "";
        const koValue = typeof item?.[`${fieldName}Ko`] === "string"
          ? item[`${fieldName}Ko`].trim()
          : "";
        const legacyValue = includeLegacyFallback && typeof item?.[fieldName] === "string"
          ? item[fieldName].trim()
          : "";

        if (uiLang === "en") {
          return enValue || legacyValue || koValue;
        }
        if (uiLang === "ko") {
          return koValue || legacyValue || enValue;
        }
        if (enValue && koValue && enValue !== koValue) {
          return `${enValue}${bilingualSeparator}${koValue}`;
        }
        return enValue || koValue || legacyValue;
      }

      function resolveRecipeSummaryText(item) {
        const key = typeof item?.summaryKey === "string" ? item.summaryKey : "";
        switch (key) {
          case "signature_spirit":
            return t(
              "Spirit: +30 Max Magicka; 28% on-hit chance for +10 percentage points Spell Absorption (5s, 10s cooldown)",
              "스피릿: 최대 마나 +30, 적중 시 28% 확률로 주문 흡수 확률 +10%p(5초, 재사용 10초)"
            );
          case "self_weapon_fury":
            return t(
              "Fury: 24% on-hit chance for +25% Attack Speed (6s) and 30 Stamina (12s cooldown)",
              "퓨리: 적중 시 24% 확률로 공격 속도 +25%(6초)·기력 30 회복(재사용 12초)"
            );
          case "self_smoke_escape":
            return t(
              "Smoke: 22% chance on taking a hit to slow the attacker by 30% (5s, 12s cooldown)",
              "스모크: 피격 시 22% 확률로 공격자 이동 속도 -30%(5초, 재사용 12초)"
            );
          case "self_carry_weight":
            return t(
              "Wealth: passive +75 Carry Weight and +15 Speechcraft",
              "웰스: 상시 소지 무게 +75·화술 +15"
            );
          default:
            break;
        }

        const localizedSummary = resolveLocalizedRecipeText(
          item,
          "summary",
          " / ",
          false
        );
        if (localizedSummary) {
          return localizedSummary;
        }
        const fallbackSummary = typeof item?.summary === "string" ? item.summary.trim() : "";
        switch (key) {
          case "adaptive_strike":
            return t("Adaptive elemental strike", "적응형 원소 참격");
          case "adaptive_exposure":
            return t("Elemental exposure / shred", "원소 노출/파쇄");
          case "signature_nadir":
            return t("Nadir: panic fear ward", "나디르: 위기 공포 결계");
          case "signature_steel":
            return t("Steel: tempered opening strike", "스틸: 단련된 선제 참격");
          case "signature_malice":
            return t("Malice: lingering venom wounds", "맬리스: 지속 독상처");
          case "signature_stealth":
            return t("Stealth: emergency concealment", "스텔스: 긴급 은신");
          case "signature_leaf":
            return t("Leaf: ember burst ignition", "리프: 불씨 폭발 점화");
          case "signature_ancients_pledge":
            return t("Ancient's Pledge: last-stand bulwark", "고대인의 서약: 최후의 방벽");
          case "signature_holy_thunder":
            return t("Holy Thunder: sanctified shock mantle", "홀리 썬더: 성전의 번개 장막");
          case "signature_zephyr":
            return t("Zephyr: gale-charged stride", "제피르: 질풍 충전 기동");
          case "signature_pattern":
            return t("Pattern: combo rhythm strike", "패턴: 연계 리듬 참격");
          case "signature_kings_grace":
            return t("King's Grace: consecrated blade arc", "왕의 은총: 성스러운 검격");
          case "signature_strength":
            return t("Strength: crushing pressure burst", "스트렝스: 압쇄 폭발");
          case "signature_edge":
            return t("Edge: razor wind shot", "엣지: 예리한 바람 사격");
          case "signature_grief":
            return t("Grief: hyper-fast chaos strike", "그리프: 초고속 혼돈 참격");
          case "signature_infinity":
            return t("Infinity: conviction-style resistance shred", "인피니티: 확신형 저항 파쇄");
          case "signature_enigma":
            return t("Enigma: emergency phase shift", "에니그마: 긴급 위상 전환");
          case "signature_call_to_arms":
            return t("Call to Arms: battle-cry haste", "콜 투 암스: 전투 함성 가속");
          case "signature_insight":
            return t("Insight: meditation pulse engine", "인사이트: 명상 맥동 엔진");
          case "signature_fortitude":
            return t("Fortitude: iron ward bastion", "포티튜드: 강철 수호 보루");
          case "signature_heart_of_the_oak":
            return t("Heart of the Oak: druidic exposure weave", "참나무의 심장: 드루이드 파쇄 직조");
          case "signature_last_wish":
            return t("Last Wish: relentless judgment omen", "라스트 위시: 집념의 심판 징조");
          case "signature_exile":
            return t("Exile: emergency barrier sanctuary", "엑자일: 긴급 장벽 성역");
          case "signature_breath_of_the_dying":
            return t("Breath of the Dying: terminal reaper surge", "죽음의 숨결: 종말의 사신 폭주");
          case "signature_chains_of_honor":
            return t("Chains of Honor: venerated phase guard", "명예의 사슬: 성역 위상 수호");
          case "signature_dream":
            return t("Dream: lucid thunder resonance", "드림: 자각의 번개 공명");
          case "signature_faith":
            return t("Faith: zeal cadence overdrive", "페이스: 열광의 율동 과부하");
          case "signature_phoenix":
            return t("Phoenix: ash-rebirth counterflare", "피닉스: 재 점화 역류");
          case "signature_doom":
            return t("Doom: glacial verdict strike", "둠: 빙결 심판 일격");
          case "signature_bone":
            return t("Bone: gravebound familiar call", "본: 묘지의 사역마 소환");
          case "fire_strike":
            return t("Fire strike proc", "화염 참격 발동");
          case "frost_strike":
            return t("Frost strike proc", "냉기 참격 발동");
          case "shock_strike":
            return t("Shock strike proc", "번개 참격 발동");
          case "poison_bloom":
            return t("Poison DoT bloom", "독 DoT 블룸");
          case "tar_bloom":
            return t("Tar bloom (slow)", "타르 블룸(둔화)");
          case "siphon_bloom":
            return t("Siphon bloom (resource drain)", "흡수 블룸(자원 약화)");
          case "curse_fragile":
            return t("Fragility curse", "취약 저주");
          case "curse_slow_attack":
            return t("Slow attack curse", "공속 저하 저주");
          case "curse_fear":
            return t("Fear crowd-control curse", "공포 군중제어 저주");
          case "curse_frenzy":
            return t("Frenzy chaos curse", "광란 혼란 저주");
          case "self_haste":
            return t("Self haste buff", "자가 가속 버프");
          case "self_ward":
            return t("Self ward defense buff", "자가 수호 방어 버프");
          case "self_barrier":
            return t("Self barrier defense buff", "자가 장벽 방어 버프");
          case "self_meditation":
            return t("Self meditation sustain buff", "자가 명상 유지 버프");
          case "self_phase":
            return t("Self phase mobility buff", "자가 위상 기동 버프");
          case "self_phoenix":
            return t("Self phoenix surge buff", "자가 피닉스 폭주 버프");
          case "self_flame_cloak":
            return t("Flame cloak aura", "화염 망토 오라");
          case "self_frost_cloak":
            return t("Frost cloak aura", "냉기 망토 오라");
          case "self_shock_cloak":
            return t("Shock cloak aura", "번개 망토 오라");
          case "self_oakflesh":
            return t("Oakflesh armor buff", "오크플레시 방어 버프");
          case "self_stoneflesh":
            return t("Stoneflesh armor buff", "스톤플레시 방어 버프");
          case "self_ironflesh":
            return t("Ironflesh armor buff", "아이언플레시 방어 버프");
          case "self_ebonyflesh":
            return t("Ebonyflesh armor buff", "에보니플레시 방어 버프");
          case "self_muffle":
            return t("Muffle stealth utility", "머플 은신 유틸");
          case "self_invisibility":
            return t("Invisibility stealth utility", "투명화 은신 유틸");
          case "soul_trap":
            return t("Soul Trap utility", "소울트랩 유틸");
          default:
            return fallbackSummary;
        }
      }

      function resolveRecipeNumericSummaryText(item) {
        return resolveRecipeSummaryText(item);
      }

      function resolveRecipeFlavorDetailText(item) {
        const key = typeof item?.summaryKey === "string" ? item.summaryKey : "";
        switch (key) {
          case "adaptive_strike":
            return t(
              "On hit, follows the target's lowest resistance lane for stable damage.",
              "적중 시 대상의 가장 낮은 저항 축을 따라 안정적으로 피해를 넣습니다."
            );
          case "adaptive_exposure":
            return t(
              "On hit, breaks the target's highest resistance first to open follow-up damage.",
              "적중 시 대상의 가장 높은 저항을 먼저 깎아 후속 피해 창을 엽니다."
            );
          case "signature_infinity":
            return t(
              "Adaptive shred is default. In manual mode, you can lock Fire/Frost/Shock.",
              "기본은 적응형 파쇄이며, 수동 모드에서 화염/냉기/번개를 고정할 수 있습니다."
            );
          case "signature_last_wish":
            return t(
              "Adaptive shred is default. In manual mode, lock Inferno/Frost/Lightning variants.",
              "기본은 적응형 파쇄이며, 수동 모드에서 열화/빙결/전격 변형을 고정할 수 있습니다."
            );
          case "signature_heart_of_the_oak":
            return t(
              "In boss phases, it prioritizes breaking the highest resistance first (StrongestResist).",
              "보스 구간에서 가장 높은 저항 축(StrongestResist)부터 먼저 파쇄합니다."
            );
          case "signature_call_to_arms":
            return t(
              "Triggers on hit and grants a short defensive warcry buff.",
              "적중 시 발동해 짧은 방어형 전투 함성 버프를 부여합니다."
            );
          case "signature_faith":
            return t(
              "Fanatic Surge: +35 Move Speed, +60 Damage Resist for 6s.",
              "광신의 돌격: 6초 동안 이동속도 +35, 피해저항 +60."
            );
          case "signature_doom":
          case "signature_dream":
            return t(
              "Uses dedicated runeword strike spells instead of generic dynamic spells.",
              "범용 동적 스펠 대신 룬워드 전용 타격 스펠을 사용합니다."
            );
          case "poison_bloom":
          case "tar_bloom":
          case "siphon_bloom":
            return t(
              "Bloom-style damage-over-time effect. Faster hit rate improves uptime value.",
              "블룸형 지속 피해 효과입니다. 적중 속도가 빠를수록 유지 효율이 좋아집니다."
            );
          case "curse_fragile":
          case "curse_slow_attack":
          case "curse_fear":
          case "curse_frenzy":
            return t(
              "Control/debuff curse effect with cooldown and per-target safeguards.",
              "쿨다운과 대상별 가드가 있는 제어/디버프 저주 효과입니다."
            );
          case "self_flame_cloak":
          case "self_frost_cloak":
          case "self_shock_cloak":
            return t(
              "Self-cloak aura effect for close-range pressure.",
              "근접 압박에 유리한 자가 망토 오라 효과입니다."
            );
          case "self_oakflesh":
          case "self_stoneflesh":
          case "self_ironflesh":
          case "self_ebonyflesh":
            return t(
              "Flesh armor effect that improves frontline survivability uptime.",
              "전선에서 버티는 시간을 늘려주는 플레시 방어 효과입니다."
            );
          case "self_meditation":
            return t(
              "Sustain effect for keeping resources up during long boss phases.",
              "장기 보스전에서 자원 유지를 돕는 지속 효과입니다."
            );
          case "self_phase":
            return t(
              "Mobility effect for emergency repositioning and reset timing.",
              "긴급 재배치와 리셋 타이밍 확보용 기동 효과입니다."
            );
          case "self_haste":
            return t(
              "Haste effect that raises action speed and clear tempo.",
              "행동 속도와 클리어 템포를 올리는 가속 효과입니다."
            );
          case "self_ward":
          case "self_barrier":
          case "self_phoenix":
            return t(
              "Mitigation effect for surviving danger windows.",
              "위험 구간을 버티기 위한 피해 완화 효과입니다."
            );
          case "signature_spirit":
            return t(
              "Passively grants +30 Max Magicka. On hit, has a 28% chance to gain 10 percentage points of Spell Absorption for 5s (10s cooldown).",
              "최대 마나 +30이 상시 적용됩니다. 적중 시 28% 확률로 주문 흡수 확률이 10%p 증가합니다(5초, 재사용 10초)."
            );
          case "self_weapon_fury":
            return t(
              "On hit, has a 24% chance to gain +25% Attack Speed for 6s and immediately restore 30 Stamina (12s cooldown).",
              "적중 시 24% 확률로 공격 속도가 25% 증가하고 기력 30을 즉시 회복합니다(6초, 재사용 12초)."
            );
          case "self_smoke_escape":
            return t(
              "On taking a hit, has a 22% chance to reduce the attacker's Move Speed by 30% for 5s (12s cooldown).",
              "피격 시 22% 확률로 공격자의 이동 속도를 30% 낮춥니다(5초, 재사용 12초)."
            );
          case "self_carry_weight":
            return t(
              "Passively grants +75 Carry Weight and +15 Speechcraft.",
              "소지 무게 +75와 화술 +15가 상시 적용됩니다."
            );
          case "signature_insight":
            return t(
              "Hybrid sustain effect supporting both combat flow and resource recovery.",
              "전투 흐름과 자원 회복을 함께 챙기는 하이브리드 유지 효과입니다."
            );
          default:
            return "";
        }
      }

      function resolveRecipeDetailText(item) {
        const fallbackDetail = resolveLocalizedRecipeText(
          item,
          "detail",
          "\n"
        );
        const mergeDetail = (mappedDetail) => {
          const mapped = typeof mappedDetail === "string" ? mappedDetail.trim() : "";
          if (fallbackDetail && mapped) {
            return `${fallbackDetail}\n${mapped}`;
          }
          return fallbackDetail || mapped;
        };
        return mergeDetail(resolveRecipeFlavorDetailText(item));
      }

      function buildRunewordTooltipLikeText(item, a_options = {}) {
        if (!item) return "";
        const includeName = a_options?.includeName !== false;
        const includeDetail = a_options?.includeDetail !== false;
        const lines = [];

        if (includeName) {
          const name = typeof item?.name === "string" ? item.name.trim() : "";
          const runes = typeof item?.runes === "string" ? item.runes.trim() : "";
          if (name && runes) {
            lines.push(`${name} [${runes}]`);
          } else if (name) {
            lines.push(name);
          } else if (runes) {
            lines.push(runes);
          }
        }

        const summaryText = resolveRecipeNumericSummaryText(item);
        if (summaryText) {
          lines.push(summaryText);
        }

        if (includeDetail) {
          const detailText = resolveRecipeDetailText(item);
          if (detailText) {
            lines.push(detailText);
          }
        }

        return lines.join("\n");
      }

      function buildRecipePreviewTooltipText(item) {
        return buildRunewordTooltipLikeText(item, {
          includeName: true,
          includeDetail: true
        });
      }

      function resolveRecipeBaseBadge(item) {
        const key = typeof item?.baseKey === "string" ? item.baseKey : "";
        switch (key) {
          case "polearm":
            return { className: "weapon", text: t("Base: Polearm/Spear", "베이스: 폴암/창") };
          case "bow":
            return { className: "weapon", text: t("Base: Bow/Crossbow", "베이스: 활/석궁") };
          case "one_handed_melee":
            return { className: "weapon", text: t("Base: One-Handed Melee", "베이스: 한손 근접 무기") };
          case "two_handed_melee":
            return { className: "weapon", text: t("Base: Two-Handed Melee", "베이스: 양손 근접 무기") };
          case "staff_wand":
            return { className: "weapon", text: t("Base: Staff/Wand", "베이스: 지팡이/완드") };
          case "claw":
            return { className: "weapon", text: t("Base: Claw", "베이스: 클로") };
          case "sword":
            return { className: "weapon", text: t("Base: Sword", "베이스: 검") };
          case "shield":
            return { className: "armor", text: t("Base: Shield", "베이스: 방패") };
          case "helm":
            return { className: "armor", text: t("Base: Helm", "베이스: 투구") };
          case "armor":
            return { className: "armor", text: t("Base: Armor", "베이스: 갑옷") };
          case "heavy_armor":
            return { className: "armor", text: t("Base: Heavy Armor", "베이스: 중갑") };
          case "weapon_shield":
            return { className: "mixed", text: t("Base: Weapon/Shield", "베이스: 무기/방패") };
          case "helm_shield":
            return { className: "mixed", text: t("Base: Helm/Shield", "베이스: 투구/방패") };
          case "armor_shield":
            return { className: "mixed", text: t("Base: Armor/Shield", "베이스: 갑옷/방패") };
          case "sword_shield":
            return { className: "mixed", text: t("Base: Sword/Shield", "베이스: 검/방패") };
          case "weapon":
            return { className: "weapon", text: t("Base: Weapon", "베이스: 무기") };
          case "mixed":
            return { className: "mixed", text: t("Base: Mixed", "베이스: 혼합") };
          case "armor_only":
            return { className: "armor", text: t("Base: Armor", "베이스: 갑옷") };
          default:
            return { className: "mixed", text: t("Base: Mixed", "베이스: 혼합") };
        }
      }

