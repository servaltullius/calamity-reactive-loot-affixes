#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "${repo_root}/tools/path_helpers.sh"
repo_run_root="$(resolve_repo_run_root "${repo_root}")"
trap cleanup_repo_run_root EXIT

show_help() {
  cat <<'EOF'
Usage: tools/release_verify.sh [options]

Runs the "static" verification chain that we expect before shipping a release.

Options:
  --fast          Skip slower steps (vibe doctor, full default build) but still compile CalamityAffixes and run core checks.
  --strict        Fail if SKSE build dir is missing.
  --no-strict     Allow SKSE checks to be skipped when build dir is missing.
  --with-mo2-zip  Also run tools/build_mo2_zip.sh (requires Papyrus compiler + Scripts.zip paths).
  -h, --help      Show this help.

Environment:
  JOBS=<n>         Parallel jobs for cmake builds (default: nproc or 4).
  CAFF_REPO_RUN_ROOT=<path>
                   Optional no-space alias for the repo root when dotnet has trouble with spaced paths.
EOF
}

fast=0
strict=1
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
    --no-strict)
      strict=0
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
dotnet_cmd="${CAFF_DOTNET:-dotnet}"
if [[ -z "${jobs}" ]]; then
  if command -v nproc >/dev/null 2>&1; then
    jobs="$(nproc)"
  else
    jobs="4"
  fi
fi

python3 "${repo_root}/tools/ensure_skse_build.py" --lane plugin --lane runtime-gate

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
  step "Check modular affix spec composition"
  python3 "${repo_root}/tools/compose_affixes.py" \
    --manifest "${spec_manifest}" \
    --check
fi

step "vibe-kit doctor (optional)"
if [[ "${fast}" -eq 1 ]]; then
  echo "(skipped: --fast)"
else
  python3 "${repo_root}/scripts/vibe.py" doctor --full
fi

step "Spec lint + generated runtime config sync"
python3 "${repo_root}/tools/lint_affixes.py" \
  --spec "${spec_json}" \
  --manifest "${repo_root}/affixes/affixes.modules.json" \
  --generated "${repo_root}/Data/SKSE/Plugins/CalamityAffixes/affixes.json"

step "tools workflow tests"
python3 -m unittest discover -s "${repo_root}/tools/tests" -p 'test_*.py'

step "MCM JSON sanity"
python3 -m json.tool "${repo_root}/Data/MCM/Config/CalamityAffixes/config.json" >/dev/null
python3 -m json.tool "${repo_root}/Data/MCM/Config/CalamityAffixes/keybinds.json" >/dev/null
echo "MCM config/keybinds: OK"

step "Runtime contract snapshot sync"
python3 "${repo_root}/tools/verify_runtime_contract_sync.py"

step "Generator tests"
(
  cd "${repo_run_root}"
  "${dotnet_cmd}" test tools/CalamityAffixes.Generator.Tests/CalamityAffixes.Generator.Tests.csproj -c Release
)

skse_build_dir="${repo_root}/skse/CalamityAffixes/build.linux-clangcl-rel"
if [[ -d "${skse_build_dir}" ]]; then
  step "SKSE static checks (cmake + ctest)"
  if [[ "${fast}" -eq 1 ]]; then
    cmake --build "${skse_build_dir}" --target CalamityAffixes --parallel "${jobs}"
  else
    cmake --build "${skse_build_dir}" --parallel "${jobs}"
  fi
  ctest --test-dir "${skse_build_dir}" --no-tests=error --output-on-failure
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
