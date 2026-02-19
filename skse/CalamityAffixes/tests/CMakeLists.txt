cmake_minimum_required(VERSION 3.20)

project(CalamityAffixesRuntimeChecks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

if(BUILD_TESTING)
    add_executable(
        runtime_gate_store_checks
        runtime_gate_store_checks_main.cpp
        runtime_gate_store_checks_runtime.cpp
        runtime_gate_store_checks_loot_policy.cpp
        runtime_gate_store_checks_runeword_policy.cpp
        runtime_gate_store_checks_settings_policy.cpp
    )

    target_include_directories(
        runtime_gate_store_checks
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../extern/vendor/include
    )

    add_test(
        NAME CalamityAffixesRuntimeGateStoreChecks
        COMMAND runtime_gate_store_checks
    )
endif()
