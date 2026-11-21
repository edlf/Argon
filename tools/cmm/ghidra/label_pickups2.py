# Ghidra Jython: pickups (collide, level-up, burst, particle systems, sound). See docs\effects\README.md ("Pickups");
# ported in src/app/app_pickups.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x0008fcd0: ('pickups_collide', '(pos, radius, time, int* type, AI flag) -> index or -1. Active pickups: |d| < radius per axis and in\n'
                 'length; player skips ones taken this lap (0x52c860); takes: active = 0, time, flag, sound counter; inactive\n'
                 'pickups come back after time + 0.4 (not power chips) with pickup_burst(pos, 0, 20, colour). See docs\\effects\\README.md.'),
    0x0006ba30: ('weapon_pickup', 'EAX = craft weapon. Type 1 red +0xa4, 0 blue +0xa8, 2 yellow +0xac (+1, caps 0x52cd0c/12/18), 3 mega-up all;\n'
                 'all three at 3 -> special +0xb0 (cap 0x52cd1e). Logs "Picked up %s", then pickup_burst(ship, 1, 10, colour).'),
    0x00088750: ('pickup_burst', 'EAX = source pos. 75 directions x 2 rounds x 3 particles into the starburst system 0x1baa28: velocity\n'
                 'size x (cos a cos b, cos a sin b, sin a), x 1.1 per particle, x 3 per round; colour halves per particle;\n'
                 'duration 0.5 + (rand & 127) / 255; mode 3 (implode + follow) when flag, else 1 (implode). colour 0 = random.'),
    0x0008ffa0: ('pickup_respawn_event', 'Replay event 9: pickup EAX active again + pickup_burst(pos, 0, 20, type colour).'),
    0x00088660: ('particles_init', 'Creates the particle systems: 0x1baa28 starburst.dds (1000, size 0.6, gravity -10, blend 2),\n'
                 '0x1baa80 starburstred.dds (500, 0.5, -13), 0x1baad8 bubble.dds (1000, 0.35, 0).'),
    0x00088700: ('particles_update_all', 'Updates the three particle systems with the game time.'),
    0x000bc640: ('particle_system_update', 'Drops expired particles; mode bit 1 implode (pos = src + vel x (1 - t)), bit 2 src from pointer,\n'
                 'else pos += vel dt, vel.y += gravity dt; colour lerp from -> to; writes the point vertex buffer.'),
    0x000bc900: ('particle_system_draw', 'Point sprites (POINTLIST), point size = system +0x10 scaled by distance, min 4 / max 16, no fog.'),
    0x0006d000: ('craft_weapon_update', 'Per frame (param flag, time): weapon selection, pickups_collide(craft +0x200, 14, ...), weapon_pickup or\n'
                 'power chip log, replay event, pickup sound 0xc3470.'),
    0x000c3470: ('sound_pickup', 'ECX = pickup type: 0..2 SF_GENERIC_POWERPICKUP (0x1a), 3 SF_GENERIC_MEGAPICKUP (0x1c), else CHIPPICKUP (0x1b).'),
    0x0006c110: ('weapon_select', 'ECX = weapon (1 red, 2 yellow, 3 blue, 4 special): +0x9c = level x 3 - 3 / -2 / -1, special 9.'),
}
comments = {
    0x0008fbc0: 'Called twice per type (0x93f1b / 0x93f3e): flag 0 = normal model set (+4), flag 1 = _ns set (+0x1e54) for pickups\n'
                'the player took this lap (0x52c860). Only active pickups (+0x14).',
}
labels = {
    0x0052b0a0: 'g_pickups',
    0x0052afb4: 'g_pickup_count',
    0x0052c860: 'g_pickup_taken_this_lap',
    0x001baa28: 'g_particles_starburst',
    0x001baa80: 'g_particles_starburstred',
    0x001baad8: 'g_particles_bubble',
    0x0052cd0c: 'g_weapon_cap_red',
    0x0052cd12: 'g_weapon_cap_blue',
    0x0052cd18: 'g_weapon_cap_yellow',
    0x0052cd1e: 'g_weapon_cap_special',
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR pickup labels')
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
    print('QR pickup labels applied: %d' % done)
finally:
    currentProgram.endTransaction(tx, True)
