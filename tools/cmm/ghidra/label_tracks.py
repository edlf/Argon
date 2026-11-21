# Ghidra Jython: names/comments for track (environment) loading and collision build.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x000af910: ('track_env_load', 'Track (environment) load: cmm_fixup_pointers on data\\tracks\\<t>\\model.cmm, then\n'
                 'visdata.vis, locator.dat, envdata\\quadsprite.dds, sprite.dat, prop.dat,\n'
                 'then collision_build_from_cmm(collision_get(), cmm header).'),
    0x000d29c0: ('collision_build_from_cmm', 'Builds track collision from the render meshes. A mesh is solid when\n'
                 'mesh.flags & 0x20 and its 48-byte surface block (+0x20) has (+0x0c & 2).\n'
                 'Surface block +0x14/+0x18 copied per mesh; triangles split into nodes of <=256 with AABBs.'),
    0x000d1b70: ('collision_get', 'Singleton for the collision system (0x81f4 bytes)'),
    0x000b03f0: ('track_load_prop_dat', 'Copies prop.dat resource (0x45e dwords) into global table at 0x390fa8'),
    0x000969c0: (None, 'loads tracks\\%s\\sky model via cmm_model_load'),
    0x000af270: ('locator_dat_fixup', 'locator.dat: u32 count @0, entries (0xc0) @+0x10: name[64], m44 @+0x40,\n'
                 'pos @+0x84, rot deg (x,y,z; Rx*Ry*Rz) @+0x90, scale @+0x9c. tools\\cmm\\trackdata.py'),
    0x000af320: ('sprite_dat_fixup', 'sprite.dat: u32 count @0, entries (0x84) @+0x18 -> quadsprite_AddCMMSprites'),
    0x000bdf40: (None, 'sprite.dat entry: s16 frame, +4 flags(bit0 additive), +8 pos, +0x14..+0x38 corner offsets,\n'
                 '+0x44 per-corner values, +0x54.. UVs'),
}
tx = currentProgram.startTransaction('QR track labels')
try:
    for a, (n, cmt) in names.items():
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            print('no function at %x' % a); continue
        if n:
            f.setName(n, SourceType.USER_DEFINED)
        old = f.getComment() or ''
        if cmt not in old:
            f.setComment((old + '\n' if old else '') + cmt)
    print('QR track labels applied')
finally:
    currentProgram.endTransaction(tx, True)
