#!/usr/bin/env bash

# Sets CAFF_REPO_RUN_ROOT_RESULT to a path for the repo root that dotnet and
# cmake can use (no spaces). Call it directly, never inside "$(...)": when no
# alias exists it creates a temp symlink directory and records it in
# CAFF_REPO_RUN_ROOT_TEMP_DIR, and a command substitution would drop that
# variable in its subshell, leaving cleanup_repo_run_root nothing to remove.
resolve_repo_run_root() {
  local repo_root="$1"
  local override="${CAFF_REPO_RUN_ROOT:-}"
  local safe_repo_link=""
  local candidate_links=(
    "$(dirname "${repo_root}")/calamity"
    "$(dirname "${repo_root}")/projects/calamity"
  )
  CAFF_REPO_RUN_ROOT_RESULT=""

  if [[ -n "${override}" ]]; then
    if [[ ! -d "${override}" ]]; then
      echo "CAFF_REPO_RUN_ROOT is not a directory: ${override}" >&2
      return 1
    fi
    if [[ "$(realpath "${override}")" != "$(realpath "${repo_root}")" ]]; then
      echo "CAFF_REPO_RUN_ROOT must resolve to the repo root: ${override}" >&2
      return 1
    fi
    CAFF_REPO_RUN_ROOT_RESULT="${override}"
    return 0
  fi

  if [[ "${repo_root}" != *" "* ]]; then
    CAFF_REPO_RUN_ROOT_RESULT="${repo_root}"
    return 0
  fi

  for safe_repo_link in "${candidate_links[@]}"; do
    if [[ -d "${safe_repo_link}" ]] && [[ "$(realpath "${safe_repo_link}")" == "$(realpath "${repo_root}")" ]]; then
      CAFF_REPO_RUN_ROOT_RESULT="${safe_repo_link}"
      return 0
    fi
  done

  CAFF_REPO_RUN_ROOT_TEMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/caff-repo-link.XXXXXX")"
  ln -s "${repo_root}" "${CAFF_REPO_RUN_ROOT_TEMP_DIR}/calamity"
  CAFF_REPO_RUN_ROOT_RESULT="${CAFF_REPO_RUN_ROOT_TEMP_DIR}/calamity"
}

cleanup_repo_run_root() {
  if [[ -n "${CAFF_REPO_RUN_ROOT_TEMP_DIR:-}" ]] && [[ -d "${CAFF_REPO_RUN_ROOT_TEMP_DIR}" ]]; then
    rm -rf "${CAFF_REPO_RUN_ROOT_TEMP_DIR}"
  fi
}

# Prints the package version (without a leading "v"). Shared by build_mo2_zip.sh
# and release_verify.sh so the zip name and the expected version cannot drift.
# Precedence:
#   1) CAFF_PACKAGE_VERSION
#   2) a v* tag on HEAD; a final tag (no pre-release suffix) wins over rc tags
#   3) the CMake project version
resolve_package_version() {
  local repo_root="$1"
  local head_tags=""
  local tag=""
  local cmake_project_file="${repo_root}/skse/CalamityAffixes/CMakeLists.txt"
  local cmake_version=""

  if [[ -n "${CAFF_PACKAGE_VERSION:-}" ]]; then
    printf '%s\n' "${CAFF_PACKAGE_VERSION#v}"
    return 0
  fi

  if command -v git >/dev/null 2>&1; then
    head_tags="$(git -C "${repo_root}" tag --points-at HEAD --list 'v*' 2>/dev/null || true)"
    if [[ -n "${head_tags}" ]]; then
      # `sort -V` orders v2.3.0 before v2.3.0-rc4, so prefer tags without a
      # pre-release/build suffix before falling back to the newest rc tag.
      tag="$(printf '%s\n' "${head_tags}" | grep -Ev -- '[-+]' | sort -V | tail -n1 || true)"
      if [[ -z "${tag}" ]]; then
        tag="$(printf '%s\n' "${head_tags}" | sort -V | tail -n1 || true)"
      fi
      if [[ -n "${tag}" ]]; then
        printf '%s\n' "${tag#v}"
        return 0
      fi
    fi
  fi

  if [[ -f "${cmake_project_file}" ]]; then
    cmake_version="$(grep -Eo 'project\(CalamityAffixes VERSION [0-9]+\.[0-9]+\.[0-9]+' "${cmake_project_file}" | awk '{print $3}' | head -n1 || true)"
  fi
  if [[ -z "${cmake_version}" ]]; then
    echo "Could not resolve CalamityAffixes package version from ${cmake_project_file}" >&2
    return 1
  fi
  printf '%s\n' "${cmake_version}"
}
