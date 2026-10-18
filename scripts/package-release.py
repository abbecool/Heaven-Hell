#!/usr/bin/env python3
"""Build and package a Linux release using only Python's standard library."""

from __future__ import annotations

import argparse
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
# These belong to the host's libc/graphics stack, not the application bundle.
HOST_LIBRARY = re.compile(
    r"^(?:ld-linux.*|ld-musl.*|lib(?:c|m|pthread|dl|rt|resolv|util|anl)\.so(?:\..*)?"
    r"|lib(?:GL|GLX|GLdispatch|OpenGL|EGL|GLESv1_CM|GLESv2|drm|gbm)\.so(?:\..*)?)$"
)


def run(command: list[str], cwd: Path = REPO_ROOT) -> None:
    subprocess.run(command, cwd=cwd, check=True)


def copy_asset(root: Path, destination: Path, relative: str) -> None:
    path = Path(relative)
    source = (root / path).resolve()
    target = (destination / path).resolve()
    if not source.is_relative_to(root.resolve()) or not target.is_relative_to(destination.resolve()):
        raise ValueError(f"Asset path escapes the package or repository: {relative}")
    if not source.is_file():
        raise FileNotFoundError(f"Configured runtime asset is missing: {relative}")
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)


def copy_configured_assets(root: Path, destination: Path) -> None:
    config = json.loads((root / "config_files/assets.json").read_text(encoding="utf-8"))
    paths = [font["path"] for font in config.get("fonts", [])]
    textures = config["textures"]
    paths.extend(str(Path(textures["master_path"]) / name)
                 for name in textures["individual_paths"])
    for manifest_path in config.get("atlas_manifests", []):
        manifest_file = (root / manifest_path).resolve()
        if not manifest_file.is_relative_to(root.resolve()):
            raise ValueError(f"Atlas manifest escapes the repository: {manifest_path}")
        manifest = json.loads(manifest_file.read_text(encoding="utf-8"))
        paths.append(manifest["atlas"]["path"])
    for category in ("audio", "music"):
        paths.extend(str(Path("assets") / category / entry["path"])
                     for entry in config.get(category, []))
    for path in sorted(set(paths)):
        copy_asset(root, destination, path)


def linked_libraries(binary: Path) -> dict[str, Path]:
    # Only inspect the executable we just built (or a trusted --skip-build input).
    result = subprocess.run(["ldd", str(binary)], check=True, text=True,
                            capture_output=True, env={**os.environ, "LC_ALL": "C"})
    libraries: dict[str, Path] = {}
    for line in result.stdout.splitlines():
        if "=> not found" in line:
            raise RuntimeError(f"Missing runtime dependency: {line.strip()}")
        match = re.match(r"\s*(\S+)\s+=>\s+(/.*?)\s+\(0x[0-9a-fA-F]+\)", line)
        if match:
            name, path = match.groups()
            # Arch can report an absolute loader path on the left of "=>".
            # Always use a basename for filtering and package destinations.
            name = Path(name).name
            if not HOST_LIBRARY.fullmatch(name):
                libraries[name] = Path(path)
    if not libraries:
        raise RuntimeError("ldd found no bundleable dependencies; expected a dynamically linked SDL game")
    return libraries


def package(root: Path, executable: Path, output: Path, name: str) -> Path:
    libraries = linked_libraries(executable)
    output.mkdir(parents=True, exist_ok=True)
    archive = output / f"{name}.tar.gz"
    # Stage privately and replace only the completed archive, not existing folders.
    with tempfile.TemporaryDirectory(prefix=".heavenhell-package-", dir=output) as temporary:
        temporary_root = Path(temporary)
        stage = temporary_root / name
        stage.mkdir()
        shutil.copy2(executable, stage / "heavenhell")
        (stage / "heavenhell").chmod(0o755)
        copy_configured_assets(root, stage)
        shutil.copytree(root / "config_files", stage / "config_files")
        library_dir = stage / "lib"
        library_dir.mkdir()
        for soname, source in sorted(libraries.items()):
            # Dereference symlinks, but preserve the name requested by the loader.
            shutil.copy2(source, library_dir / soname)
        launcher = stage / "play.sh"
        launcher.write_text(
            '#!/bin/sh\nset -eu\n'
            'GAME_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\n'
            'cd "$GAME_DIR"\n'
            'export LD_LIBRARY_PATH="$GAME_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n'
            'exec "$GAME_DIR/heavenhell" "$@"\n', encoding="utf-8")
        launcher.chmod(0o755)
        (stage / "README.txt").write_text(
            f"{name}\n\nRun ./play.sh to start the game.\n\n"
            "This package targets the build machine's CPU architecture.\n"
            "Use the same Linux distribution/version or a compatible newer system.\n"
            "The host must provide glibc, a graphical session, and OpenGL drivers.\n"
            "Linked dependencies are bundled; dynamically loaded plugins may still\n"
            "require host packages. This is not a universal Linux/AppImage build.\n",
            encoding="utf-8")
        staged_archive = temporary_root / archive.name
        with tarfile.open(staged_archive, "w:gz") as tar:
            tar.add(stage, arcname=name)
        staged_archive.replace(archive)
    return archive


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-build", action="store_true",
                        help="Package the trusted existing run/Release/heavenhell executable")
    parser.add_argument("--output-dir", type=Path, default=REPO_ROOT / "dist",
                        help="Archive destination (default: repository dist directory)")
    args = parser.parse_args()
    try:
        if sys.platform != "linux":
            raise RuntimeError("Run this script on Linux; it does not cross-compile")
        for tool in (["ldd"] if args.skip_build else ["cmake", "ldd"]):
            if not shutil.which(tool):
                raise RuntimeError(f"Required tool not found on PATH: {tool}")
        version = (REPO_ROOT / "version.txt").read_text(encoding="utf-8").strip()
        if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", version):
            raise ValueError("version.txt must contain a safe, nonempty package version")
        if not args.skip_build:
            run(["cmake", "--preset", "linux-release"])
            run(["cmake", "--build", "--preset", "linux-release", "--parallel"])
        executable = REPO_ROOT / "run/Release/heavenhell"
        if not executable.is_file():
            raise FileNotFoundError(f"Game executable not found: {executable}")
        name = f"HeavenHell-{version}-linux-{platform.machine()}"
        archive = package(REPO_ROOT, executable, args.output_dir.resolve(), name)
        print(f"Created release package: {archive}")
        return 0
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Packaging failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
