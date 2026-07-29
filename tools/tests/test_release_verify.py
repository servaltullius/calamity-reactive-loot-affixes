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

    def test_release_workflow_runs_static_gates_before_build_and_packaging_e2e_after(self) -> None:
        source = (self.repo_root / ".github" / "workflows" / "release.yml").read_text(encoding="utf-8")
        release_build = source.index("python3 tools/release_build.py --version")
        required_pre_build_gates = (
            "uses: actions/setup-node@v4",
            "python3 tools/verify_prisma_view.py",
            "python3 tools/verify_runtime_contract_sync.py",
        )

        for gate in required_pre_build_gates:
            self.assertIn(gate, source)
            self.assertLess(source.index(gate), release_build)

        tools_suite = "python3 -m unittest discover -s tools/tests -p 'test_*.py'"
        self.assertIn(tools_suite, source)
        self.assertGreater(source.index(tools_suite), release_build)
        self.assertIn('CALAMITY_REQUIRE_NODE: "1"', source)
        self.assertIn('CALAMITY_REQUIRE_PACKAGING_E2E: "1"', source)

    def test_ci_verify_runs_required_packaging_e2e_after_plugin_build(self) -> None:
        source = self.ci_verify_path.read_text(encoding="utf-8")
        runtime_job = source.split("  runtime-gate-tests:\n", 1)[1]
        e2e_step = "- name: Run compiler-less Papyrus packaging E2E"

        self.assertIn("uses: actions/setup-dotnet@v4", runtime_job)
        self.assertIn("python3 -m pip install --upgrade pip jsonschema", runtime_job)
        self.assertIn('CALAMITY_REQUIRE_PACKAGING_E2E: "1"', runtime_job)
        self.assertIn(
            "test_build_mo2_zip_packages_verified_prebuilt_pex_without_a_compiler",
            runtime_job,
        )
        self.assertIn(
            "test_build_mo2_zip_refuses_to_package_when_the_papyrus_pin_fails",
            runtime_job,
        )
        self.assertLess(
            runtime_job.index("- name: Build SKSE plugin target"),
            runtime_job.index(e2e_step),
        )


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

    def test_release_workflow_publishes_instead_of_drafting(self) -> None:
        """Tagging is the decision to release; nothing should wait on a click.

        Every release here is a public test build, so a draft only delayed the
        download. Pinned because the regression is silent -- a release that
        quietly went back to draft would look like a successful run, and the
        first sign would be someone asking where the download went.
        """
        source = (self.repo_root / ".github" / "workflows" / "release.yml").read_text(encoding="utf-8")
        self.assertNotIn("--draft", source)
        self.assertIn("--prerelease", source)
        self.assertIn("gh release create", source)

    def test_release_workflow_still_marks_prerelease_tags(self) -> None:
        """`--prerelease` is what keeps an rc off the "Latest release" badge.

        Publishing directly makes this the only thing separating a candidate
        from a final release, so the tag -> prerelease derivation has to hold.
        """
        source = (self.repo_root / ".github" / "workflows" / "release.yml").read_text(encoding="utf-8")
        self.assertIn('base_version="${version%%[-+]*}"', source)
        self.assertIn('echo "prerelease=true" >> "$GITHUB_OUTPUT"', source)
        self.assertIn('echo "prerelease=false" >> "$GITHUB_OUTPUT"', source)

    def test_build_mo2_zip_falls_back_only_when_the_compiler_is_absent(self) -> None:
        """The fallback must key on "no compiler", not on "compile failed".

        PapyrusCompiler.exe cannot be installed on a GitHub runner, so CI has to
        package the committed .pex. But a compile that runs and fails is a very
        different signal from a compiler that was never there, and
        compile_papyrus.sh refuses to substitute a staged PEX for the former --
        a fallback keyed on failure would quietly undo that.
        """
        source = self.build_mo2_zip_path.read_text(encoding="utf-8")
        self.assertIn('if [[ -f "${papyrus_compiler}" ]]; then', source)
        self.assertIn('"${repo_root}/tools/compile_papyrus.sh" --data "${stage_data_dir}"', source)
        self.assertIn('python3 "${repo_root}/tools/verify_papyrus_pin.py"', source)

    def _require_packaging_e2e_dll(self) -> None:
        if self.repo_build_dll.is_file():
            return

        message = "packaging smoke test requires a built CalamityAffixes.dll"
        if os.environ.get("CALAMITY_REQUIRE_PACKAGING_E2E") == "1":
            self.fail(message)
        self.skipTest(message)

    def test_build_mo2_zip_packages_verified_prebuilt_pex_without_a_compiler(self) -> None:
        """End-to-end proof of the CI packaging path.

        Runs the real packaging script with the compiler pointed at a path that
        does not exist -- the exact condition on a GitHub runner -- and requires
        the .pex inside the zip to be byte-identical to the committed ones. If
        the fallback ever started shipping something else, a hash pin on the
        repository copy alone would not notice.
        """
        self._require_packaging_e2e_dll()

        package_version = "test-prebuilt-pex"
        out_zip = self._package_zip_path(package_version)
        out_zip.unlink(missing_ok=True)

        with tempfile.TemporaryDirectory(prefix="caff-papyrus-prebuilt-") as temp_dir:
            temp_root = Path(temp_dir)
            env = self._build_mo2_zip_env(
                temp_root=temp_root,
                package_version=package_version,
                # _build_mo2_zip_env installs a fake compiler; point past it so
                # the absent-compiler branch is the one under test.
                extra_env={"PAPYRUS_COMPILER_EXE": str(temp_root / "no-such-PapyrusCompiler.exe")},
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
                self.assertIn("Using verified prebuilt Papyrus scripts", result.stdout)

                with zipfile.ZipFile(out_zip) as archive:
                    shipped = {
                        Path(name).name: archive.read(name)
                        for name in archive.namelist()
                        if name.endswith(".pex")
                    }
                committed = {
                    path.name: path.read_bytes()
                    for path in (self.repo_root / "Data" / "Scripts").glob("*.pex")
                }
                self.assertEqual(committed, shipped)
                self.assertEqual(3, len(shipped))
            finally:
                out_zip.unlink(missing_ok=True)

    def test_build_mo2_zip_refuses_to_package_when_the_papyrus_pin_fails(self) -> None:
        """No compiler AND an unverifiable .pex must abort, not ship anyway.

        The pin is the only thing standing between a compiler-less runner and a
        .pex that no longer matches its .psc, so a non-zero exit from the
        verifier has to stop packaging. Forced here by shimming python3 to fail
        for that one script, which avoids tampering with the real Data/ tree.
        """
        self._require_packaging_e2e_dll()

        package_version = "test-pin-refusal"
        out_zip = self._package_zip_path(package_version)
        out_zip.unlink(missing_ok=True)

        with tempfile.TemporaryDirectory(prefix="caff-papyrus-pin-fail-") as temp_dir:
            temp_root = Path(temp_dir)
            bin_dir = temp_root / "bin"
            bin_dir.mkdir(parents=True)
            real_python = shutil.which("python3")
            self.assertIsNotNone(real_python)
            shim = bin_dir / "python3"
            shim.write_text(
                "#!/usr/bin/env bash\n"
                'if [[ "${1:-}" == */verify_papyrus_pin.py ]]; then\n'
                '  echo "simulated pin mismatch" >&2\n'
                "  exit 1\n"
                "fi\n"
                f'exec "{real_python}" "$@"\n',
                encoding="utf-8",
            )
            shim.chmod(0o755)

            env = self._build_mo2_zip_env(
                temp_root=temp_root,
                package_version=package_version,
                extra_env={
                    "PAPYRUS_COMPILER_EXE": str(temp_root / "no-such-PapyrusCompiler.exe"),
                    "PATH": f"{bin_dir}:{os.environ['PATH']}",
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
                self.assertNotEqual(result.returncode, 0, msg=f"stdout={result.stdout}")
                self.assertIn("Refusing to package", result.stderr)
                self.assertFalse(out_zip.exists(), "an unverifiable .pex must not produce a package")
            finally:
                out_zip.unlink(missing_ok=True)

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

    def test_verify_mo2_zip_rejects_a_panel_view_missing_a_file_it_loads(self) -> None:
        """A dropped stylesheet or script leaves the panel unstyled or dead in
        game, and nothing in the log says so -- packaging is the last place it
        can still be caught."""
        with tempfile.TemporaryDirectory(prefix="caff-verify-zip-view-") as temp_dir:
            zip_path = Path(temp_dir) / "CalamityAffixes_MO2_v1.2.3_2026-07-14.zip"
            self._write_minimal_mo2_zip(
                zip_path,
                omit={"CalamityAffixes/PrismaUI/views/CalamityAffixes/scripts/dom.js"},
            )

            result = subprocess.run(
                ["python3", str(self.verify_mo2_zip_path), str(zip_path)],
                text=True,
                capture_output=True,
                check=False,
            )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn(
            "MISSING VIEW ASSET: CalamityAffixes/PrismaUI/views/CalamityAffixes/scripts/dom.js",
            result.stdout,
        )

    def test_verify_mo2_zip_rejects_a_panel_view_file_nothing_loads(self) -> None:
        """The other direction: a file shipped but never referenced is either
        dead weight or the trace of a dropped <script src>."""
        with tempfile.TemporaryDirectory(prefix="caff-verify-zip-orphan-") as temp_dir:
            zip_path = Path(temp_dir) / "CalamityAffixes_MO2_v1.2.3_2026-07-14.zip"
            self._write_minimal_mo2_zip(
                zip_path,
                extra={
                    "CalamityAffixes/PrismaUI/views/CalamityAffixes/scripts/orphan.js": b"//"
                },
            )

            result = subprocess.run(
                ["python3", str(self.verify_mo2_zip_path), str(zip_path)],
                text=True,
                capture_output=True,
                check=False,
            )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn(
            "UNREFERENCED VIEW ASSET: "
            "CalamityAffixes/PrismaUI/views/CalamityAffixes/scripts/orphan.js",
            result.stdout,
        )

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
            # The panel view is index.html plus the files it loads. The fixture
            # carries a stylesheet and a script so the packaging check for those
            # references is exercised instead of passing on an empty view.
            "CalamityAffixes/PrismaUI/views/CalamityAffixes/index.html": (
                b'<html><head><link rel="stylesheet" href="styles/base.css" /></head>'
                b'<body><script src="scripts/dom.js"></script></body></html>'
            ),
            "CalamityAffixes/PrismaUI/views/CalamityAffixes/styles/base.css": b":root{}",
            "CalamityAffixes/PrismaUI/views/CalamityAffixes/scripts/dom.js": b'"use strict";',
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
