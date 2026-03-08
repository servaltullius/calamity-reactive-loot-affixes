#!/usr/bin/env bash

resolve_repo_run_root() {
  local repo_root="$1"
  local override="${CAFF_REPO_RUN_ROOT:-}"
  local safe_repo_link=""
  local candidate_links=(
    "$(dirname "${repo_root}")/calamity"
    "$(dirname "${repo_root}")/projects/calamity"
  )

  if [[ -n "${override}" ]]; then
    if [[ ! -d "${override}" ]]; then
      echo "CAFF_REPO_RUN_ROOT is not a directory: ${override}" >&2
      return 1
    fi
    if [[ "$(realpath "${override}")" != "$(realpath "${repo_root}")" ]]; then
      echo "CAFF_REPO_RUN_ROOT must resolve to the repo root: ${override}" >&2
      return 1
    fi
    printf '%s\n' "${override}"
    return 0
  fi

  if [[ "${repo_root}" != *" "* ]]; then
    printf '%s\n' "${repo_root}"
    return 0
  fi

  for safe_repo_link in "${candidate_links[@]}"; do
    if [[ -d "${safe_repo_link}" ]] && [[ "$(realpath "${safe_repo_link}")" == "$(realpath "${repo_root}")" ]]; then
      printf '%s\n' "${safe_repo_link}"
      return 0
    fi
  done

  CAFF_REPO_RUN_ROOT_TEMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/caff-repo-link.XXXXXX")"
  ln -s "${repo_root}" "${CAFF_REPO_RUN_ROOT_TEMP_DIR}/calamity"
  printf '%s\n' "${CAFF_REPO_RUN_ROOT_TEMP_DIR}/calamity"
}

cleanup_repo_run_root() {
  if [[ -n "${CAFF_REPO_RUN_ROOT_TEMP_DIR:-}" ]] && [[ -d "${CAFF_REPO_RUN_ROOT_TEMP_DIR}" ]]; then
    rm -rf "${CAFF_REPO_RUN_ROOT_TEMP_DIR}"
  fi
}
