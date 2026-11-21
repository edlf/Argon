# Ghidra Jython: weapons shared core (records, firing, projectiles, hits, damage). See docs/weapons/README.md;
# ported in src/app/app_weapons.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x000640c0: ('weapons_load', 'data/weapon.nwp: 16 pilot blocks of 0x2384 (11 records of 0x334 at +0x48) -> 0x1c5270, 0x1480 bytes -> 0x1e8bb8. docs/weapons/README.md.'),
    0x0006c110: ('weapon_select', 'Fire colour 1 red / 2 yellow / 3 blue / 4 special: slot (level-1)*3+colour or 9; launch all +0x168 projectiles; level and special -> 0.'),
    0x00064e20: ('weapon_launch', 'Projectile i: show its ship-model bone, speed = fwd speed x rec+0x198 % + rec+0x19c, inherit rec+0x180 % of the craft velocity.'),
    0x00066d30: ('weapon_projectile_update', 'Life +0x140, homing (+0x13e, lock +0x188, delay +0x194, turn +0x160), accel +0x174 / max +0x178, drop +0x17c, hugging +0x152/4/5 at +0x15c, bounces +0x165/+0x16c, craft hit 0x683b0, world ray.'),
    0x00065f80: ('weapon_impact', 'Impact kind 1 craft, 2 world, 3 expired, 4 bounce.'),
    0x00065e60: ('weapon_projectile_done', 'WE_DEAD: projectile finished.'),
    0x00066650: ('weapon_trail', 'Trail particles: every rec+0xfc, type rec+0xec (1 starburst, 2 starburstred, 3 bubble, >3 sprites), colour +0xf4, life +0x128, drift +0x104, jitter +0x108.'),
    0x000683b0: ('weapon_craft_hit_test', 'Closest craft to the step segment within the radius (excluding the shooter); within 200 sets the craft danger flag +0xb5.'),
    0x0006acf0: ('weapon_damage', 'Target energy (+0) -= damage (scaled first, not decoded), max 100; attack / nemesis points 0x83f90, rumble 0x6d430/0x6d400.'),
    0x0006a390: ('weapon_fire_all', 'Launches every projectile of the selected slot.'),
}
comments = {}
labels = {
    0x001c5270: 'g_weapon_records',
    0x001e8bb8: 'g_weapon_common',
}

def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR weapon labels')
done = 0
try:
    for a, (n, cmt) in sorted(names.items()):
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            f = createFunction(toAddr(a), n)
            if f is None:
                print('could not create a function at %x' % a)
                continue
            print('created function %s at %x' % (n, a))
        f.setName(n, SourceType.USER_DEFINED)
        add_comment(f, cmt)
        done += 1
    for a, cmt in sorted(comments.items()):
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            print('no function at %x' % a)
            continue
        add_comment(f, cmt)
        done += 1
    st = currentProgram.getSymbolTable()
    for a, n in sorted(labels.items()):
        st.createLabel(toAddr(a), n, SourceType.USER_DEFINED)
        done += 1
    print('QR weapon labels applied: %d' % done)
finally:
    currentProgram.endTransaction(tx, True)
