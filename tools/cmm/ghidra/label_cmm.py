# Ghidra Jython script: write .cmm model-format findings into the project.
# Renames loader functions, adds plate comments, and creates struct types
# QR/CmmHeader, QR/CmmMesh, QR/CmmMaterial, QR/CmmBone.
from ghidra.program.model.data import (StructureDataType, CategoryPath, DataTypeConflictHandler,
                                       IntegerDataType, UnsignedIntegerDataType, ShortDataType,
                                       FloatDataType, ArrayDataType, CharDataType, PointerDataType,
                                       Undefined1DataType)
from ghidra.program.model.symbol import SourceType
from ghidra.program.model.listing import CodeUnit

cat = CategoryPath('/QR')
dtm = currentProgram.getDataTypeManager()
I, U, S, F = IntegerDataType.dataType, UnsignedIntegerDataType.dataType, ShortDataType.dataType, FloatDataType.dataType
P = PointerDataType.dataType

def struct(name, size, fields):
    s = StructureDataType(cat, name, size)
    for off, dt, fname, cmt in fields:
        s.replaceAtOffset(off, dt, dt.getLength(), fname, cmt)
    return dtm.addDataType(s, DataTypeConflictHandler.REPLACE_HANDLER)

mat44 = ArrayDataType(F, 16, 4)

tx = currentProgram.startTransaction('QR cmm labels')
try:
    struct('CmmHeader', 0x48, [
        (0x00, I, 'version', '300'),
        (0x04, U, 'num_meshes', 'CmmMesh[] at +0x48, 200 bytes each'),
        (0x08, U, 'num_textures', 'tex%d.dds / lotex%d.dds'),
        (0x0c, U, 'flags', '0x1000 = texture table present'),
        (0x14, P, 'meshes', ''),
        (0x18, I, 'num_materials', 'CmmMaterial[] 0xec each'),
        (0x1c, P, 'materials', ''),
        (0x20, I, 'num_bones', 'CmmBone[] 0x1a0 each, 16-aligned'),
        (0x24, P, 'bones', ''),
        (0x2c, P, 'texture_table', 'num_textures * 0x10'),
        (0x30, I, 'num_rec5bc', 'records of 0x5bc + size at +0x50'),
        (0x34, P, 'rec5bc', ''),
        (0x40, I, 'num_rec80', 'records of 0x80'),
        (0x44, P, 'rec80', ''),
    ])
    struct('CmmMesh', 200, [
        (0x00, I, 'bone', 'owning bone index'),
        (0x04, I, 'unk04', ''),
        (0x08, U, 'num_verts', ''),
        (0x0c, U, 'flags', '1=tristrip 2=morph 0x20=pushbuffer blk 0x100 0x40000=skinned'),
        (0x18, P, 'verts', 'stride by vtype: 0..3 = 0x24+8n (pos,nrm,D3DCOLOR,uv*), 4..7 = 0x18+8n (packed nrm)'),
        (0x1c, P, 'morph_or_skin', ''),
        (0x20, P, 'pushbuffer', 'size-prefixed block'),
        (0x28, U, 'num_faces', 'strip: faces+2 u16 indices'),
        (0x2c, P, 'indices', ''),
        (0x34, U, 'num_tex', 'texture stages'),
        (0x38, ArrayDataType(S, 4, 2), 'tex', 'texture index per stage'),
        (0x48, S, 'material', 'CmmMaterial index'),
        (0x4c, ArrayDataType(F, 3, 4), 'bone_world_pos', ''),
        (0xb4, S, 'runtime_index', 'set by loader'),
        (0xc2, S, 'vtype', 'vertex format 0..7'),
    ])
    struct('CmmMaterial', 0xec, [
        (0x24, ArrayDataType(F, 3, 4), 'stage0_uv_scroll', 'loader checks != 0'),
        (0x58, ArrayDataType(F, 3, 4), 'stage1_uv_scroll', ''),
        (0x8c, ArrayDataType(F, 3, 4), 'stage2_uv_scroll', ''),
        (0xc0, ArrayDataType(F, 3, 4), 'stage3_uv_scroll', ''),
    ])
    struct('CmmBone', 0x1a0, [
        (0x000, mat44, 'local', 'D3D row-vector, translation in row 3'),
        (0x080, mat44, 'world', 'as saved; spinning parts mid-rotation'),
        (0x11c, ArrayDataType(F, 3, 4), 'scale', ''),
        (0x128, I, 'index', ''),
        (0x12c, I, 'num_meshes', ''),
        (0x140, ArrayDataType(CharDataType.dataType, 0x30, 1), 'name', ''),
        (0x180, S, 'parent', ''),
        (0x182, S, 'first_child', ''),
        (0x184, S, 'prev_sibling', ''),
        (0x186, S, 'next_sibling', ''),
    ])

    names = {
        0x000b5c40: ('cmm_model_load', 'Opens data\\%s\\model.cmm via CResourceServer; see docs\\models\\README.md'),
        0x000b6070: ('cmm_fixup_pointers', 'Walks a loaded .cmm (raw struct dump) and rebuilds every pointer\n'
                     'in file order: CmmMesh[n] -> CmmMaterial[] -> CmmBone[] (align16) -> pushbuffer blocks\n'
                     '-> vertices (+morph/skin) per mesh -> indices per mesh -> texture table -> rec5bc -> rec80.\n'
                     'Then loads tex%d.dds / lotex%d.dds. Python reader: tools\\cmm\\cmm.py'),
        0x000b59c0: ('cmm_reload_textures', 'Re-binds bones/textures for an already fixed-up cmm (shared resource)'),
        0x000b9540: (None, 'called from cmm_fixup_pointers for CmmBone[]'),
    }
    fm = currentProgram.getFunctionManager()
    for a, (n, cmt) in names.items():
        addr = toAddr(a)
        f = fm.getFunctionAt(addr)
        if f is None:
            continue
        if n:
            f.setName(n, SourceType.USER_DEFINED)
        old = f.getComment() or ''
        if cmt not in old:
            f.setComment((old + '\n' if old else '') + cmt)
    print('QR cmm labels applied')
finally:
    currentProgram.endTransaction(tx, True)
