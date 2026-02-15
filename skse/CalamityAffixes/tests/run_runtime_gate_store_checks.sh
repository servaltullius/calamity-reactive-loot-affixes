#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd -- "${script_dir}/.." && pwd)"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "${tmp_dir}"' EXIT

cxx="${CXX:-c++}"
if ! command -v "${cxx}" >/dev/null 2>&1; then
  echo "runtime_gate_store_checks: skipped (no host C++ compiler: ${cxx})"
  exit 0
fi

"${cxx}" \
  -std=c++23 \
  -O2 \
  -Wall \
  -Wextra \
  -pedantic \
  -I"${project_dir}/include" \
  "${script_dir}/runtime_gate_store_checks.cpp" \
  -o "${tmp_dir}/runtime_gate_store_checks"

"${tmp_dir}/runtime_gate_store_checks"
echo "runtime_gate_store_checks: OK"
