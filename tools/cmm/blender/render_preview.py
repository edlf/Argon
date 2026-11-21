# Blender script: import a .glb and render preview images (workbench, textured).
# usage: blender -b --factory-startup -P render_preview.py -- <in.glb> <out_prefix> [hide_regex]
import bpy, sys, math, re
from mathutils import Vector

argv = sys.argv[sys.argv.index('--') + 1:]
src, prefix = argv[0], argv[1]
hide = re.compile(argv[2]) if len(argv) > 2 else None

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
sc = bpy.context.scene
objs = [o for o in sc.objects if o.type == 'MESH']
if hide:
    for o in objs:
        if hide.search(o.name) or (o.parent and hide.search(o.parent.name)):
            o.hide_render = True
vis = [o for o in objs if not o.hide_render]
pts = [o.matrix_world @ Vector(c) for o in vis for c in o.bound_box]
lo = Vector([min(p[i] for p in pts) for i in range(3)])
hi = Vector([max(p[i] for p in pts) for i in range(3)])
ctr, rad = (lo + hi) / 2, (hi - lo).length / 2
print('BOUNDS', tuple(round(x, 2) for x in lo), tuple(round(x, 2) for x in hi))

sc.render.engine = 'BLENDER_WORKBENCH'
sc.display.shading.light = 'STUDIO'
sc.display.shading.color_type = 'TEXTURE'
sc.render.resolution_x, sc.render.resolution_y = 960, 640
sc.world = bpy.data.worlds.new('w'); sc.world.color = (0.15, 0.15, 0.18)
cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
sc.collection.objects.link(cam); sc.camera = cam
cam.data.clip_start, cam.data.clip_end = rad * 0.01, rad * 10
for tag, (az, el) in {'34': (35, 25), 'top': (0, 89), 'side': (90, 0), 'front': (180, 5)}.items():
    a, e = math.radians(az), math.radians(el)
    d = Vector((math.sin(a) * math.cos(e), math.sin(e), math.cos(a) * math.cos(e)))
    # glTF +Z (Blender -Y after import) ... just orbit in Blender space
    d = Vector((d.x, -d.z, d.y))
    cam.location = ctr + d * rad * 2.6
    cam.rotation_euler = (ctr - cam.location).to_track_quat('-Z', 'Y').to_euler()
    sc.render.filepath = '%s_%s.png' % (prefix, tag)
    bpy.ops.render.render(write_still=True)
