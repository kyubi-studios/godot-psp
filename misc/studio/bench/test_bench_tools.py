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
import run_bench  # noqa: E402


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


class RunBenchHelpersTest(unittest.TestCase):
    def test_median_odd_and_even(self):
        self.assertEqual(run_bench.median([3.0, 1.0, 2.0]), 2.0)
        self.assertEqual(run_bench.median([4.0, 1.0, 2.0, 3.0]), 2.5)

    def test_parse_load_ms(self):
        self.assertEqual(run_bench.parse_load_ms("noise\nBENCH_LOAD_MS=12.500\nmore"), 12.5)
        self.assertIsNone(run_bench.parse_load_ms("BENCH_LOAD_FAILED"))

    def test_summarize_excludes_failed_runs(self):
        summary = run_bench.summarize([10.0, None, 30.0, 20.0])
        self.assertEqual(summary["median"], 20.0)
        self.assertEqual(summary["min"], 10.0)
        self.assertEqual(summary["max"], 30.0)
        self.assertEqual(summary["runs"], 3)
        self.assertEqual(summary["failed"], 1)

    def test_summarize_all_failed(self):
        summary = run_bench.summarize([None, None])
        self.assertIsNone(summary["median"])
        self.assertEqual(summary["failed"], 2)
        self.assertEqual(summary["runs"], 0)

    def test_failed_process_is_not_timed(self):
        # A crashing command must produce None, never a duration.
        self.assertIsNone(run_bench.time_command([sys.executable, "-c", "import sys; sys.exit(3)"], timeout=30))
        self.assertIsNotNone(run_bench.time_command([sys.executable, "-c", "pass"], timeout=30))


    def test_import_errors_count_as_failure(self):
        self.assertTrue(run_bench.import_failed("x\nERROR: Error importing 'res://a.png'.\n"))
        self.assertFalse(run_bench.import_failed("all good\nWARNING: something\n"))

    def test_cpu_model(self):
        cpuinfo = "processor\t: 0\nmodel name\t: Fancy CPU 9000\nflags\t: x\n"
        self.assertEqual(run_bench.cpu_model(cpuinfo), "Fancy CPU 9000")
        self.assertEqual(run_bench.cpu_model("nothing"), "unknown")


class RunBenchSafetyTest(unittest.TestCase):
    def test_refuses_non_bench_project(self):
        with tempfile.TemporaryDirectory() as tmp:
            os.makedirs(os.path.join(tmp, ".godot"))
            self.assertFalse(run_bench.is_bench_project(tmp))
            with open(os.path.join(tmp, "bench_load.gd"), "w") as f:
                f.write("extends SceneTree\n")
            self.assertTrue(run_bench.is_bench_project(tmp))

    def test_cold_run_fails_when_godot_dir_cannot_be_removed(self):
        with tempfile.TemporaryDirectory() as tmp:
            target = os.path.join(tmp, "elsewhere")
            os.makedirs(target)
            project = os.path.join(tmp, "project")
            os.makedirs(project)
            # rmtree refuses symlinks, so .godot survives the "cold" reset.
            os.symlink(target, os.path.join(project, ".godot"))
            self.assertFalse(run_bench.reset_import_cache(project))
            self.assertTrue(os.path.isdir(target))

    def test_cold_reset_removes_godot_dir(self):
        with tempfile.TemporaryDirectory() as tmp:
            os.makedirs(os.path.join(tmp, ".godot", "imported"))
            self.assertTrue(run_bench.reset_import_cache(tmp))
            self.assertFalse(os.path.exists(os.path.join(tmp, ".godot")))


if __name__ == "__main__":
    unittest.main()
