#!/usr/bin/env python3
"""
Build a texture atlas from assets/images/dual_sheets/*.png.

The output JSON is an asset-manifest fragment: it names the generated atlas
texture and lists each original dual-sheet sprite with the atlas region that
replaces its old standalone texture.

Requires Pillow:
    python -m pip install pillow
"""

from __future__ import annotations

import argparse
import json
import math
from dataclasses import dataclass
from pathlib import Path

from PIL import Image


@dataclass(frozen=True)
class SourceImage:
    name: str
    path: Path
    width: int
    height: int


@dataclass(frozen=True)
class PackedImage:
    source: SourceImage
    x: int
    y: int


def asset_name_from_path(path: Path) -> str:
    return path.stem


def next_power_of_two(value: int) -> int:
    if value <= 1:
        return 1
    return 1 << (value - 1).bit_length()


def load_existing_animation_metadata(assets_json: Path) -> dict[str, dict]:
    if not assets_json.exists():
        return {}

    with assets_json.open("r", encoding="utf-8") as file:
        data = json.load(file)

    return {
        animation["name"]: animation
        for animation in data.get("animations", [])
        if "name" in animation
    }


def load_sources(source_dir: Path) -> list[SourceImage]:
    sources: list[SourceImage] = []
    for path in sorted(source_dir.glob("*.png")):
        with Image.open(path) as image:
            sources.append(SourceImage(
                name=asset_name_from_path(path),
                path=path,
                width=image.width,
                height=image.height,
            ))
    return sources


def choose_atlas_width(sources: list[SourceImage], padding: int, max_width: int | None) -> int:
    widest = max(source.width for source in sources) + padding * 2
    total_area = sum((source.width + padding * 2) * (source.height + padding * 2) for source in sources)
    width = max(widest, next_power_of_two(math.ceil(math.sqrt(total_area))))
    if max_width is not None:
        width = min(max_width, width)
        if width < widest:
            raise ValueError(f"max atlas width {max_width} is smaller than widest source {widest}")
    return width


def pack_images(sources: list[SourceImage], padding: int, atlas_width: int) -> tuple[list[PackedImage], int]:
    packed: list[PackedImage] = []
    cursor_x = 0
    cursor_y = 0
    row_height = 0

    for source in sources:
        outer_width = source.width + padding * 2
        outer_height = source.height + padding * 2

        if cursor_x > 0 and cursor_x + outer_width > atlas_width:
            cursor_x = 0
            cursor_y += row_height
            row_height = 0

        packed.append(PackedImage(
            source=source,
            x=cursor_x + padding,
            y=cursor_y + padding,
        ))
        cursor_x += outer_width
        row_height = max(row_height, outer_height)

    return packed, cursor_y + row_height


def paste_with_extruded_padding(atlas: Image.Image, image: Image.Image, x: int, y: int, padding: int) -> None:
    atlas.paste(image, (x, y), image)
    if padding <= 0:
        return

    width, height = image.size

    top_row = image.crop((0, 0, width, 1))
    bottom_row = image.crop((0, height - 1, width, height))
    left_col = image.crop((0, 0, 1, height))
    right_col = image.crop((width - 1, 0, width, height))

    for offset in range(1, padding + 1):
        atlas.paste(top_row, (x, y - offset))
        atlas.paste(bottom_row, (x, y + height - 1 + offset))
        atlas.paste(left_col, (x - offset, y))
        atlas.paste(right_col, (x + width - 1 + offset, y))

    top_left = image.crop((0, 0, 1, 1))
    top_right = image.crop((width - 1, 0, width, 1))
    bottom_left = image.crop((0, height - 1, 1, height))
    bottom_right = image.crop((width - 1, height - 1, width, height))

    for dx in range(1, padding + 1):
        for dy in range(1, padding + 1):
            atlas.paste(top_left, (x - dx, y - dy))
            atlas.paste(top_right, (x + width - 1 + dx, y - dy))
            atlas.paste(bottom_left, (x - dx, y + height - 1 + dy))
            atlas.paste(bottom_right, (x + width - 1 + dx, y + height - 1 + dy))


