#!/usr/bin/env python3
from __future__ import annotations

import datetime as dt
import os
import shutil
import subprocess
import tempfile
import unittest
import zipfile
from pathlib import Path


class ReleaseVerifyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        cls.release_verify_path = cls.repo_root / "tools" / "release_verify.sh"
        cls.build_mo2_zip_path = cls.repo_root / "tools" / "build_mo2_zip.sh"
        cls.runtime_contract_sync_path = cls.repo_root / "tools" / "verify_runtime_contract_sync.py"
        cls.ci_verify_path = cls.repo_root / ".github" / "workflows" / "ci-verify.yml"
        cls.repo_data_dll = cls.repo_root / "Data" / "SKSE" / "Plugins" / "CalamityAffixes.dll"

    def test_runtime_contract_sync_script_passes_for_repo_snapshot(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-fake-dotnet-contract-") as temp_dir:
            temp_root = Path(temp_dir)
            fake_dotnet = self._write_fake_dotnet(temp_root / "bin")
            result = subprocess.run(
                ["python3", str(self.runtime_contract_sync_path)],
                cwd=self.repo_root,
                text=True,
                capture_output=True,
                check=False,
                env={
                    **os.environ,
                    "CAFF_DOTNET": str(fake_dotnet),
                    "CAFF_TEST_REPO_ROOT": str(self.repo_root),
                },
            )
            self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
            self.assertIn("runtime_contract sync: OK", result.stdout)

    def test_release_verify_invokes_runtime_contract_sync(self) -> None:
        source = self.release_verify_path.read_text(encoding="utf-8")
        self.assertIn("verify_runtime_contract_sync.py", source)

    def test_release_verify_fast_mode_still_builds_plugin_target(self) -> None:
        source = self.release_verify_path.read_text(encoding="utf-8")
        self.assertIn('cmake --build "${skse_build_dir}" --target CalamityAffixes --parallel "${jobs}"', source)

    def test_release_verify_checks_modular_spec_without_rewriting_tracked_affixes_json(self) -> None:
        source = self.release_verify_path.read_text(encoding="utf-8")
        self.assertIn("--check", source)
        self.assertNotIn('--output "${spec_json}"', source)

    def test_release_verify_ensures_skse_build_lanes_before_checks(self) -> None:
        source = self.release_verify_path.read_text(encoding="utf-8")
        self.assertIn('python3 "${repo_root}/tools/ensure_skse_build.py" --lane plugin --lane runtime-gate', source)

    def test_ci_verify_runs_tools_workflow_tests(self) -> None:
        source = self.ci_verify_path.read_text(encoding="utf-8")
        self.assertIn("python3 -m unittest discover -s tools/tests -p 'test_*.py'", source)

    def test_ci_verify_runs_runtime_contract_sync(self) -> None:
        source = self.ci_verify_path.read_text(encoding="utf-8")
        self.assertIn("python3 tools/verify_runtime_contract_sync.py", source)

    def test_plugin_version_comes_from_cmake_generated_header(self) -> None:
        cmake_source = (self.repo_root / "skse" / "CalamityAffixes" / "CMakeLists.txt").read_text(encoding="utf-8")
        main_source = (self.repo_root / "skse" / "CalamityAffixes" / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertIn("configure_file(", cmake_source)
        self.assertIn("generated/CalamityAffixes/Version.h", cmake_source)
        self.assertIn('#include "CalamityAffixes/Version.h"', main_source)
        self.assertIn("CALAMITYAFFIXES_VERSION_MAJOR", main_source)

    def test_build_mo2_zip_rebuilds_fresh_plugin_target(self) -> None:
        source = self.build_mo2_zip_path.read_text(encoding="utf-8")
        self.assertIn('python3 "${repo_root}/tools/ensure_skse_build.py" --lane plugin', source)
        self.assertIn("cmake --build", source)
        self.assertIn("--target CalamityAffixes", source)

    def test_build_mo2_zip_checks_modular_spec_without_rewriting_tracked_affixes_json(self) -> None:
        source = self.build_mo2_zip_path.read_text(encoding="utf-8")
        self.assertIn("--check", source)
        self.assertNotIn('--output "${spec_json}"', source)

    def test_fake_papyrus_compiler_handles_posix_output_path_without_wslpath(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-fake-papyrus-") as temp_dir:
            temp_root = Path(temp_dir)
            env = self._build_mo2_zip_env(temp_root=temp_root, package_version="fake-papyrus")
            compiler = Path(env["PAPYRUS_COMPILER_EXE"])
            out_dir = temp_root / "compiled"
            helper_bin = temp_root / "helper-bin"
            helper_bin.mkdir(parents=True, exist_ok=True)

            for tool in ("bash", "mkdir", "touch", "python3"):
                tool_path = shutil.which(tool)
                self.assertIsNotNone(tool_path, f"required host tool missing for fake compiler smoke: {tool}")
                (helper_bin / tool).symlink_to(tool_path)

            result = subprocess.run(
                [str(compiler), "ExampleScript.psc", f"-o={out_dir}"],
                text=True,
                capture_output=True,
                check=False,
                env={**os.environ, "PATH": str(helper_bin)},
            )
            self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
            self.assertTrue((out_dir / "ExampleScript.pex").is_file(), "expected fake compiler to emit .pex output")

    def test_build_mo2_zip_uses_no_space_build_path_for_cmake(self) -> None:
        if " " not in str(self.repo_root):
            self.skipTest("repo path does not require no-space cmake build routing")
        self.assertTrue(self.repo_data_dll.is_file(), "expected checked-in Data DLL for packaging smoke test")

        package_version = "test-safe-path"
        out_zip = self._package_zip_path(package_version)
        with tempfile.TemporaryDirectory(prefix="caff-build-mock-") as temp_dir:
            temp_root = Path(temp_dir)
            cmake_log = temp_root / "cmake-build-path.txt"
            self._write_fake_cmake(temp_root / "bin", fail_on_space=True)
            fake_dotnet = self._write_fake_dotnet(temp_root / "bin")
            env = self._build_mo2_zip_env(
                temp_root=temp_root,
                package_version=package_version,
                extra_env={
                    "PATH": f"{temp_root / 'bin'}:{os.environ['PATH']}",
                    "CAFF_TEST_CMAKE_LOG": str(cmake_log),
                    "CAFF_DOTNET": str(fake_dotnet),
                    "CAFF_TEST_REPO_ROOT": str(self.repo_root),
                },
            )

            try:
                result = subprocess.run(
                    [str(self.build_mo2_zip_path)],
                    cwd=self.repo_root,
                    text=True,
                    capture_output=True,
                    check=False,
                    env=env,
                )
                self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
                self.assertTrue(cmake_log.is_file(), "expected fake cmake to capture build path")
                build_path = cmake_log.read_text(encoding="utf-8").strip()
                self.assertTrue(build_path, "expected cmake --build path to be recorded")
                self.assertNotIn(" ", build_path, "cmake --build should use a no-space repo alias path")
            finally:
                out_zip.unlink(missing_ok=True)

    def test_build_mo2_zip_falls_back_to_data_dll_when_build_dir_missing(self) -> None:
        self.assertTrue(self.repo_data_dll.is_file(), "expected checked-in Data DLL for fallback packaging test")

        package_version = "test-build-fallback"
        out_zip = self._package_zip_path(package_version)
        with tempfile.TemporaryDirectory(prefix="caff-build-fallback-") as temp_dir:
            temp_root = Path(temp_dir)
            self._write_fake_cmake(temp_root / "bin", fail_on_call=True)
            fake_dotnet = self._write_fake_dotnet(temp_root / "bin")
            env = self._build_mo2_zip_env(
                temp_root=temp_root,
                package_version=package_version,
                extra_env={
                    "PATH": f"{temp_root / 'bin'}:{os.environ['PATH']}",
                    "CAFF_SKSE_BUILD_DIR": str(temp_root / "missing-build-dir"),
                    "CAFF_LINUX_CROSS_DLL": str(temp_root / "missing-build-dir" / "CalamityAffixes.dll"),
                    "CAFF_DOTNET": str(fake_dotnet),
                    "CAFF_TEST_REPO_ROOT": str(self.repo_root),
                },
            )

            try:
                result = subprocess.run(
                    [str(self.build_mo2_zip_path)],
                    cwd=self.repo_root,
                    text=True,
                    capture_output=True,
                    check=False,
                    env=env,
                )
                self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
                self.assertTrue(out_zip.is_file(), "expected MO2 zip output to be created")
                with zipfile.ZipFile(out_zip) as archive:
                    dll_path = "CalamityAffixes/SKSE/Plugins/CalamityAffixes.dll"
                    self.assertIn(dll_path, archive.namelist())
                    self.assertEqual(archive.read(dll_path), self.repo_data_dll.read_bytes())
            finally:
                out_zip.unlink(missing_ok=True)

    def _build_mo2_zip_env(
        self,
        *,
        temp_root: Path,
        package_version: str,
        extra_env: dict[str, str] | None = None,
    ) -> dict[str, str]:
        env = os.environ.copy()
        env["CAFF_PACKAGE_VERSION"] = package_version

        fake_compiler = temp_root / "fake_papyrus_compiler.sh"
        fake_compiler.write_text(
            "#!/usr/bin/env bash\n"
            "set -euo pipefail\n"
            "script_name=\"$1\"\n"
            "shift\n"
            "out_dir=\"\"\n"
            "for arg in \"$@\"; do\n"
            "  if [[ \"$arg\" == -o=* ]]; then\n"
            "    raw_out_dir=\"${arg#-o=}\"\n"
            "    if command -v wslpath >/dev/null 2>&1; then\n"
            "      out_dir=\"$(wslpath -u \"$raw_out_dir\")\"\n"
            "    else\n"
            "      out_dir=\"$(python3 -c 'import pathlib,sys; print(pathlib.Path(sys.argv[1]).as_posix())' \"$raw_out_dir\")\"\n"
            "    fi\n"
            "  fi\n"
            "done\n"
            "if [[ -z \"$out_dir\" ]]; then\n"
            "  echo \"missing -o output dir\" >&2\n"
            "  exit 2\n"
            "fi\n"
            "mkdir -p \"$out_dir\"\n"
            "touch \"$out_dir/${script_name%.psc}.pex\"\n",
            encoding="utf-8",
        )
        fake_compiler.chmod(0o755)

        scripts_zip = temp_root / "Scripts.zip"
        with zipfile.ZipFile(scripts_zip, "w") as archive:
            archive.writestr("Source/Scripts/Quest.psc", "Scriptname Quest\n")

        env["PAPYRUS_COMPILER_EXE"] = str(fake_compiler)
        env["PAPYRUS_SCRIPTS_ZIP"] = str(scripts_zip)
        env["PAPYRUS_CACHE_DIR"] = str(temp_root / "papyrus-cache")

        if extra_env:
            env.update(extra_env)
        return env

    def _package_zip_path(self, package_version: str) -> Path:
        return self.repo_root / "dist" / f"CalamityAffixes_MO2_v{package_version}_{dt.date.today():%Y-%m-%d}.zip"

    def _write_fake_cmake(self, bin_dir: Path, *, fail_on_space: bool = False, fail_on_call: bool = False) -> None:
        bin_dir.mkdir(parents=True, exist_ok=True)
        cmake_path = bin_dir / "cmake"
        cmake_path.write_text(
            "#!/usr/bin/env bash\n"
            "set -euo pipefail\n"
            "if [[ \"${CAFF_TEST_CMAKE_FAIL_ON_CALL:-0}\" == \"1\" ]]; then\n"
            "  echo \"cmake should not have been called\" >&2\n"
            "  exit 99\n"
            "fi\n"
            "build_path=\"\"\n"
            "while [[ $# -gt 0 ]]; do\n"
            "  if [[ \"$1\" == \"--build\" ]]; then\n"
            "    build_path=\"$2\"\n"
            "    break\n"
            "  fi\n"
            "  shift\n"
            "done\n"
            "if [[ -n \"${CAFF_TEST_CMAKE_LOG:-}\" ]]; then\n"
            "  printf '%s' \"$build_path\" > \"${CAFF_TEST_CMAKE_LOG}\"\n"
            "fi\n"
            "if [[ \"${CAFF_TEST_CMAKE_FAIL_ON_SPACE:-0}\" == \"1\" && \"$build_path\" == *\" \"* ]]; then\n"
            "  echo \"space path not allowed: $build_path\" >&2\n"
            "  exit 9\n"
            "fi\n",
            encoding="utf-8",
        )
        if fail_on_space:
            extra = "export CAFF_TEST_CMAKE_FAIL_ON_SPACE=1\n"
        elif fail_on_call:
            extra = "export CAFF_TEST_CMAKE_FAIL_ON_CALL=1\n"
        else:
            extra = ""
        cmake_path.write_text(cmake_path.read_text(encoding="utf-8") + extra + "exit 0\n", encoding="utf-8")
        cmake_path.chmod(0o755)

    def _write_fake_dotnet(self, bin_dir: Path) -> Path:
        bin_dir.mkdir(parents=True, exist_ok=True)
        dotnet_path = bin_dir / "dotnet"
        dotnet_path.write_text(
            "#!/usr/bin/env bash\n"
            "set -euo pipefail\n"
            "repo_root=\"${CAFF_TEST_REPO_ROOT:?}\"\n"
            "if [[ \"${1:-}\" == \"--version\" ]]; then\n"
            "  echo \"9.9.9-test\"\n"
            "  exit 0\n"
            "fi\n"
            "if [[ \"${1:-}\" == \"test\" ]]; then\n"
            "  exit 0\n"
            "fi\n"
            "if [[ \"${1:-}\" != \"run\" ]]; then\n"
            "  echo \"unsupported fake dotnet invocation: $*\" >&2\n"
            "  exit 3\n"
            "fi\n"
            "shift\n"
            "out_dir=\"\"\n"
            "while [[ $# -gt 0 ]]; do\n"
            "  if [[ \"$1\" == \"--data\" ]]; then\n"
            "    out_dir=\"$2\"\n"
            "    shift 2\n"
            "    continue\n"
            "  fi\n"
            "  shift\n"
            "done\n"
            "if [[ -z \"$out_dir\" ]]; then\n"
            "  echo \"missing --data output dir\" >&2\n"
            "  exit 4\n"
            "fi\n"
            "if [[ \"$out_dir\" != /* ]]; then\n"
            "  out_dir=\"$PWD/$out_dir\"\n"
            "fi\n"
            "mkdir -p \"$out_dir/SKSE/Plugins/CalamityAffixes\"\n"
            "cp \"$repo_root/Data/SKSE/Plugins/CalamityAffixes/runtime_contract.json\" "
            "\"$out_dir/SKSE/Plugins/CalamityAffixes/runtime_contract.json\"\n"
            "cp \"$repo_root/Data/SKSE/Plugins/CalamityAffixes/affixes.json\" "
            "\"$out_dir/SKSE/Plugins/CalamityAffixes/affixes.json\"\n"
            "exit 0\n",
            encoding="utf-8",
        )
        dotnet_path.chmod(0o755)
        return dotnet_path


if __name__ == "__main__":
    unittest.main()
