"""Run with: python3 -m unittest discover -s tests/scripts -v"""

import importlib.util
import json
import subprocess
import tarfile
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


SCRIPT = Path(__file__).resolve().parents[2] / "scripts/package-release.py"
SPEC = importlib.util.spec_from_file_location("package_release", SCRIPT)
release = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(release)


class PackageReleaseTests(unittest.TestCase):
    def test_dependency_filtering(self):
        listing = (
            "linux-vdso.so.1 (0x123)\n"
            "libSDL3.so.0 => /usr/lib/libSDL3.so.0 (0x123)\n"
            "libstdc++.so.6 => /usr/lib/libstdc++.so.6 (0x123)\n"
            "libc.so.6 => /usr/lib/libc.so.6 (0x123)\n"
            "libm.so.6 => /usr/lib/libm.so.6 (0x123)\n"
            "libGLX.so.0 => /usr/lib/libGLX.so.0 (0x123)\n"
            "/lib64/ld-linux-x86-64.so.2 (0x123)\n"
        )
        with patch.object(release.subprocess, "run", return_value=
                          subprocess.CompletedProcess([], 0, listing, "")):
            libraries = release.linked_libraries(Path("game"))
        self.assertEqual(set(libraries), {"libSDL3.so.0", "libstdc++.so.6"})

    def test_missing_dependency_fails(self):
        with patch.object(release.subprocess, "run", return_value=
                          subprocess.CompletedProcess([], 0, "libSDL3.so.0 => not found\n", "")):
            with self.assertRaisesRegex(RuntimeError, "Missing runtime dependency"):
                release.linked_libraries(Path("game"))

    def test_absolute_loader_path_is_not_bundled(self):
        listing = (
            "libSDL3.so.0 => /usr/lib/libSDL3.so.0 (0x123)\n"
            "/lib64/ld-linux-x86-64.so.2 => /usr/lib64/ld-linux-x86-64.so.2 (0x123)\n"
        )
        with patch.object(release.subprocess, "run", return_value=
                          subprocess.CompletedProcess([], 0, listing, "")):
            libraries = release.linked_libraries(Path("game"))
        self.assertEqual(libraries, {"libSDL3.so.0": Path("/usr/lib/libSDL3.so.0")})

    def test_asset_escape_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "repo"
            root.mkdir()
            with self.assertRaisesRegex(ValueError, "escapes"):
                release.copy_asset(root, Path(temporary) / "stage", "../outside")

    def test_missing_asset_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with self.assertRaisesRegex(FileNotFoundError, "asset is missing"):
                release.copy_asset(root, root / "stage", "assets/missing.png")

    def test_archive_and_launcher(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "repo"
            config_dir = root / "config_files"
            config_dir.mkdir(parents=True)
            (config_dir / "assets.json").write_text(json.dumps({
                "fonts": [{"path": "assets/font.ttf"}],
                "textures": {"master_path": "assets", "individual_paths": ["sprite.png"]},
                "atlas_manifests": ["config_files/atlas.json"],
                "audio": [{"path": "sound.wav"}],
                "music": [{"path": "song.ogg"}],
            }))
            (config_dir / "atlas.json").write_text(json.dumps({
                "atlas": {"path": "assets/atlas.png"},
            }))
            for relative in ("font.ttf", "sprite.png", "atlas.png",
                             "audio/sound.wav", "music/song.ogg"):
                path = root / "assets" / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"asset")
            executable = root / "game"
            executable.write_bytes(b"executable")
            library = root / "libSDL3-real.so"
            library.write_bytes(b"library")
            symlink = root / "libSDL3.so.0"
            symlink.symlink_to(library.name)
            with patch.object(release, "linked_libraries", return_value={"libSDL3.so.0": symlink}):
                archive = release.package(root, executable, root / "dist", "TestGame")
            with tarfile.open(archive) as tar:
                self.assertTrue(tar.getmember("TestGame/play.sh").mode & 0o111)
                self.assertTrue(tar.getmember("TestGame/heavenhell").mode & 0o111)
                self.assertTrue(tar.getmember("TestGame/lib/libSDL3.so.0").isfile())
                launcher = tar.extractfile("TestGame/play.sh").read().decode()
                self.assertIn('cd "$GAME_DIR"', launcher)
                self.assertIn("LD_LIBRARY_PATH", launcher)
                for relative in ("assets/font.ttf", "assets/sprite.png", "assets/atlas.png",
                                 "assets/audio/sound.wav", "assets/music/song.ogg",
                                 "config_files/assets.json", "config_files/atlas.json"):
                    self.assertTrue(tar.getmember(f"TestGame/{relative}").isfile())


if __name__ == "__main__":
    unittest.main()
