# Ghidra Jython: opponents (catch-up, overtaking, craft vs craft contacts). See docs/physics/README.md ("Opponents");
# ported in src/app/app_race.cpp and craftsim.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x0006d650: ('ai_catch_up', 'AI engine gain / power per frame (see docs/physics/README.md, Opponents): head start, 5 distance zones vs the player\n'
                 '(gain + zone modifier, power x (1 + ZoneEnginePowerModifier / 100)), place factor, pilot + player-is gains.'),
    0x000e5990: ('ai_values_load', 'Parses config/aivalues_<pilot>.txt and config/ai_values_<track>.txt into 0x43a014.'),
    0x000e4be0: ('ai_tactics_points', 'P1/P2/P3 at radius 1 / 1+e+x / 21+e+x; overtaking: < 5 % behind the craft ahead -> points move\n'
                 'sideways by up x (P1 - P3) x (10 - 3 gap) clamped +-5, side opposite to the craft ahead (timer 8..15 s).'),
    0x000cf920: ('physics_objects_contacts', 'After every object stepped: hull boxes into the sweep and prune (0xd5840), pairs (0xd4410),\n'
                 'contacts bound to the two states and resolved (two-body branch of contact_response 0xd5140).'),
    0x000d4410: ('contacts_object_pairs', 'One contact per overlapping pair: point between the box centres, normal centre a -> centre b, type 1.'),
    0x000d4d30: ('broadphase_pairs', 'Sweep and prune on x / y / z (0xd4dc0 bits 1/2/4): pairs overlapping on all three (== 7).'),
}
comments = {
    0x000d5140: 'Two-body branch (contact type != 0): J = m_a (v_a - v_b).n; J > 0: J += 5, P_a -= 0.7 J n, P_b += 0.7 J n,\n'
                'v_a = v_b = P_a / m_a (as shipped).',
}
labels = {
    0x0043a014: 'g_ai_values',
    0x005972f8: 'g_race_slots',
    0x00596e00: 'g_race_place',
}

def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR opponent labels')
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
    print('QR opponent labels applied: %d' % done)
finally:
    currentProgram.endTransaction(tx, True)
