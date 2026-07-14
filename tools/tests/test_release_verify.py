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
        cls.verify_mo2_zip_path = cls.repo_root / "tools" / "verify_mo2_zip.py"
        cls.compile_papyrus_path = cls.repo_root / "tools" / "compile_papyrus.sh"
        cls.runtime_contract_sync_path = cls.repo_root / "tools" / "verify_runtime_contract_sync.py"
        cls.ci_verify_path = cls.repo_root / ".github" / "workflows" / "ci-verify.yml"
        cls.repo_build_dll = cls.repo_root / "skse" / "CalamityAffixes" / "build.linux-clangcl-rel" / "CalamityAffixes.dll"

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

    def test_release_verify_validates_the_reported_package_identity(self) -> None:
        source = self.release_verify_path.read_text(encoding="utf-8")
        self.assertIn('CAFF_PACKAGE_OUTPUT_FILE="${package_path_file}"', source)
        self.assertIn('tools/verify_mo2_zip.py', source)
        self.assertIn('--expected-dll "${expected_dll}"', source)
        self.assertIn('--expected-version "${expected_version}"', source)

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

    def test_build_mo2_zip_translates_stage_path_for_windows_dotnet(self) -> None:
        source = self.build_mo2_zip_path.read_text(encoding="utf-8")
        self.assertIn("dotnet_uses_windows_interop", source)
        self.assertIn('generator_data_dir="$(wslpath -w "${stage_data_dir}")"', source)
        self.assertIn('--data "${generator_data_dir}"', source)

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
        self.assertTrue(self.repo_build_dll.is_file(), "expected built DLL for packaging smoke test")

        package_version = "test-safe-path"
        out_zip = self._package_zip_path(package_version)
        with tempfile.TemporaryDirectory(prefix="caff-build-mock-") as temp_dir:
            temp_root = Path(temp_dir)
            cmake_log = temp_root / "cmake-build-path.txt"
            self._write_fake_cmake(temp_root / "bin", fail_on_space=True)
            fake_dotnet = self._write_fake_dotnet(temp_root / "bin")

            # This test isolates path routing. Let all Python tools run normally,
            # except the generated-data lint that may be stale during concurrent spec work.
            real_python = shutil.which("python3")
            self.assertIsNotNone(real_python)
            fake_python = temp_root / "bin" / "python3"
            fake_python.write_text(
                "#!/usr/bin/env bash\n"
                "if [[ \"${1:-}\" == */lint_affixes.py ]]; then exit 0; fi\n"
                f'exec "{real_python}" "$@"\n',
                encoding="utf-8",
            )
            fake_python.chmod(0o755)

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


    def test_build_mo2_zip_fails_when_build_dir_is_missing(self) -> None:
        package_version = "test-build-missing-dir"
        out_zip = self._package_zip_path(package_version)
        out_zip.unlink(missing_ok=True)

        with tempfile.TemporaryDirectory(prefix="caff-build-missing-") as temp_dir:
            temp_root = Path(temp_dir)
            self._write_fake_cmake(temp_root / "bin", fail_on_call=True)
            fake_dotnet = self._write_fake_dotnet(temp_root / "bin")
            missing_build_dir = temp_root / "missing-build-dir"
            env = self._build_mo2_zip_env(
                temp_root=temp_root,
                package_version=package_version,
                extra_env={
                    "PATH": f"{temp_root / 'bin'}:{os.environ['PATH']}",
                    "CAFF_SKSE_BUILD_DIR": str(missing_build_dir),
                    "CAFF_LINUX_CROSS_DLL": str(missing_build_dir / "CalamityAffixes.dll"),
                    "CAFF_DOTNET": str(fake_dotnet),
                    "CAFF_TEST_REPO_ROOT": str(self.repo_root),
                },
            )

            result = subprocess.run(
                [str(self.build_mo2_zip_path)],
                cwd=self.repo_root,
                text=True,
                capture_output=True,
                check=False,
                env=env,
            )

        self.assertNotEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("SKSE build dir missing", result.stderr)
        self.assertFalse(out_zip.exists(), "a missing build directory must not fall back to the tracked Data DLL")

    def test_compile_papyrus_refuses_staged_pex_after_compiler_failure(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-papyrus-fail-hard-") as temp_dir:
            temp_root = Path(temp_dir)
            data_dir = temp_root / "Data"
            source_dir = data_dir / "Scripts" / "Source"
            output_dir = data_dir / "Scripts"
            source_dir.mkdir(parents=True)
            target_stems = (
                "CalamityAffixes_ModeControl",
                "CalamityAffixes_ModEventEmitter",
                "CalamityAffixes_MCMConfig",
            )
            for stem in target_stems:
                shutil.copy2(
                    self.repo_root / "Data" / "Scripts" / "Source" / f"{stem}.psc",
                    source_dir / f"{stem}.psc",
                )
                (output_dir / f"{stem}.pex").write_bytes(b"stale-pex")

            compiler = temp_root / "fail-compiler.sh"
            compiler.write_text("#!/usr/bin/env bash\nexit 17\n", encoding="utf-8")
            compiler.chmod(0o755)
            scripts_zip = temp_root / "Scripts.zip"
            with zipfile.ZipFile(scripts_zip, "w") as archive:
                archive.writestr("Source/Scripts/Quest.psc", "Scriptname Quest\n")

            result = subprocess.run(
                [
                    str(self.compile_papyrus_path),
                    "--data",
                    str(data_dir),
                    "--compiler",
                    str(compiler),
                    "--scripts-zip",
                    str(scripts_zip),
                    "--cache-dir",
                    str(temp_root / "papyrus-cache"),
                ],
                cwd=self.repo_root,
                text=True,
                capture_output=True,
                check=False,
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertIn("refusing to reuse an existing PEX", result.stderr)
            for stem in target_stems:
                self.assertEqual((output_dir / f"{stem}.pex").read_bytes(), b"stale-pex")

    def test_verify_mo2_zip_accepts_exact_release_payload_and_matching_identity(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-verify-zip-valid-") as temp_dir:
            temp_root = Path(temp_dir)
            expected_dll = temp_root / "CalamityAffixes.dll"
            expected_dll.write_bytes(b"fresh-build-dll")
            zip_path = temp_root / "CalamityAffixes_MO2_v9.8.7_2026-07-14.zip"
            self._write_minimal_mo2_zip(zip_path, dll_bytes=expected_dll.read_bytes())

            result = subprocess.run(
                [
                    "python3",
                    str(self.verify_mo2_zip_path),
                    str(zip_path),
                    "--expected-dll",
                    str(expected_dll),
                    "--expected-version",
                    "9.8.7",
                ],
                text=True,
                capture_output=True,
                check=False,
            )

        self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("exact PEX set", result.stdout)
        self.assertIn("DLL SHA256 match", result.stdout)
        self.assertIn("Package version: v9.8.7", result.stdout)

    def test_verify_mo2_zip_rejects_missing_contract_and_extra_pex(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-verify-zip-content-") as temp_dir:
            temp_root = Path(temp_dir)
            zip_path = temp_root / "CalamityAffixes_MO2_v1.2.3_2026-07-14.zip"
            self._write_minimal_mo2_zip(
                zip_path,
                omit={"CalamityAffixes/SKSE/Plugins/CalamityAffixes/runtime_contract.json"},
                extra={
                    "CalamityAffixes/Scripts/CalamityAffixes_AffixManager.pex": b"legacy-pex"
                },
            )

            result = subprocess.run(
                ["python3", str(self.verify_mo2_zip_path), str(zip_path)],
                text=True,
                capture_output=True,
                check=False,
            )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("MISSING: CalamityAffixes/SKSE/Plugins/CalamityAffixes/runtime_contract.json", result.stdout)
        self.assertIn("UNEXPECTED PEX: CalamityAffixes/Scripts/CalamityAffixes_AffixManager.pex", result.stdout)

    def test_verify_mo2_zip_rejects_dll_hash_and_package_version_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-verify-zip-identity-") as temp_dir:
            temp_root = Path(temp_dir)
            expected_dll = temp_root / "CalamityAffixes.dll"
            expected_dll.write_bytes(b"fresh-build-dll")
            zip_path = temp_root / "CalamityAffixes_MO2_v9.8.7_2026-07-14.zip"
            self._write_minimal_mo2_zip(zip_path, dll_bytes=b"different-dll")

            result = subprocess.run(
                [
                    "python3",
                    str(self.verify_mo2_zip_path),
                    str(zip_path),
                    "--expected-dll",
                    str(expected_dll),
                    "--expected-version",
                    "1.2.3",
                ],
                text=True,
                capture_output=True,
                check=False,
            )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("VERSION MISMATCH", result.stdout)
        self.assertIn("DLL HASH MISMATCH", result.stdout)

    def _write_minimal_mo2_zip(
        self,
        zip_path: Path,
        *,
        dll_bytes: bytes = b"fresh-build-dll",
        omit: set[str] | None = None,
        extra: dict[str, bytes] | None = None,
    ) -> None:
        entries: dict[str, bytes] = {
            "CalamityAffixes/CalamityAffixes.esp": b"esp",
            "CalamityAffixes/CalamityAffixes_KID.ini": b"kid",
            "CalamityAffixes/CalamityAffixes_DISTR.ini": b"spid",
            "CalamityAffixes/SKSE/Plugins/CalamityAffixes.dll": dll_bytes,
            "CalamityAffixes/SKSE/Plugins/CalamityAffixes/affixes.json": b"{}",
            "CalamityAffixes/SKSE/Plugins/CalamityAffixes/runtime_contract.json": b"{}",
            "CalamityAffixes/SKSE/Plugins/InventoryInjector/CalamityAffixes.json": b"{}",
            "CalamityAffixes/MCM/Config/CalamityAffixes/settings.ini": b"[Settings]",
            "CalamityAffixes/MCM/Config/CalamityAffixes/config.json": b"{}",
            "CalamityAffixes/MCM/Config/CalamityAffixes/keybinds.json": b"{}",
            "CalamityAffixes/PrismaUI/views/CalamityAffixes/index.html": b"<html></html>",
            "CalamityAffixes/Scripts/CalamityAffixes_ModeControl.pex": b"pex-mode",
            "CalamityAffixes/Scripts/CalamityAffixes_ModEventEmitter.pex": b"pex-events",
            "CalamityAffixes/Scripts/CalamityAffixes_MCMConfig.pex": b"pex-mcm",
        }
        for omitted in omit or set():
            entries.pop(omitted, None)
        entries.update(extra or {})
        with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for name, payload in entries.items():
                archive.writestr(name, payload)

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
            "    if [[ \"$raw_out_dir\" == /* ]]; then\n"
            "      out_dir=\"$raw_out_dir\"\n"
            "    elif command -v wslpath >/dev/null 2>&1; then\n"
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
