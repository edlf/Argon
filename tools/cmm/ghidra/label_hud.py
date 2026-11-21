# Ghidra Jython: in-race HUD (gamehud.cpp) names. See docs\physics\README.md (HUD), ported in src\hud\game_hud.cpp (GameHud).
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x000448b0: ('CGameHud_ctor', 'CGameHud constructor: builds the HUD element tree (groups 0x19aec0, texts 0x19afc0, sprites 0x19b0d0).'),
    0x00046040: ('gamehud_setup', 'Creates/configures the HUD elements: strings from race_strings.str, colours (labels 0xff80d0ff, values white,\n'
                 'prev/best 0xffffc864), fonts (0 = Square721 18, 1 = 34), flags (text +0x204).'),
    0x00045270: ('gamehud_layout', 'Positions the elements in the 640x480 safe area: single player L/R = vx+-52, T/B = vy+-36.\n'
                 'Lap (R-4,T) / lap value (R-4,T+10) / time,prev,best labels (R-114,T+52/66/80) values (R-4,..) /\n'
                 'km/h (R-3,B-28) / speed (R-55,B-40) / speedo overlay (R-167,B-166) / minimap (L+4,B-232).'),
    0x000496f0: ('gamehud_update', 'Per frame: huddata (+0x5588) -> texts. Speed "%d" (+0x14), turbo "%.1fs" (+0xc), shield "%d" (+0x48),\n'
                 'lap "%d[F:0] / %d" (min(lap, 0x52cdd8) / laps; "--" when 0), lap times via hud_time_format.'),
    0x0004a100: ('gamehud_render', 'Draws the HUD: speedo overlay, turbo fan (7 verts, centre (R-71,B-70), 225 deg - k*f; f from turbo secs,\n'
                 'colour 0xe040ff40 / blinking), "Turbo: %.1fs" box, needles via hud_draw_rotated_quad (len 96 w 24: km/h*0.12008;\n'
                 'len 64 w 16: smoothed huddata+0x18*1.441), shield bar (fill 0xffffd000, red blink below 0.33).'),
    0x0004f090: ('hud_minimap_draw', 'Minimap element (vtable 0x1a0df8) draw: data\\hud\\minimap\\<t>.dds, UV centre = (pos.xz/9.9 + 256)/512\n'
                 '(11.88 in modes 2/10), rotated by 180 + atan2(fwd.x, fwd.z); quad 80x160, top corners 0xb0ffffff, bottom 0x00ffffff,\n'
                 'centre 0xffffffff, tris (0,1,2)(2,1,3)(0,1,4); other crafts as 12 px icons (0xcbc40); position list with portraits.'),
    0x000cbe50: ('hud_draw_rotated_quad', '(x, y, angle_deg, ?, len, width): textured quad from the pivot along (sin a, cos a); V = 0 at the tip.'),
    0x000cbc40: ('hud_draw_icon', '(x, y, angle, size): rotated craft icon on the minimap.'),
    0x000c24d0: ('hud_draw_bar', 'Bar: back rect (colour in EAX) + fill of value*0.01*len (colour in EBX), height 20.'),
    0x000b1660: ('font_draw_text', 'Text renderer: glyph quad (u1-u0, v1-v0) * tex size at the pen, pen += .map array 5 * tex size;\n'
                 '"[F:n]" switches font; flags &0xf align (0 L, 1 R, 2 C), &0xf0 vertical, &0xf000 per-line align.'),
    0x0001f860: ('hud_time_split', 'Seconds -> minutes / seconds / hundredths (invalid when >= 999999).'),
    0x0001f910: ('hud_time_format', 'Time text "%02d:%02d.%02d"; unset -> "00:00.00" (arg 1, current lap) or "--:--.--".'),
    0x000af0a0: ('hud_group_draw', 'HUD group element +0x2c: draws children.'),
    0x000af140: ('hud_elem_set_pos', 'HUD element +0x40: position (+0x24/+0x28).'),
    0x000af180: ('hud_elem_set_size', 'HUD element +0x44: size (+0x3c/+0x40).'),
    0x000af1a0: ('hud_elem_set_scale', 'HUD element +0x48: scale (+0x30/+0x34; UV extent for sprites).'),
    0x000af1c0: ('hud_elem_set_visible', 'HUD element +0x4c: visible flag (bit 8 of +0x1c4).'),
    0x000ad620: ('hud_text_draw', 'Text element +0x2c draw: colour +0x1a0, font +0x1d4, flags +0x204 -> font_draw_text.'),
    0x000ad480: ('hud_text_set_scale', 'Text element +0x48.'),
    0x000ad290: ('hud_text_set_string', 'Text element +0x64: set the string.'),
    0x000abaf0: ('hud_sprite_draw', 'Sprite element +0x2c draw: texture +0x1d4, UV offset +0x1fc/+0x200, top-left anchored.'),
    0x000ad1b0: ('hud_text_ctor', 'Text element constructor (vtable 0x19afc0).'),
    0x000ab8b0: ('hud_sprite_ctor', 'Sprite element constructor (vtable 0x19b0d0).'),
    0x000aed50: ('hud_group_ctor', 'Group element constructor (vtable 0x19aec0).'),
}
tx = currentProgram.startTransaction('QR HUD labels')
done = 0
try:
    for a, (n, cmt) in names.items():
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            print('no function at %x' % a); continue
        f.setName(n, SourceType.USER_DEFINED)
        old = f.getComment() or ''
        if cmt not in old:
            f.setComment((old + '\n' if old else '') + cmt)
        done += 1
    print('QR HUD labels applied: %d' % done)
finally:
    currentProgram.endTransaction(tx, True)
