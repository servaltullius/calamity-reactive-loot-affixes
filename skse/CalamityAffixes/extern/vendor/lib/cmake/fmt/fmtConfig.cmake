# Minimal header-only fmt package config for this repo's cross-compile toolchain.
#
# We intentionally avoid relying on system-installed fmt when building with clang-cl
# + a Windows sysroot, because adding /usr/include pollutes the include search path
# and can break MSVC standard library headers.

get_filename_component(_FMT_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

set(fmt_INCLUDE_DIRS "${_FMT_PREFIX}/include")

if (NOT TARGET fmt::fmt)
    add_library(fmt::fmt INTERFACE IMPORTED)
    set_target_properties(fmt::fmt PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${fmt_INCLUDE_DIRS}"
    )
endif()

