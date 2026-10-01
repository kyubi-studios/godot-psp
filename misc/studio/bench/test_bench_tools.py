#!/usr/bin/env python3
"""Unit tests for the Studio benchmark tools. Run: python3 -m unittest misc/studio/bench/test_bench_tools.py"""

import base64
import json
import os
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import make_project  # noqa: E402


class WritePngTest(unittest.TestCase):
    def test_png_signature_and_size(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "a.png")
            make_project.write_png(path, 32, 16, seed=3)
            with open(path, "rb") as f:
                data = f.read()
            self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n")
            self.assertEqual(data[12:16], b"IHDR")
            width, height = struct.unpack(">II", data[16:24])
            self.assertEqual((width, height), (32, 16))
            self.assertEqual(data[-8:-4], b"IEND")

    def test_png_is_deterministic(self):
        with tempfile.TemporaryDirectory() as tmp:
            a = os.path.join(tmp, "a.png")
            b = os.path.join(tmp, "b.png")
            make_project.write_png(a, 16, 16, seed=7)
            make_project.write_png(b, 16, 16, seed=7)
            with open(a, "rb") as fa, open(b, "rb") as fb:
                self.assertEqual(fa.read(), fb.read())


class WriteGltfTest(unittest.TestCase):
    def test_gltf_buffer_length_matches_embedded_data(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "m.gltf")
            make_project.write_gltf(path, seed=2)
            with open(path, encoding="utf-8") as f:
                gltf = json.load(f)
            uri = gltf["buffers"][0]["uri"]
            prefix = "data:application/octet-stream;base64,"
            self.assertTrue(uri.startswith(prefix))
            payload = base64.b64decode(uri[len(prefix) :])
            self.assertEqual(gltf["buffers"][0]["byteLength"], len(payload))
            self.assertEqual(len(gltf["meshes"]), 1)


class GenerateTest(unittest.TestCase):
    def test_generate_counts(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = os.path.join(tmp, "proj")
            counts = make_project.generate(out, textures=5, texture_size=8, gltf=2, scene_nodes=30, seed=1, force=False)
            self.assertEqual(counts, {"textures": 5, "gltf": 2, "scene_nodes": 30})
            self.assertTrue(os.path.isfile(os.path.join(out, "project.godot")))
            self.assertTrue(os.path.isfile(os.path.join(out, "bench_load.gd")))
            self.assertEqual(len(os.listdir(os.path.join(out, "textures"))), 5)
            self.assertEqual(len(os.listdir(os.path.join(out, "models"))), 2)
            with open(os.path.join(out, "scenes", "big.tscn"), encoding="utf-8") as f:
                scene = f.read()
            self.assertEqual(scene.count("[node "), 30)

    def test_refuses_non_empty_dir_without_force(self):
        with tempfile.TemporaryDirectory() as tmp:
            keep = os.path.join(tmp, "important.txt")
            with open(keep, "w") as f:
                f.write("keep me")
            with self.assertRaises(FileExistsError):
                make_project.generate(tmp, textures=1, texture_size=8, gltf=0, scene_nodes=1, seed=1, force=False)
            with open(keep) as f:
                self.assertEqual(f.read(), "keep me")
            self.assertFalse(os.path.exists(os.path.join(tmp, "project.godot")))


if __name__ == "__main__":
    unittest.main()