def build_atlas(packed: list[PackedImage], atlas_width: int, atlas_height: int, padding: int) -> Image.Image:
    atlas = Image.new("RGBA", (atlas_width, atlas_height), (0, 0, 0, 0))
    for packed_image in packed:
        with Image.open(packed_image.source.path) as source_image:
            image = source_image.convert("RGBA")
            paste_with_extruded_padding(atlas, image, packed_image.x, packed_image.y, padding)
    return atlas


def infer_animation_metadata(source: SourceImage, tile_size: int) -> dict:
    return {
        "name": source.name,
        "frames": 1,
        "frametime": 100,
        "cols": max(1, source.width // tile_size),
        "rows": max(1, source.height // tile_size),
        "width": tile_size,
        "height": tile_size,
    }


def make_manifest(
    packed: list[PackedImage],
    atlas_image: Path,
    atlas_width: int,
    atlas_height: int,
    padding: int,
    existing_metadata: dict[str, dict],
    tile_size: int,
    project_root: Path,
) -> dict:
    atlas_name = atlas_image.stem
    project_root = project_root.resolve()
    atlas_image = atlas_image.resolve()
    atlas_path = atlas_image.relative_to(project_root).as_posix()
    image_path_for_assets_json = atlas_image.relative_to((project_root / "assets/images").resolve()).as_posix()

    sprites = []
    for packed_image in packed:
        source = packed_image.source
        metadata = dict(existing_metadata.get(source.name, infer_animation_metadata(source, tile_size)))
        metadata["name"] = source.name
        metadata["texture"] = atlas_name
        metadata["atlas"] = {
            "x": packed_image.x,
            "y": packed_image.y,
            "w": source.width,
            "h": source.height,
        }
        sprites.append(metadata)

    return {
        "atlas": {
            "name": atlas_name,
            "path": atlas_path,
            "width": atlas_width,
            "height": atlas_height,
            "padding": padding,
        },
        "textures": {
            "master_path": "assets/images/",
            "individual_paths": [
                image_path_for_assets_json,
            ],
        },
        "animations": sprites,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build the dual-sheets texture atlas.")
    parser.add_argument("--source-dir", default="assets/images/dual_sheets", type=Path)
    parser.add_argument("--assets-json", default="config_files/assets.json", type=Path)
    parser.add_argument("--atlas-image", default="assets/images/dual_sheets_atlas.png", type=Path)
    parser.add_argument("--metadata-json", default="config_files/dual_sheets_atlas.json", type=Path)
    parser.add_argument("--tile-size", default=16, type=int)
    parser.add_argument("--padding", default=1, type=int)
    parser.add_argument("--max-width", default=None, type=int)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    project_root = Path.cwd()
    source_dir = args.source_dir
    assets_json = args.assets_json
    atlas_image = args.atlas_image
    metadata_json = args.metadata_json

    sources = load_sources(source_dir)
    if not sources:
        raise RuntimeError(f"No PNG files found in {source_dir}")

    atlas_width = choose_atlas_width(sources, args.padding, args.max_width)
    packed, atlas_height = pack_images(sources, args.padding, atlas_width)
    atlas = build_atlas(packed, atlas_width, atlas_height, args.padding)

    atlas_image.parent.mkdir(parents=True, exist_ok=True)
    metadata_json.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(atlas_image)

    manifest = make_manifest(
        packed=packed,
        atlas_image=atlas_image,
        atlas_width=atlas_width,
        atlas_height=atlas_height,
        padding=args.padding,
        existing_metadata=load_existing_animation_metadata(assets_json),
        tile_size=args.tile_size,
        project_root=project_root,
    )
    with metadata_json.open("w", encoding="utf-8") as file:
        json.dump(manifest, file, indent=4)
        file.write("\n")

    print(f"Wrote {atlas_image} ({atlas_width}x{atlas_height})")
    print(f"Wrote {metadata_json}")
    print(f"Packed {len(packed)} dual sheets")


if __name__ == "__main__":
    main()
