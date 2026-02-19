#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd -- "${script_dir}/.." && pwd)"
repo_root="$(cd -- "${project_dir}/../.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

cxx="${CXX:-c++}"
if ! command -v "${cxx}" >/dev/null 2>&1; then
  echo "runtime_gate_store_checks: skipped (no host C++ compiler: ${cxx})"
  exit 0
fi

contract_path="${repo_root}/Data/SKSE/Plugins/CalamityAffixes/runtime_contract.json"
if [[ ! -f "${contract_path}" ]]; then
  if ! command -v dotnet >/dev/null 2>&1; then
    echo "runtime_gate_store_checks: failed (missing runtime_contract.json and dotnet is unavailable)"
    exit 1
  fi

  generated_data_dir="${tmp_dir}/generated_data"
  (
    cd "${repo_root}"
    dotnet run --project tools/CalamityAffixes.Generator -- \
      --spec affixes/affixes.json \
      --data "${generated_data_dir}" >/dev/null
  )

  generated_contract_path="${generated_data_dir}/SKSE/Plugins/CalamityAffixes/runtime_contract.json"
  if [[ ! -f "${generated_contract_path}" ]]; then
    echo "runtime_gate_store_checks: failed (generator did not produce runtime_contract.json)"
    exit 1
  fi
  contract_path="${generated_contract_path}"
fi
export CAFF_RUNTIME_CONTRACT_PATH="${contract_path}"

sources=(
  "${script_dir}/runtime_gate_store_checks_main.cpp"
  "${script_dir}/runtime_gate_store_checks_runtime.cpp"
  "${script_dir}/runtime_gate_store_checks_loot_policy.cpp"
  "${script_dir}/runtime_gate_store_checks_runeword_policy.cpp"
  "${script_dir}/runtime_gate_store_checks_settings_policy.cpp"
)

"${cxx}" \
  -std=c++23 \
  -O2 \
  -Wall \
  -Wextra \
  -pedantic \
  -I"${project_dir}/include" \
  -I"${project_dir}/extern/vendor/include" \
  "${sources[@]}" \
  -o "${tmp_dir}/runtime_gate_store_checks"

"${tmp_dir}/runtime_gate_store_checks"
echo "runtime_gate_store_checks: OK"
