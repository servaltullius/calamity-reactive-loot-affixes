#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
repo_run_root="${repo_root}"
safe_repo_link="$(dirname "${repo_root}")/calamity"

if [[ "${repo_root}" == *" "* && -d "${safe_repo_link}" ]]; then
  if [[ "$(realpath "${safe_repo_link}")" == "$(realpath "${repo_root}")" ]]; then
    repo_run_root="${safe_repo_link}"
  fi
fi

spec_path="${CAFF_SPEC_PATH:-affixes/affixes.json}"
output_path="${CAFF_USERPATCH_OUTPUT:-Data/CalamityAffixes_UserPatch.esp}"
masters_dir="${CALAMITY_MASTERS_DIR:-}"
loadorder_path="${CALAMITY_LOADORDER_PATH:-}"
spec_manifest_path="${CAFF_SPEC_MANIFEST:-affixes/affixes.modules.json}"
if [[ "${spec_manifest_path}" = /* ]]; then
  spec_manifest_abs="${spec_manifest_path}"
else
  spec_manifest_abs="${repo_root}/${spec_manifest_path}"
fi

if [[ -f "${spec_manifest_abs}" ]]; then
  python3 "${repo_root}/tools/compose_affixes.py" \
    --manifest "${spec_manifest_abs}" \
    --output "${repo_root}/affixes/affixes.json"
fi

if [[ -z "${masters_dir}" ]]; then
  echo "ERROR: CALAMITY_MASTERS_DIR is required." >&2
  echo "       Example: export CALAMITY_MASTERS_DIR='C:\\\\Games\\\\Skyrim\\\\Data'" >&2
  exit 1
fi

if [[ -z "${loadorder_path}" ]]; then
  echo "ERROR: CALAMITY_LOADORDER_PATH is required." >&2
  echo "       Example: export CALAMITY_LOADORDER_PATH='C:\\\\Users\\\\<user>\\\\AppData\\\\Local\\\\Skyrim Special Edition\\\\plugins.txt'" >&2
  exit 1
fi

dotnet_info="$(dotnet --info 2>/dev/null || true)"
dotnet_os_platform="$(printf '%s\n' "${dotnet_info}" | awk -F: '/OS Platform/ { gsub(/^[[:space:]]+/, "", $2); print $2; exit }')"
dotnet_os_platform="${dotnet_os_platform//$'\r'/}"
if [[ "${dotnet_os_platform}" == "Windows" ]]; then
  if command -v wslpath >/dev/null 2>&1; then
    if [[ "${masters_dir}" == /mnt/* ]]; then
      masters_dir="$(wslpath -w "${masters_dir}")"
    fi
    if [[ "${loadorder_path}" == /mnt/* ]]; then
      loadorder_path="$(wslpath -w "${loadorder_path}")"
    fi
    if [[ "${output_path}" == /mnt/* ]]; then
      output_path="$(wslpath -w "${output_path}")"
    fi
    if [[ "${spec_path}" == /mnt/* ]]; then
      spec_path="$(wslpath -w "${spec_path}")"
    fi
  fi
fi

(
  cd "${repo_run_root}"
  dotnet run --project tools/CalamityAffixes.UserPatch -- \
    --spec "${spec_path}" \
    --masters "${masters_dir}" \
    --loadorder "${loadorder_path}" \
    --output "${output_path}" \
    "$@"
)

echo "Wrote user patch: ${output_path}"
