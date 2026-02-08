#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
data_dir="${repo_root}/Data"

papyrus_compiler="${PAPYRUS_COMPILER_EXE:-/mnt/c/Program Files (x86)/Steam/steamapps/content/app_1946180/depot_1946183/Papyrus Compiler/PapyrusCompiler.exe}"
scripts_zip="${PAPYRUS_SCRIPTS_ZIP:-/mnt/c/Program Files (x86)/Steam/steamapps/content/app_1946180/depot_1946183/Data/Scripts.zip}"
cache_dir="${PAPYRUS_CACHE_DIR:-${repo_root}/.cache/papyrus/ck}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --data)
      data_dir="$2"
      shift 2
      ;;
    --compiler)
      papyrus_compiler="$2"
      shift 2
      ;;
    --scripts-zip)
      scripts_zip="$2"
      shift 2
      ;;
    --cache-dir)
      cache_dir="$2"
      shift 2
      ;;
    -h|--help)
      cat <<'EOF'
Usage: tools/compile_papyrus.sh [options]

Options:
  --data <path>         Data directory to compile into (default: repo Data/)
  --compiler <path>     PapyrusCompiler.exe path
  --scripts-zip <path>  CK Scripts.zip path (for vanilla .psc + flags)
  --cache-dir <path>    Cache directory for extracted scripts
EOF
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

if [[ ! -f "${papyrus_compiler}" ]]; then
  echo "Papyrus compiler not found: ${papyrus_compiler}" >&2
  echo "Set PAPYRUS_COMPILER_EXE or pass --compiler." >&2
  exit 1
fi

if [[ ! -f "${scripts_zip}" ]]; then
  echo "Scripts.zip not found: ${scripts_zip}" >&2
  echo "Set PAPYRUS_SCRIPTS_ZIP or pass --scripts-zip." >&2
  exit 1
fi

src_dir="${data_dir}/Scripts/Source"
out_dir="${data_dir}/Scripts"
stub_dir="${repo_root}/tools/papyrus-stubs"
flags_file="${stub_dir}/Calamity_Papyrus_Flags.flg"
vanilla_src_dir="${cache_dir}/Source/Scripts"

if [[ ! -d "${src_dir}" ]]; then
  echo "Papyrus source dir missing: ${src_dir}" >&2
  exit 1
fi

mkdir -p "${out_dir}" "${cache_dir}"

if [[ ! -f "${vanilla_src_dir}/Quest.psc" ]]; then
  unzip -oq "${scripts_zip}" 'Source/Scripts/*' -d "${cache_dir}"
fi

if [[ ! -f "${vanilla_src_dir}/Quest.psc" ]]; then
  echo "Failed to locate vanilla script sources after extraction: ${vanilla_src_dir}" >&2
  exit 1
fi

if [[ ! -f "${flags_file}" ]]; then
  echo "Missing custom flags file: ${flags_file}" >&2
  exit 1
fi

if ! command -v wslpath >/dev/null 2>&1; then
  echo "This script requires wslpath (WSL environment)." >&2
  exit 1
fi

targets=(
  "CalamityAffixes_ModeControl.psc"
  "CalamityAffixes_ModEventEmitter.psc"
  "CalamityAffixes_MCMConfig.psc"
)

for script in "${targets[@]}"; do
  if [[ ! -f "${src_dir}/${script}" ]]; then
    echo "Missing papyrus source: ${src_dir}/${script}" >&2
    exit 1
  fi
done

imports="$(wslpath -w "${src_dir}");$(wslpath -w "${stub_dir}");$(wslpath -w "${vanilla_src_dir}")"
flags_win="$(wslpath -w "${flags_file}")"
out_win="$(wslpath -w "${out_dir}")"

stale_outputs=(
  "CalamityAffixes_AffixEffectBase.pex"
  "CalamityAffixes_AffixManager.pex"
  "CalamityAffixes_PlayerAlias.pex"
  "CalamityAffixes_SkseBridge.pex"
)
for stale in "${stale_outputs[@]}"; do
  rm -f "${out_dir}/${stale}"
done

pushd "${src_dir}" >/dev/null
for script in "${targets[@]}"; do
  "${papyrus_compiler}" "${script}" "-i=${imports}" "-o=${out_win}" "-f=${flags_win}" -op
done
popd >/dev/null

echo "Compiled Papyrus scripts to: ${out_dir}"
