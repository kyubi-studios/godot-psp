#!/usr/bin/env python3
"""Generate a deterministic Godot project for editor benchmarks (stdlib only).

The project contains many PNG textures (texture importer, threaded), glTF models
(scene importer) and one large scene referencing the textures, plus a script that
measures how long the scene takes to load.

Usage: make_project.py <dir> [--textures 400] [--texture-size 256] [--gltf 20]
                             [--scene-nodes 3000] [--seed 1] [--force]
"""

import argparse
import base64
import json
import os
import random
import shutil
import struct
import sys
import zlib

PROJECT_GODOT = """config_version=5

[application]

config/name="Studio Benchmark"
"""

BENCH_LOAD_GD = """extends SceneTree

# Prints how long the big scene takes to load (bypassing the resource cache) and instantiate.

func _init() -> void:
	var start := Time.get_ticks_usec()
	var scene: PackedScene = ResourceLoader.load("res://scenes/big.tscn", "", ResourceLoader.CACHE_MODE_IGNORE)
	var load_ms := (Time.get_ticks_usec() - start) / 1000.0
	if scene == null:
		printerr("BENCH_LOAD_FAILED")
		quit(1)
		return
	var instance := scene.instantiate()
	var total_ms := (Time.get_ticks_usec() - start) / 1000.0
	print("BENCH_LOAD_MS=%.3f" % load_ms)
	print("BENCH_INSTANTIATE_MS=%.3f" % total_ms)
	instance.free()
	quit(0)
"""


def _png_chunk(kind, data):
    chunk = kind + data
    return struct.pack(">I", len(data)) + chunk + struct.pack(">I", zlib.crc32(chunk) & 0xFFFFFFFF)


def write_png(path, width, height, seed):
    """Write an RGBA PNG with seeded noisy gradients (compresses like real art, not like flat color)."""
    rng = random.Random(seed)
    base = [rng.randrange(256) for _ in range(4)]
    rows = bytearray()
    for y in range(height):
        rows.append(0)  # Filter type: none.
        for x in range(width):
            rows += bytes(
                (
                    (base[0] + x * 3 + rng.randrange(8)) & 0xFF,
                    (base[1] + y * 3 + rng.randrange(8)) & 0xFF,
                    (base[2] + (x ^ y) + rng.randrange(8)) & 0xFF,
                    255,
                )
            )
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    data = b"\x89PNG\r\n\x1a\n" + _png_chunk(b"IHDR", ihdr) + _png_chunk(b"IDAT", zlib.compress(bytes(rows), 6))
    data += _png_chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(data)


def write_gltf(path, seed):
    """Write a glTF 2.0 file with one box mesh and an embedded buffer."""
    rng = random.Random(seed)
    s = 0.5 + rng.random()
    corners = [(x, y, z) for x in (-s, s) for y in (-s, s) for z in (-s, s)]
    faces = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
    indices = []
    for a, b, c, d in faces:
        indices += [a, b, c, a, c, d]
    positions = b"".join(struct.pack("<fff", *p) for p in corners)
    index_data = b"".join(struct.pack("<H", i) for i in indices)
    buffer = positions + index_data
    gltf = {
        "asset": {"version": "2.0", "generator": "studio-bench"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0, "name": "Box"}],
        "meshes": [{"primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]}],
        "buffers": [
            {
                "byteLength": len(buffer),
                "uri": "data:application/octet-stream;base64," + base64.b64encode(buffer).decode("ascii"),
            }
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": len(positions), "target": 34962},
            {"buffer": 0, "byteOffset": len(positions), "byteLength": len(index_data), "target": 34963},
        ],
        "accessors": [
            {
                "bufferView": 0,
                "componentType": 5126,
                "count": len(corners),
                "type": "VEC3",
                "min": [-s, -s, -s],
                "max": [s, s, s],
            },
            {"bufferView": 1, "componentType": 5123, "count": len(indices), "type": "SCALAR"},
        ],
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(gltf, f, indent=1, sort_keys=True)


def _write_scene(path, scene_nodes, textures, seed):
    rng = random.Random(seed)
    sprite_count = (scene_nodes - 1) // 10 if textures else 0
    used_textures = min(textures, sprite_count)
    lines = ["[gd_scene format=3]", ""]
    for i in range(used_textures):
        lines.append(f'[ext_resource type="Texture2D" path="res://textures/tex_{i:04d}.png" id="{i + 1}"]')
    if used_textures:
        lines.append("")
    lines.append('[node name="Root" type="Node2D"]')
    lines.append("")
    for n in range(1, scene_nodes):
        x, y = rng.randrange(4096), rng.randrange(4096)
        if used_textures and n % 10 == 0:
            lines.append(f'[node name="Sprite{n}" type="Sprite2D" parent="."]')
            lines.append(f"position = Vector2({x}, {y})")
            lines.append(f'texture = ExtResource("{(n // 10) % used_textures + 1}")')
        else:
            lines.append(f'[node name="Node{n}" type="Node2D" parent="."]')
            lines.append(f"position = Vector2({x}, {y})")
        lines.append("")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


def generate(out_dir, textures, texture_size, gltf, scene_nodes, seed, force):
    if os.path.isdir(out_dir) and os.listdir(out_dir):
        if not force:
            raise FileExistsError(f"'{out_dir}' is not empty; pass --force to replace it")
        shutil.rmtree(out_dir)
    for sub in ("textures", "models", "scenes"):
        os.makedirs(os.path.join(out_dir, sub), exist_ok=True)

    with open(os.path.join(out_dir, "project.godot"), "w", encoding="utf-8") as f:
        f.write(PROJECT_GODOT)
    with open(os.path.join(out_dir, "bench_load.gd"), "w", encoding="utf-8") as f:
        f.write(BENCH_LOAD_GD)
    for i in range(textures):
        write_png(os.path.join(out_dir, "textures", f"tex_{i:04d}.png"), texture_size, texture_size, seed * 100003 + i)
    for i in range(gltf):
        write_gltf(os.path.join(out_dir, "models", f"model_{i:03d}.gltf"), seed * 100003 + i)
    _write_scene(os.path.join(out_dir, "scenes", "big.tscn"), max(1, scene_nodes), textures, seed)
    return {"textures": textures, "gltf": gltf, "scene_nodes": max(1, scene_nodes)}


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("dir")
    parser.add_argument("--textures", type=int, default=400)
    parser.add_argument("--texture-size", type=int, default=256)
    parser.add_argument("--gltf", type=int, default=20)
    parser.add_argument("--scene-nodes", type=int, default=3000)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--force", action="store_true", help="replace a non-empty output directory")
    args = parser.parse_args(argv)
    try:
        counts = generate(args.dir, args.textures, args.texture_size, args.gltf, args.scene_nodes, args.seed, args.force)
    except FileExistsError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 2
    print(json.dumps(counts))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
