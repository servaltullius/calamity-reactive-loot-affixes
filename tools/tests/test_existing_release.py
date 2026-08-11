#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path


class ExistingReleaseVerifierTests(unittest.TestCase):
    TAG = "v9.8.7-rc1"
    TAG_OBJECT_SHA = "a" * 40
    COMMIT_SHA = "b" * 40
    REPO = "example/calamity"
    ASSETS = {
        "CalamityAffixes.dll": b"dll-bytes",
        "CalamityAffixes.esp": b"esp-bytes",
        "CalamityAffixes_MO2_v9.8.7-rc1_2026-07-27.zip": b"zip-bytes",
        "CalamityAffixes_MO2_v9.8.7-rc1_2026-07-27.zip.sha256": b"zip-hash\n",
    }

    @classmethod
    def setUpClass(cls) -> None:
        cls.repo_root = Path(__file__).resolve().parents[2]
        cls.verifier = cls.repo_root / "tools" / "verify_existing_release.py"
        cls.workflow = cls.repo_root / ".github" / "workflows" / "release.yml"

    def test_accepts_exact_public_release_and_annotated_tag(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-existing-release-ok-") as temp_dir:
            fixture = self._fixture(Path(temp_dir))
            result = self._run(fixture)

        self.assertEqual(result.returncode, 0, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("Existing release v9.8.7-rc1 exactly matches this build", result.stdout)
        self.assertEqual(4, result.stdout.count("SHA256 match:"))

    def test_rejects_draft_prerelease_and_release_tag_mismatches(self) -> None:
        cases = (
            ("draft", {"isDraft": True}, "is a draft"),
            ("prerelease", {"isPrerelease": False}, "prerelease=False"),
            ("release-tag", {"tagName": "v9.8.7-rc1-wrong"}, "release tag is"),
        )
        for label, changes, expected_error in cases:
            with self.subTest(label=label), tempfile.TemporaryDirectory(
                prefix=f"caff-existing-release-{label}-"
            ) as temp_dir:
                fixture = self._fixture(Path(temp_dir))
                fixture["state"]["release"].update(changes)
                self._write_state(fixture)
                result = self._run(fixture)

            self.assertEqual(result.returncode, 1, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
            self.assertIn(expected_error, result.stderr)

    def test_rejects_missing_asset_and_sha256_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-existing-release-missing-") as temp_dir:
            fixture = self._fixture(Path(temp_dir))
            fixture["state"]["release"]["assets"].pop()
            self._write_state(fixture)
            missing = self._run(fixture)

        self.assertEqual(missing.returncode, 1)
        self.assertIn("release asset names differ", missing.stderr)

        with tempfile.TemporaryDirectory(prefix="caff-existing-release-hash-") as temp_dir:
            fixture = self._fixture(Path(temp_dir))
            (fixture["remote_dir"] / "CalamityAffixes.dll").write_bytes(b"different-dll")
            mismatch = self._run(fixture)

        self.assertEqual(mismatch.returncode, 1)
        self.assertIn("SHA256 mismatch for CalamityAffixes.dll", mismatch.stderr)

    def test_rejects_remote_tag_that_no_longer_points_to_built_commit(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-existing-release-tag-sha-") as temp_dir:
            fixture = self._fixture(Path(temp_dir))
            fixture["state"]["tagObjects"][self.TAG_OBJECT_SHA]["object"]["sha"] = "c" * 40
            self._write_state(fixture)
            result = self._run(fixture)

        self.assertEqual(result.returncode, 1)
        self.assertIn("but this workflow built", result.stderr)

    def test_no_release_has_distinct_exit_code_after_tag_verification(self) -> None:
        with tempfile.TemporaryDirectory(prefix="caff-existing-release-none-") as temp_dir:
            fixture = self._fixture(Path(temp_dir))
            fixture["state"]["releaseList"] = []
            self._write_state(fixture)
            result = self._run(fixture)

        self.assertEqual(result.returncode, 3, msg=f"stdout={result.stdout}\nstderr={result.stderr}")
        self.assertIn("verified remote tag", result.stdout)

    def test_workflow_creates_only_after_not_found_and_verifies_remote_tag(self) -> None:
        source = self.workflow.read_text(encoding="utf-8")
        self.assertIn("python3 tools/verify_existing_release.py", source)
        self.assertIn('if [[ "${verify_status}" -ne 3 ]]', source)
        self.assertIn("--verify-tag", source)
        self.assertNotIn('if gh release view "${GITHUB_REF_NAME}" >/dev/null', source)

    def test_workflow_archives_matching_pdb_outside_public_release_assets(self) -> None:
        source = self.workflow.read_text(encoding="utf-8")

        self.assertIn('pdb_path="${zip_path%.zip}.pdb"', source)
        self.assertIn('cp "${pdb_path}" symbol-artifacts/', source)
        self.assertIn("path: symbol-artifacts/", source)
        self.assertIn(
            "name: calamity-affixes-symbols-${{ steps.version.outputs.version }}-${{ github.sha }}",
            source,
        )
        self.assertIn("symbol-artifacts/CalamityAffixes.dll.sha256", source)
        self.assertIn("symbol-artifacts/build-identity.txt", source)
        self.assertIn("release-artifacts/*", source)
        self.assertNotIn('cp "${pdb_path}" release-artifacts/', source)

    def test_release_workflow_rejects_stale_generated_public_docs(self) -> None:
        source = self.workflow.read_text(encoding="utf-8")

        self.assertIn("python3 tools/update_docs.py --check", source)

    def _fixture(self, temp_root: Path) -> dict[str, object]:
        local_dir = temp_root / "local"
        remote_dir = temp_root / "remote"
        local_dir.mkdir()
        remote_dir.mkdir()
        for name, payload in self.ASSETS.items():
            (local_dir / name).write_bytes(payload)
            (remote_dir / name).write_bytes(payload)

        release = {
            "tagName": self.TAG,
            "isDraft": False,
            "isPrerelease": True,
            "assets": [{"name": name} for name in sorted(self.ASSETS)],
        }
        state: dict[str, object] = {
            "releaseList": [
                {
                    "tagName": self.TAG,
                    "isDraft": False,
                    "isPrerelease": True,
                }
            ],
            "release": release,
            "tagRef": {"object": {"type": "tag", "sha": self.TAG_OBJECT_SHA}},
            "tagObjects": {
                self.TAG_OBJECT_SHA: {"object": {"type": "commit", "sha": self.COMMIT_SHA}}
            },
        }
        state_path = temp_root / "state.json"
        fake_bin = temp_root / "bin"
        fake_bin.mkdir()
        fake_gh = fake_bin / "gh"
        fake_gh.write_text(
            "#!/usr/bin/env python3\n"
            "import json, os, pathlib, shutil, sys\n"
            "state = json.loads(pathlib.Path(os.environ['FAKE_GH_STATE']).read_text())\n"
            "args = sys.argv[1:]\n"
            "if args[:2] == ['release', 'list']:\n"
            "    print(json.dumps(state['releaseList']))\n"
            "elif args[:2] == ['release', 'view']:\n"
            "    print(json.dumps(state['release']))\n"
            "elif args[:2] == ['release', 'download']:\n"
            "    target = pathlib.Path(args[args.index('--dir') + 1])\n"
            "    target.mkdir(parents=True, exist_ok=True)\n"
            "    for source in pathlib.Path(os.environ['FAKE_GH_REMOTE']).iterdir():\n"
            "        if source.is_file():\n"
            "            shutil.copy2(source, target / source.name)\n"
            "elif args and args[0] == 'api' and '/git/ref/tags/' in args[1]:\n"
            "    print(json.dumps(state['tagRef']))\n"
            "elif args and args[0] == 'api' and '/git/tags/' in args[1]:\n"
            "    sha = args[1].rsplit('/', 1)[-1]\n"
            "    print(json.dumps(state['tagObjects'][sha]))\n"
            "else:\n"
            "    print('unsupported fake gh invocation: ' + ' '.join(args), file=sys.stderr)\n"
            "    raise SystemExit(97)\n",
            encoding="utf-8",
        )
        fake_gh.chmod(0o755)

        fixture: dict[str, object] = {
            "local_dir": local_dir,
            "remote_dir": remote_dir,
            "state": state,
            "state_path": state_path,
            "fake_bin": fake_bin,
        }
        self._write_state(fixture)
        return fixture

    def _write_state(self, fixture: dict[str, object]) -> None:
        state_path = fixture["state_path"]
        self.assertIsInstance(state_path, Path)
        state_path.write_text(json.dumps(fixture["state"]), encoding="utf-8")

    def _run(self, fixture: dict[str, object]) -> subprocess.CompletedProcess[str]:
        local_dir = fixture["local_dir"]
        remote_dir = fixture["remote_dir"]
        state_path = fixture["state_path"]
        fake_bin = fixture["fake_bin"]
        self.assertIsInstance(local_dir, Path)
        self.assertIsInstance(remote_dir, Path)
        self.assertIsInstance(state_path, Path)
        self.assertIsInstance(fake_bin, Path)
        return subprocess.run(
            [
                "python3",
                str(self.verifier),
                "--repo",
                self.REPO,
                "--tag",
                self.TAG,
                "--artifacts",
                str(local_dir),
                "--expected-commit",
                self.COMMIT_SHA,
                "--expected-prerelease",
                "true",
            ],
            cwd=self.repo_root,
            text=True,
            capture_output=True,
            check=False,
            env={
                **os.environ,
                "PATH": f"{fake_bin}:{os.environ['PATH']}",
                "FAKE_GH_STATE": str(state_path),
                "FAKE_GH_REMOTE": str(remote_dir),
            },
        )


if __name__ == "__main__":
    unittest.main()
