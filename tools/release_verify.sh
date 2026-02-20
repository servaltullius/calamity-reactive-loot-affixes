#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
repo_run_root="${repo_root}"

# Windows-hosted dotnet can break on WSL paths containing spaces.
# Prefer the existing no-space symlink path when it points to the same repo.
safe_repo_link="$(dirname "${repo_root}")/calamity"
if [[ "${repo_root}" == *" "* && -d "${safe_repo_link}" ]]; then
  if [[ "$(realpath "${safe_repo_link}")" == "$(realpath "${repo_root}")" ]]; then
    repo_run_root="${safe_repo_link}"
  fi
fi

show_help() {
  cat <<'EOF'
Usage: tools/release_verify.sh [options]

Runs the "static" verification chain that we expect before shipping a release.

Options:
  --fast          Skip slower steps (vibe doctor, cmake build) but still run core checks.
  --strict        Fail if SKSE build dir is missing (default: skip SKSE checks when missing).
  --with-mo2-zip  Also run tools/build_mo2_zip.sh (requires Papyrus compiler + Scripts.zip paths).
  -h, --help      Show this help.

Environment:
  JOBS=<n>         Parallel jobs for cmake builds (default: nproc or 4).
EOF
}

fast=0
strict=0
with_mo2_zip=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --fast)
      fast=1
      shift
      ;;
    --strict)
      strict=1
      shift
      ;;
    --with-mo2-zip)
      with_mo2_zip=1
      shift
      ;;
    -h|--help)
      show_help
      exit 0
      ;;
    *)
      echo "Unknown arg: $1" >&2
      show_help >&2
      exit 2
      ;;
  esac
done

jobs="${JOBS:-}"
if [[ -z "${jobs}" ]]; then
  if command -v nproc >/dev/null 2>&1; then
    jobs="$(nproc)"
  else
    jobs="4"
  fi
fi

step() {
  echo ""
  echo "==> $*"
}

step "Repo root: ${repo_root}"
if [[ "${repo_run_root}" != "${repo_root}" ]]; then
  step "Using no-space path for dotnet: ${repo_run_root}"
fi

spec_manifest="${repo_root}/affixes/affixes.modules.json"
spec_json="${repo_root}/affixes/affixes.json"
if [[ -f "${spec_manifest}" ]]; then
  step "Compose modular affix spec"
  python3 "${repo_root}/tools/compose_affixes.py" \
    --manifest "${spec_manifest}" \
    --output "${spec_json}"
fi

step "vibe-kit doctor (optional)"
if [[ "${fast}" -eq 1 ]]; then
  echo "(skipped: --fast)"
else
  python3 "${repo_root}/scripts/vibe.py" doctor --full
fi

step "Spec lint + generated runtime config sync"
python3 "${repo_root}/tools/lint_affixes.py" \
  --spec "${repo_root}/affixes/affixes.json" \
  --manifest "${repo_root}/affixes/affixes.modules.json" \
  --generated "${repo_root}/Data/SKSE/Plugins/CalamityAffixes/affixes.json"

step "MCM JSON sanity"
python3 -m json.tool "${repo_root}/Data/MCM/Config/CalamityAffixes/config.json" >/dev/null
python3 -m json.tool "${repo_root}/Data/MCM/Config/CalamityAffixes/keybinds.json" >/dev/null
echo "MCM config/keybinds: OK"

step "Generator tests"
(
  cd "${repo_run_root}"
  dotnet test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release
)

step "UserPatch CLI build"
(
  cd "${repo_run_root}"
  dotnet build tools/CalamityAffixes.UserPatch/CalamityAffixes.UserPatch.csproj -c Release
)

skse_build_dir="${repo_root}/skse/CalamityAffixes/build.linux-clangcl-rel"
if [[ -d "${skse_build_dir}" ]]; then
  step "SKSE static checks (cmake + ctest)"
  if [[ "${fast}" -eq 1 ]]; then
    echo "(cmake build skipped: --fast)"
  else
    cmake --build "${skse_build_dir}" --parallel "${jobs}"
  fi
  ctest --test-dir "${skse_build_dir}" --output-on-failure
else
  msg="SKSE build dir missing: ${skse_build_dir}"
  if [[ "${strict}" -eq 1 ]]; then
    echo "${msg}" >&2
    exit 1
  fi
  echo "${msg} (skipped)"
fi

if [[ "${with_mo2_zip}" -eq 1 ]]; then
  step "MO2 zip build"
  "${repo_root}/tools/build_mo2_zip.sh"
else
  step "MO2 zip build"
  echo "(skipped: pass --with-mo2-zip to run packaging)"
fi

echo ""
echo "All selected verification steps completed successfully."
