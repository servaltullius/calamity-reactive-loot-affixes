cmake_minimum_required(VERSION 3.21)

project(CalamityAffixes VERSION 1.2.19 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

	set(headers
	    include/CalamityAffixes/CombatContext.h
	    include/CalamityAffixes/AffixToken.h
	    include/CalamityAffixes/EventBridge.h
	    include/CalamityAffixes/HitDataUtil.h
	    include/CalamityAffixes/Hooks.h
	    include/CalamityAffixes/InstanceStateKey.h
	    include/CalamityAffixes/LootCurrencyLedger.h
	    include/CalamityAffixes/LootRerollGuard.h
	    include/CalamityAffixes/LootUiGuards.h
	    include/CalamityAffixes/NonHostileFirstHitGate.h
	    include/CalamityAffixes/Papyrus.h
	    include/CalamityAffixes/PerTargetCooldownStore.h
	    include/CalamityAffixes/PointerSafety.h
		    include/CalamityAffixes/PlayerOwnership.h
		    include/CalamityAffixes/PrismaLayoutPersistence.h
		    include/CalamityAffixes/PrismaTooltip.h
		    include/CalamityAffixes/RuntimeContract.h
		    include/CalamityAffixes/RuntimePolicy.h
		    include/CalamityAffixes/RuntimePaths.h
		    include/CalamityAffixes/RuntimeUserSettingsDebounce.h
	    include/CalamityAffixes/RunewordContractSnapshot.h
	    include/CalamityAffixes/RunewordUtil.h
	    include/CalamityAffixes/RunewordUiPolicy.h
	    include/CalamityAffixes/TriggerGuards.h
	    include/CalamityAffixes/TrapSystem.h
	    include/CalamityAffixes/UserSettingsPersistence.h
	)

	set(sources
	    src/main.cpp
	    src/EventBridge.cpp
	    src/EventBridge.Config.cpp
	    src/EventBridge.Config.Reset.cpp
	    src/EventBridge.Config.LootRuntime.cpp
	    src/EventBridge.Config.RuntimeSettings.cpp
	    src/EventBridge.Config.AffixParsing.cpp
	    src/EventBridge.Config.AffixTriggerParsing.cpp
	    src/EventBridge.Config.AffixActionParsing.cpp
	    src/EventBridge.Config.Indexing.cpp
	    src/EventBridge.Config.IndexingShared.cpp
	    src/EventBridge.Config.AffixRegistry.cpp
	    src/EventBridge.Config.LootPools.cpp
	    src/EventBridge.Config.RunewordSynthesis.cpp
	    src/EventBridge.Config.RunewordSynthesis.StyleSelection.cpp
	    src/EventBridge.Config.RunewordSynthesis.Style.cpp
	    src/EventBridge.Config.RunewordSynthesis.StyleExecution.cpp
	    src/EventBridge.Config.RunewordSynthesis.SpellSet.cpp
	    src/EventBridge.Loot.cpp
	    src/EventBridge.Loot.Runeword.Detail.cpp
	    src/EventBridge.Loot.Runeword.Selection.cpp
	    src/EventBridge.Loot.Runeword.BaseSelection.cpp
	    src/EventBridge.Loot.Runeword.RecipeUi.cpp
	    src/EventBridge.Loot.Runeword.RecipeEntries.cpp
	    src/EventBridge.Loot.Runeword.PanelState.cpp
	    src/EventBridge.Loot.Runeword.Catalog.cpp
	    src/EventBridge.Loot.Runeword.Crafting.cpp
	    src/EventBridge.Loot.Runeword.Transmute.cpp
	    src/EventBridge.Loot.Runeword.Reforge.cpp
	    src/EventBridge.Loot.Runeword.Policy.cpp
	    src/EventBridge.Loot.Runtime.cpp
	    src/EventBridge.Loot.TooltipResolution.cpp
	    src/EventBridge.Loot.PreviewClaimStore.cpp
	    src/EventBridge.Loot.AffixSlotRoll.cpp
	    src/EventBridge.Loot.Assign.cpp
	    src/EventBridge.Triggers.cpp
	    src/EventBridge.Triggers.HealthDamage.cpp
	    src/EventBridge.Triggers.HitEvent.cpp
	    src/EventBridge.Triggers.DeathEvent.cpp
	    src/EventBridge.Triggers.EquipEvent.cpp
	    src/EventBridge.Triggers.ActivateEvent.cpp
	    src/EventBridge.Triggers.MagicEffectApply.cpp
	    src/EventBridge.Triggers.ModCallback.cpp
	    src/EventBridge.Triggers.ActiveCounts.cpp
	    src/EventBridge.Actions.cpp
	    src/EventBridge.Actions.Dispatch.cpp
	    src/EventBridge.Actions.Cast.cpp
	    src/EventBridge.Actions.Trap.cpp
	    src/EventBridge.Actions.Special.cpp
	    src/EventBridge.Actions.Corpse.cpp
	    src/EventBridge.Traps.cpp
	    src/EventBridge.Serialization.cpp
	    src/Hooks.cpp
	    src/Papyrus.cpp
	    src/PrismaLayoutPersistence.cpp
	    src/PrismaTooltip.cpp
	    src/RuntimePaths.cpp
	    src/RunewordContractSnapshot.cpp
	    src/RunewordIdentityOverrides.cpp
	    src/TrapSystem.cpp
	    src/UserSettingsPersistence.cpp
	)

add_library(
    ${PROJECT_NAME}
    SHARED
        ${headers}
        ${sources}
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

# CommonLibSSE install exports do not always propagate rapidcsv include paths.
# Add local fallbacks when available so REL/Relocation.h can resolve rapidcsv.h.
set(calamity_optional_include_dirs "")
foreach(calamity_inc_dir IN ITEMS
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/vendor/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/rapidcsv"
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibSSE-NG/extern/rapidcsv/src"
)
    if (EXISTS "${calamity_inc_dir}/rapidcsv.h")
        list(APPEND calamity_optional_include_dirs "${calamity_inc_dir}")
    endif()
endforeach()
if (calamity_optional_include_dirs)
    target_include_directories(${PROJECT_NAME} PRIVATE ${calamity_optional_include_dirs})
endif()

# Optional (nice-to-have): disable Windows min/max macro pollution.
target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE CommonLibSSE::CommonLibSSE nlohmann_json::nlohmann_json)

# Linux/WSL cross-linking with lld-link via CMake's `-E vs_link_dll` can sanitize
# the `LIB` environment variable, so we add explicit `/LIBPATH:` entries when an
# xwin sysroot is present.
function(calamity_add_xwin_libpaths target)
    set(xwin_root "${CMAKE_CURRENT_SOURCE_DIR}/.xwin")
    if (NOT EXISTS "${xwin_root}")
        return()
    endif()

    file(GLOB msvc_versions LIST_DIRECTORIES true RELATIVE "${xwin_root}/VC/Tools/MSVC" "${xwin_root}/VC/Tools/MSVC/*")
    list(SORT msvc_versions)
    list(LENGTH msvc_versions msvc_count)
    if (msvc_count EQUAL 0)
        return()
    endif()
    list(GET msvc_versions -1 msvc_ver)
    set(msvc_lib "${xwin_root}/VC/Tools/MSVC/${msvc_ver}/lib/x86_64")

    file(GLOB sdk_versions LIST_DIRECTORIES true RELATIVE "${xwin_root}/Windows Kits/10/Lib" "${xwin_root}/Windows Kits/10/Lib/*")
    list(SORT sdk_versions)
    list(LENGTH sdk_versions sdk_count)
    if (sdk_count EQUAL 0)
        return()
    endif()
    list(GET sdk_versions -1 sdk_ver)
    set(sdk_um "${xwin_root}/Windows Kits/10/Lib/${sdk_ver}/um/x86_64")
    set(sdk_ucrt "${xwin_root}/Windows Kits/10/Lib/${sdk_ver}/ucrt/x86_64")

    if (EXISTS "${msvc_lib}" AND EXISTS "${sdk_um}" AND EXISTS "${sdk_ucrt}")
        target_link_options(${target} PRIVATE
            "/LIBPATH:${msvc_lib}"
            "/LIBPATH:${sdk_um}"
            "/LIBPATH:${sdk_ucrt}"
        )
    endif()
endfunction()

calamity_add_xwin_libpaths(${PROJECT_NAME})

include(CTest)

if (BUILD_TESTING)
    # Compile-time checks (static_assert). Keep excluded from default build and
    # expose an explicit target that CTest can invoke in any toolchain.
    add_library(
        ${PROJECT_NAME}Tests
        OBJECT
        EXCLUDE_FROM_ALL
            tests/test_affix_token.cpp
            tests/test_adaptive_element.cpp
            tests/test_magnitude_scaling.cpp
            tests/test_loot_reroll_guard.cpp
            tests/test_loot_eligibility.cpp
            tests/test_loot_ui_guards.cpp
            tests/test_instance_state_key.cpp
            tests/test_resync_scheduler.cpp
            tests/test_runeword_util.cpp
            tests/test_reforge_rules.cpp
            tests/test_trigger_guards.cpp
            tests/test_non_hostile_first_hit_gate.cpp
            tests/test_per_target_cooldown_store.cpp
            tests/test_instance_affix_slots.cpp
            tests/test_effective_loot_weight.cpp
            tests/test_pointer_safety.cpp
            tests/test_editor_id_safety.cpp
    )
    target_include_directories(${PROJECT_NAME}Tests PRIVATE include)

    add_custom_target(
        ${PROJECT_NAME}StaticChecks
        DEPENDS ${PROJECT_NAME}Tests
    )

    set(calamity_static_checks_cmd
        ${CMAKE_COMMAND}
        --build ${CMAKE_BINARY_DIR}
        --target ${PROJECT_NAME}StaticChecks
    )
    if (CMAKE_CONFIGURATION_TYPES)
        list(APPEND calamity_static_checks_cmd --config $<CONFIG>)
    endif()

    add_test(
        NAME ${PROJECT_NAME}StaticChecks
        COMMAND ${calamity_static_checks_cmd}
    )

    if (CMAKE_HOST_UNIX)
        add_test(
            NAME ${PROJECT_NAME}RuntimeGateStoreChecks
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_runtime_gate_store_checks.sh
        )
    endif()

    # Micro-benchmark (manual-run only, not registered with ctest).
    add_executable(bench_core_ops EXCLUDE_FROM_ALL tests/bench_core_ops.cpp)
    target_include_directories(bench_core_ops PRIVATE include)
    calamity_add_xwin_libpaths(bench_core_ops)
endif()
