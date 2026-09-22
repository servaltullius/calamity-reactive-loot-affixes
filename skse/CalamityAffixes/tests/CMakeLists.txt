cmake_minimum_required(VERSION 3.20)

project(CalamityAffixesRuntimeChecks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

if(BUILD_TESTING)
    find_package(Threads REQUIRED)

    add_executable(
        runtime_gate_store_checks
        runtime_gate_store_checks_main.cpp
        runtime_gate_store_checks_runtime.cpp
        runtime_gate_store_checks_loot_policy.cpp
        runtime_gate_store_checks_runeword_policy.cpp
        runtime_gate_store_checks_settings_policy.cpp
        runtime_gate_store_checks_stateful.cpp
        runtime_gate_store_checks_loot_distribution.cpp
        runtime_gate_store_checks_trigger_dispatch.cpp
        runtime_gate_store_checks_serialization.cpp
    )

    target_include_directories(
        runtime_gate_store_checks
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../extern/vendor/include
    )
    target_link_libraries(runtime_gate_store_checks PRIVATE Threads::Threads)

    add_executable(deferred_event_dispatcher_tests deferred_event_dispatcher_tests.cpp)
    target_include_directories(deferred_event_dispatcher_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(deferred_event_dispatcher_tests PRIVATE Threads::Threads)

    add_executable(summon_protection_tests summon_protection_tests.cpp)
    target_include_directories(summon_protection_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    add_test(NAME CalamityAffixesSummonProtectionChecks COMMAND summon_protection_tests)

    add_test(
        NAME CalamityAffixesDeferredEventDispatcherChecks
        COMMAND deferred_event_dispatcher_tests
    )
    set_tests_properties(CalamityAffixesDeferredEventDispatcherChecks PROPERTIES TIMEOUT 30)

    add_test(
        NAME CalamityAffixesRuntimeGateStoreChecks
        COMMAND runtime_gate_store_checks
    )
endif()
