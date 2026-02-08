# Minimal header-only spdlog package config for this repo's cross-compile toolchain.
#
# This pairs with the vendored fmt headers and avoids pulling in /usr/include while
# targeting Windows via clang-cl + xwin.

get_filename_component(_SPDLOG_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

set(spdlog_INCLUDE_DIRS "${_SPDLOG_PREFIX}/include")

if (NOT TARGET spdlog::spdlog_header_only)
    add_library(spdlog::spdlog_header_only INTERFACE IMPORTED)
    set_target_properties(spdlog::spdlog_header_only PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${spdlog_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "fmt::fmt"
    )
endif()

if (NOT TARGET spdlog::spdlog)
    add_library(spdlog::spdlog INTERFACE IMPORTED)
    set_target_properties(spdlog::spdlog PROPERTIES
        INTERFACE_LINK_LIBRARIES "spdlog::spdlog_header_only"
    )
endif()

