"""Tests for tools\\cmm\\gltf2cmm.py: writing a model.cmm's geometry back, everything else kept byte for byte.

run: python -m unittest discover -s tools\\tests   (test.bat runs it through CTest)
"""
import os
import struct
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(TOOLS / 'cmm'))
import cmm2gltf
import gltf2cmm
from cmm import CMM, MF_STRIP, MF_WEIGHTED

GAME_MODELS = TOOLS.parent / 'game_data' / 'data'
TAIL = b'EFFECTS, PROGRAMS AND ANIMATION KEYS'
WHITE = 0xffffffff


def mesh_record(vertex_count, face_count, flags, vtype):
    record = bytearray(gltf2cmm.MESH_RECORD_SIZE)
    struct.pack_into('<i', record, 0x00, -1)                   # no bone: world geometry
    struct.pack_into('<I', record, 0x04, (vtype & 3) + 1)
    struct.pack_into('<I', record, 0x08, vertex_count)
    struct.pack_into('<I', record, 0x0c, flags)
    struct.pack_into('<I', record, 0x28, face_count)
    struct.pack_into('<h', record, 0x48, -1)
    struct.pack_into('<h', record, 0xc2, vtype)
    return bytes(record)


def full_vertex(x, y, colour=WHITE):
    return struct.pack('<3f3fI2f', x, y, 0.0, 0.0, 0.0, -1.0, colour, x, y)


def packed_vertex(x, y, colour=WHITE):
    return struct.pack('<3fI2f', x, y, 0.0, colour, x, y)


def two_mesh_model():
    """Mesh 0: vertex type 0, one listed triangle. Mesh 1: vertex type 4, a strip of two triangles."""
    header = bytearray(0x48)
    struct.pack_into('<II', header, 0, 300, 2)
    meshes = mesh_record(3, 1, 0, 0) + mesh_record(4, 2, MF_STRIP, 4)
    vertices = (full_vertex(0, 0) + full_vertex(1, 0) + full_vertex(0, 1)
                + packed_vertex(0, 0, 0x80ff0000) + packed_vertex(1, 0) + packed_vertex(0, 1) + packed_vertex(1, 1))
    indices = struct.pack('<3H', 0, 1, 2) + struct.pack('<4H', 0, 1, 2, 3)
    return bytes(header) + meshes + vertices + indices + TAIL


class ModelFilesTest(unittest.TestCase):
    """Each test's model files in a folder of its own, removed after it."""

    def setUp(self):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.folder, self.files = folder.name, 0

    def written_model(self, data):
        self.files += 1
        path = os.path.join(self.folder, 'model%d' % self.files, 'model.cmm')
        os.makedirs(os.path.dirname(path))
        with open(path, 'wb') as f:
            f.write(data)
        return path

    def model(self, data):
        return CMM(self.written_model(data))


def square():
    """Two triangles over four vertices, in the game's clockwise winding."""
    vertices = [((0.0, 0.0, 0.0), (0.0, 0.0, -1.0), [(0.0, 0.0)], 0xff102030),
                ((2.0, 0.0, 0.0), (0.0, 0.0, -1.0), [(1.0, 0.0)], 0xff102030),
                ((0.0, 2.0, 0.0), (0.0, 0.0, -1.0), [(0.0, 1.0)], 0xff102030),
                ((2.0, 2.0, 0.0), (0.0, 0.0, -1.0), [(1.0, 1.0)], 0xff102030)]
    return gltf2cmm.Geometry(vertices, [(0, 1, 2), (2, 1, 3)])


class RebuildTest(ModelFilesTest):
    def test_nothing_replaced_writes_the_same_bytes(self):
        model = self.model(two_mesh_model())
        self.assertEqual(gltf2cmm.rebuild(model, {}), model.data)

    def test_a_replaced_mesh_takes_the_new_vertex_count(self):
        model = self.model(two_mesh_model())
        rebuilt = self.model(gltf2cmm.rebuild(model, {0: square()}))
        self.assertEqual(rebuilt.meshes[0].num_verts, 4)

    def test_a_replaced_mesh_keeps_the_games_winding(self):
        model = self.model(two_mesh_model())
        rebuilt = self.model(gltf2cmm.rebuild(model, {0: square()}))
        self.assertEqual(rebuilt.triangles(rebuilt.meshes[0]), [(0, 1, 2), (2, 1, 3)])

    def test_a_replaced_strip_is_written_as_a_list(self):
        model = self.model(two_mesh_model())
        rebuilt = self.model(gltf2cmm.rebuild(model, {1: square()}))
        self.assertFalse(rebuilt.meshes[1].flags & MF_STRIP)

    def test_the_mesh_after_a_resized_one_is_kept(self):
        model = self.model(two_mesh_model())
        rebuilt = self.model(gltf2cmm.rebuild(model, {0: square()}))
        self.assertEqual(rebuilt.colours(rebuilt.meshes[1])[0], (0xff, 0, 0, 0x80))

    def test_the_records_after_the_index_data_are_kept(self):
        model = self.model(two_mesh_model())
        self.assertTrue(gltf2cmm.rebuild(model, {0: square()}).endswith(TAIL))

    def test_a_skinned_mesh_is_refused(self):
        model = self.model(two_mesh_model())
        model.meshes[0].flags |= MF_WEIGHTED
        with self.assertRaises(ValueError):
            gltf2cmm.rebuild(model, {0: square()})

    def test_more_vertices_than_u16_indices_reach_are_refused(self):
        model = self.model(two_mesh_model())
        too_many = gltf2cmm.Geometry(square().vertices * 20000, [(0, 1, 2)])
        with self.assertRaises(ValueError):
            gltf2cmm.rebuild(model, {0: too_many})


class GlbRoundTripTest(ModelFilesTest):
    def round_trip(self):
        original = self.written_model(two_mesh_model())
        glb = os.path.join(os.path.dirname(original), 'model.glb')
        cmm2gltf.convert(original, glb)
        return CMM(original), self.model(gltf2cmm.rebuild(CMM(original), gltf2cmm.Glb(glb).mesh_geometries()))

    def test_the_exported_triangles_come_back(self):
        before, after = self.round_trip()
        self.assertEqual([after.triangles(m) for m in after.meshes], [before.triangles(m) for m in before.meshes])

    def test_the_exported_vertices_come_back(self):
        before, after = self.round_trip()
        self.assertEqual(after.vertices(after.meshes[0]), before.vertices(before.meshes[0]))

    def test_the_exported_colours_come_back(self):
        before, after = self.round_trip()
        self.assertEqual(after.colours(after.meshes[1]), before.colours(before.meshes[1]))


@unittest.skipUnless(GAME_MODELS.is_dir(), 'no game_data\\data')
class GameModelsTest(unittest.TestCase):
    def test_every_model_on_the_disc_rebuilds_byte_for_byte(self):
        differing = []
        for root, _, files in os.walk(GAME_MODELS):
            if 'model.cmm' not in (f.lower() for f in files):
                continue
            path = os.path.join(root, 'model.cmm')
            if struct.unpack_from('<I', open(path, 'rb').read(4))[0] != 300:
                continue                                       # the two version-200 HUD models
            model = CMM(path)
            if gltf2cmm.rebuild(model, {}) != model.data:
                differing.append(path)
        self.assertEqual(differing, [])


if __name__ == '__main__':
    unittest.main()
