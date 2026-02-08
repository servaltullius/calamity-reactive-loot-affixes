# Minimal header-only nlohmann_json package config for this repo's cross-compile toolchain.
#
# Avoid /usr/include while targeting Windows via clang-cl + xwin.

get_filename_component(_NLOHMANN_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

set(nlohmann_json_INCLUDE_DIRS "${_NLOHMANN_PREFIX}/include")

if (NOT TARGET nlohmann_json::nlohmann_json)
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${nlohmann_json_INCLUDE_DIRS}"
    )
endif()

