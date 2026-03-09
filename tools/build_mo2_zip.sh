#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
mod_name="CalamityAffixes"
source "${repo_root}/tools/path_helpers.sh"
repo_run_root="$(resolve_repo_run_root "${repo_root}")"
version="0.0.0"

# Version precedence for package naming:
# 1) explicit env override (CAFF_PACKAGE_VERSION)
# 2) exact git tag on HEAD (v*)
# 3) fallback to CMake project version
if [[ -n "${CAFF_PACKAGE_VERSION:-}" ]]; then
  version="${CAFF_PACKAGE_VERSION}"
elif command -v git >/dev/null 2>&1; then
  tagged_version="$(git -C "${repo_root}" tag --points-at HEAD --list 'v*' | sort -V | tail -n1 || true)"
  if [[ -n "${tagged_version}" ]]; then
    version="${tagged_version#v}"
  fi
fi

cmake_project_file="${repo_root}/skse/CalamityAffixes/CMakeLists.txt"
if [[ "${version}" == "0.0.0" && -f "${cmake_project_file}" ]]; then
  detected_version="$(grep -Eo 'project\(CalamityAffixes VERSION [0-9]+\.[0-9]+\.[0-9]+' "${cmake_project_file}" | awk '{print $3}' | head -n1)"
  if [[ -n "${detected_version}" ]]; then
    version="${detected_version}"
  fi
fi

data_dir="${repo_root}/Data"
dist_dir="${repo_root}/dist"
repo_data_dll="${data_dir}/SKSE/Plugins/CalamityAffixes.dll"
skse_build_dir_default="${repo_root}/skse/CalamityAffixes/build.linux-clangcl-rel"
linux_cross_dll_default="${skse_build_dir_default}/CalamityAffixes.dll"
skse_build_dir="${CAFF_SKSE_BUILD_DIR:-${skse_build_dir_default}}"
linux_cross_dll="${CAFF_LINUX_CROSS_DLL:-${linux_cross_dll_default}}"
spec_manifest="${repo_root}/affixes/affixes.modules.json"
spec_json="${repo_root}/affixes/affixes.json"
tmp_dir="$(mktemp -d)"
cleanup() {
  cleanup_repo_run_root
  rm -rf "${tmp_dir}"
}
trap cleanup EXIT

jobs="${JOBS:-}"
dotnet_cmd="${CAFF_DOTNET:-dotnet}"
if [[ -z "${jobs}" ]]; then
  if command -v nproc >/dev/null 2>&1; then
    jobs="$(nproc)"
  else
    jobs="4"
  fi
fi

stage_root="${tmp_dir}/stage"
stage_data_dir="${stage_root}/Data"
stage_dll="${stage_data_dir}/SKSE/Plugins/CalamityAffixes.dll"

skse_build_dir_run="${skse_build_dir}"
if [[ -z "${CAFF_SKSE_BUILD_DIR:-}" && "${repo_run_root}" != "${repo_root}" ]]; then
  skse_build_dir_run="${repo_run_root}/skse/CalamityAffixes/build.linux-clangcl-rel"
fi

if [[ -d "${skse_build_dir}" ]]; then
  python3 "${repo_root}/tools/ensure_skse_build.py" --lane plugin
  cmake --build "${skse_build_dir_run}" --target CalamityAffixes --parallel "${jobs}"
else
  echo "SKSE build dir missing: ${skse_build_dir} (using staged Data DLL if present)" >&2
fi

# Refuse to package from a stale modular spec snapshot, but do not rewrite tracked files.
if [[ -f "${spec_manifest}" ]]; then
  python3 "${repo_root}/tools/compose_affixes.py" \
    --manifest "${spec_manifest}" \
    --check
fi

# Stage Data/* first so packaging never mutates tracked repository outputs.
mkdir -p "${stage_data_dir}"
cp -a "${data_dir}/." "${stage_data_dir}/"

if [[ -f "${linux_cross_dll}" ]]; then
  mkdir -p "$(dirname "${stage_dll}")"
  cp -f "${linux_cross_dll}" "${stage_dll}"
  echo "Staged DLL: ${linux_cross_dll} -> ${stage_dll}"
elif [[ -f "${stage_dll}" ]]; then
  echo "Staged DLL: using ${repo_data_dll}"
else
  echo "No packaged DLL available: ${linux_cross_dll} or ${repo_data_dll}" >&2
  exit 1
fi

# Regenerate data-driven outputs (keywords/MCM plugin/runtime json) into staged Data.
generator_args=(
  --spec affixes/affixes.json
  --data "${stage_data_dir}"
)

(
  cd "${repo_run_root}"
  "${dotnet_cmd}" run --project tools/CalamityAffixes.Generator -- "${generator_args[@]}"
)

# Compile required Papyrus scripts into staged Data/Scripts/*.pex.
"${repo_root}/tools/compile_papyrus.sh" --data "${stage_data_dir}"

# Lint spec + ensure generated runtime config is up to date (prevents shipping stale Data/*).
python3 "${repo_root}/tools/lint_affixes.py" \
  --spec "${repo_root}/affixes/affixes.json" \
  --manifest "${repo_root}/affixes/affixes.modules.json" \
  --generated "${stage_data_dir}/SKSE/Plugins/CalamityAffixes/affixes.json"

date_tag="$(date +%Y-%m-%d)"
out_zip="${dist_dir}/${mod_name}_MO2_v${version}_${date_tag}.zip"

mkdir -p "${dist_dir}"
rm -f "${out_zip}"

mkdir -p "${tmp_dir}/${mod_name}"

# Copy Data/* into the MO2 mod root (do NOT nest under a Data/ folder).
cp -a "${stage_data_dir}/." "${tmp_dir}/${mod_name}/"

# Include docs for CK setup / debugging.
mkdir -p "${tmp_dir}/${mod_name}/Docs"
cp -a "${repo_root}/README.md" "${tmp_dir}/${mod_name}/Docs/README.md"
cp -a "${repo_root}/docs/design/개발명세서.md" "${tmp_dir}/${mod_name}/Docs/개발명세서.md"
cp -a "${repo_root}/docs/design/CK_MVP_셋업_체크리스트.md" "${tmp_dir}/${mod_name}/Docs/CK_MVP_셋업_체크리스트.md"
cp -a "${repo_root}/docs/design/데이터주도_생성기_워크플로우.md" "${tmp_dir}/${mod_name}/Docs/데이터주도_생성기_워크플로우.md"

(
  cd "${tmp_dir}"
  zip -r "${out_zip}" "${mod_name}" >/dev/null
)

echo "Wrote: ${out_zip}"
