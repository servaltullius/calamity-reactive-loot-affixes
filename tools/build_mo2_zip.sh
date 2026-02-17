#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
mod_name="CalamityAffixes"
repo_run_root="${repo_root}"
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

# Windows-hosted dotnet can break on WSL paths containing spaces.
# Prefer the existing no-space symlink path when it points to the same repo.
safe_repo_link="$(dirname "${repo_root}")/calamity"
if [[ "${repo_root}" == *" "* && -d "${safe_repo_link}" ]]; then
  if [[ "$(realpath "${safe_repo_link}")" == "$(realpath "${repo_root}")" ]]; then
    repo_run_root="${safe_repo_link}"
  fi
fi

data_dir="${repo_root}/Data"
dist_dir="${repo_root}/dist"
data_dll="${data_dir}/SKSE/Plugins/CalamityAffixes.dll"
linux_cross_dll="${repo_root}/skse/CalamityAffixes/build.linux-clangcl-rel/CalamityAffixes.dll"

# Keep Data/SKSE/Plugins DLL in sync with latest Linux cross-build output when present.
# (Windows/VS builds can still overwrite Data directly; this only helps avoid stale MO2 zips on WSL.)
if [[ -f "${linux_cross_dll}" ]]; then
  mkdir -p "$(dirname "${data_dll}")"
  if [[ ! -f "${data_dll}" || "${linux_cross_dll}" -nt "${data_dll}" ]]; then
    cp -f "${linux_cross_dll}" "${data_dll}"
    echo "Synced DLL: ${linux_cross_dll} -> ${data_dll}"
  fi
fi

# Regenerate data-driven outputs (keywords/MCM plugin/runtime json) from spec.
(
  cd "${repo_run_root}"
  dotnet run --project tools/CalamityAffixes.Generator -- \
    --spec affixes/affixes.json \
    --data Data
)

# Compile required Papyrus scripts into Data/Scripts/*.pex.
"${repo_root}/tools/compile_papyrus.sh" --data "${data_dir}"

# Lint spec + ensure generated runtime config is up to date (prevents shipping stale Data/*).
python3 "${repo_root}/tools/lint_affixes.py" \
  --spec "${repo_root}/affixes/affixes.json" \
  --generated "${data_dir}/SKSE/Plugins/CalamityAffixes/affixes.json"

date_tag="$(date +%Y-%m-%d)"
out_zip="${dist_dir}/${mod_name}_MO2_v${version}_${date_tag}.zip"

mkdir -p "${dist_dir}"
rm -f "${out_zip}"

tmp_dir="$(mktemp -d)"
cleanup() { rm -rf "${tmp_dir}"; }
trap cleanup EXIT

mkdir -p "${tmp_dir}/${mod_name}"

# Copy Data/* into the MO2 mod root (do NOT nest under a Data/ folder).
cp -a "${data_dir}/." "${tmp_dir}/${mod_name}/"

# Include docs for CK setup / debugging.
mkdir -p "${tmp_dir}/${mod_name}/Docs"
cp -a "${repo_root}/README.md" "${tmp_dir}/${mod_name}/Docs/README.md"
cp -a "${repo_root}/doc/1.개발명세서.md" "${tmp_dir}/${mod_name}/Docs/1.개발명세서.md"
cp -a "${repo_root}/doc/2.CK_MVP_셋업_체크리스트.md" "${tmp_dir}/${mod_name}/Docs/2.CK_MVP_셋업_체크리스트.md"
cp -a "${repo_root}/doc/3.B_데이터주도_생성기_워크플로우.md" "${tmp_dir}/${mod_name}/Docs/3.B_데이터주도_생성기_워크플로우.md"

(
  cd "${tmp_dir}"
  zip -r "${out_zip}" "${mod_name}" >/dev/null
)

echo "Wrote: ${out_zip}"
