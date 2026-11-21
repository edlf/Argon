# Ghidra Jython: the ship classes' airbrake flaps. See docs/physics/turbo-flaps-cameras.md "Airbrake flaps"; ported in src/craft/craft_flaps.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00058e60: ('craft_class_create', 'Ship class per pilot id (jump table 0x59754): vtables 0x1a0254.., setup slot 0, part animation slot 2.'),
    0x000504a0: ('craft_flaps_shared', 'Two flaps: brake: a += 4 dt; else demand (airbrake + steer) x k, eased at rate (x2 opening); 0..1; pitch -a (flag 0).'),
    0x00050400: ('craft_bones_refresh', 'Per instance 0xb99f0 on the class bone list.'),
    0x000b99f0: ('bone_set_rotation', 'Local rows 0..2 = D3DXMatrixRotationYawPitchRoll(y, x, z) rows x row scale +0x11c; angles to +0x110.'),
    0x000b9c40: ('cmm_find_bone', 'Bone index by name (+0x140), -1 if none.'),
    0x000576c0: ('craft_flaps_vladimir', 'Demand x 0.003, rates 8 / 4; left yaw -a, right yaw +a.'),
    0x00057990: ('craft_flaps_yuri', 'Airbrake slot x 0.009, rates 8 / 4, + climb-dive tilt x 0.001; -0.9..0.9; pitch +a.'),
    0x00051d40: ('craft_flaps_abi', 'Airbrake slot x 0.009, rates 8 / 4, + climb-dive tilt x 0.0003; -0.9..0.9; pitch +0.4 a.'),
    0x00054b90: ('craft_flaps_kumiko', 'AIRBRAKE_MAIN: max(airbrakes) x 0.01 at 5 / s, 0..1; pitch -a.'),
}
tx = currentProgram.startTransaction('QR flaps labels')
done = 0
try:
    for a, (n, cmt) in names.items():
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            continue
        f.setName(n, SourceType.USER_DEFINED)
        f.setComment(cmt)
        done += 1
finally:
    currentProgram.endTransaction(tx, True)
print('QR flaps labels applied: %d' % done)
